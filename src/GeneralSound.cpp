/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2025
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Globals.h"
#include "Emulation.h"
#include "GeneralSound.h"

void GsPorts::initConnections()
{
    AddressableDevice::initConnections();

    REG_INPUT("dataIn", GsPorts::setDataIn);
    REG_INPUT("status", GsPorts::setStatus);

    m_dataOutput = registerOutput("dataOut");
    m_commandOutput = registerOutput("command");
    m_statusOutput = registerOutput("status");
}

void GsPorts::writeByte(int addr, uint8_t value)
{
    if (addr == 0) {
        // data register
        m_dataOut = value;
        m_status |= 0x80; // set data bit
        m_dataOutput->setValue(m_dataOut);
        m_statusOutput->setValue(m_status);
    } else {
        // command register
        m_command = value;
        m_status |= 0x01; // set command bit
        m_commandOutput->setValue(m_command);
        m_statusOutput->setValue(m_status);
    }
}


uint8_t GsPorts::readByte(int addr)
{
    if (addr == 0) {
        // data register
        m_status &= ~0x80; // unset data bit
        m_statusOutput->setValue(m_status);
        return m_dataIn;
    } else {
        // status register
        return m_status | 0x7e;
    }
}


void GsPorts::setDataIn(uint8_t dataIn)
{
    m_dataIn = dataIn;
}


void GsPorts::setStatus(uint8_t status)
{
    m_status = status;
}


void GsInternalPorts::initConnections()
{
    AddressableDevice::initConnections();

    REG_INPUT("dataIn", GsInternalPorts::setDataIn);
    REG_INPUT("status", GsInternalPorts::setStatus);
    REG_INPUT("command", GsInternalPorts::setCommand);

    m_dataOutput = registerOutput("dataOut");
    m_statusOutput = registerOutput("status");
    m_memPageOutput = registerOutput("memPage");

    m_volumeOutputs[0] = registerOutput("volume0");
    m_volumeOutputs[1] = registerOutput("volume1");
    m_volumeOutputs[2] = registerOutput("volume2");
    m_volumeOutputs[3] = registerOutput("volume3");
}


void GsInternalPorts::writeByte(int addr, uint8_t value)
{
    addr &= 0xf;

    switch (addr) {
    case 0:
        //memory page port
        m_memPage = value & m_pageMask;
        m_memPageOutput->setValue(m_memPage);
        break;
    case 3:
        // data out port
        m_dataOut = value;
        m_status |= 0x80;
        m_dataOutput->setValue(m_dataOut);
        m_statusOutput->setValue(m_status);
        break;
    case 5:
        // clear command bit port
        m_status &= ~0x01; // reset command bit
        m_statusOutput->setValue(m_status);
        break;
    case 6:
    case 7:
    case 8:
    case 9:
        // volume ports
        m_volumeOutputs[addr - 6]->setValue(value & 0x3f);
        break;
    case 0xa:
        m_status = (m_status & ~0x80) | ((~m_memPage << 6) & 0x80);
        m_statusOutput->setValue(m_status);
        break;
    case 0xb:
        break;
    default:
        break;
    }
}


uint8_t GsInternalPorts::readByte(int addr)
{
    addr &= 0xf;

    switch (addr) {
    case 1:
        // command port
        return m_command;
    case 2:
        // data in port
        m_status &= ~0x80; // clear data bit
        m_statusOutput->setValue(m_status);
        return m_dataIn;
    case 4:
        // status port
        return m_status;
    case 5:
        // clear command bit port
        m_status &= ~0x01; // reset command bit
        m_statusOutput->setValue(m_status);
        break;
    case 0xa:
        m_status = (m_status & ~0x80) | ((~m_memPage << 6) & 0x80);
        m_statusOutput->setValue(m_status);
        break;
    case 0xb:
        break;
    default:
        return 0;
    }
    return 0;
}


bool GsInternalPorts::setProperty(const std::string &propertyName, const EmuValuesList &values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "pageBits") {
        int bits = values[0].asInt();
        m_pageMask = (((1 << bits) - 1) & 0xFF);
        return true;
        return true;
    }

    return false;
}


void GsInternalPorts::setDataIn(uint8_t dataIn)
{
    m_dataIn = dataIn;
}


void GsInternalPorts::setStatus(uint8_t status)
{
    m_status = status;
}


void GsInternalPorts::setCommand(uint8_t command)
{
    m_command = command;
}


void GsSoundMem::initConnections()
{
    m_sampleOutputs[0] = registerOutput("sample0");
    m_sampleOutputs[1] = registerOutput("sample1");
    m_sampleOutputs[2] = registerOutput("sample2");
    m_sampleOutputs[3] = registerOutput("sample3");
}


void GsSoundMem::writeByte(int addr, uint8_t value)
{
    m_as->writeByte(addr, value);
}


uint8_t GsSoundMem::readByte(int addr)
{
    uint8_t res = m_as->readByte(addr);
    if (addr >= 0x2000)
        m_sampleOutputs[(addr >> 8) & 3]->setValue(res);
    return res;
}


bool GsSoundMem::setProperty(const std::string &propertyName, const EmuValuesList &values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "addrSpace") {
        m_as = static_cast<AddressableDevice*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


void GsSoundSource::initConnections()
{
    REG_INDEXED_INPUT("sample", GsSoundSource::setSample);
    REG_INDEXED_INPUT("volume", GsSoundSource::setVolume);
}


int GsSoundSource::calcValue()
{
    // not used since getSample is implemented
    return 0;
}


int GsSoundSource::getMinimumSampleValue()
{
    return - MAX_SND_AMP * 5 / 2;
}


StereoSample GsSoundSource::getSample()
{
    updateStats();

    int16_t outputs[4];

    int64_t ticks = g_emulation->getCurClock() - m_initClock;
    if (ticks == 0)
        return {0, 0};

    for (int i = 0; i < 4; i++) {
        outputs[i] = (int64_t(m_sumVals[i])) * MAX_SND_AMP / ticks / 8064;
        m_sumVals[i] = 0;
    }

    m_initClock = g_emulation->getCurClock();

    // max amp = 2.5
    if (true/*m_stereo*/) {
        int left =  (outputs[0] + outputs[1] + outputs[2] / 4 + outputs[3] / 4) * m_ampFactor / 100;
        int right =  (outputs[2] + outputs[3] + outputs[0] / 4 + outputs[1] / 4) * m_ampFactor / 100;
        return {left, right};
    } else {
        int mono = (outputs[0] + outputs[1] + outputs[2] + outputs[3]) * 5 * m_ampFactor / 8 / 100;
        return {mono, mono};
    }
}


void GsSoundSource::setSample(int ch, uint8_t sample)
{
    updateStats();
    m_curValues[ch] = sample;
}


void GsSoundSource::setVolume(int ch, uint8_t volume)
{
    updateStats();
    m_curVolumes[ch] = volume;
}


void GsSoundSource::updateStats()
{
    uint64_t curClock = g_emulation->getCurClock();

    int clocks = curClock - m_prevClock;
    for (int i = 0; i < 4; i++) {
        m_sumVals[i] += clocks * (int(m_curValues[i]) - 128) * m_curVolumes[i];
    }

    m_prevClock = curClock;
}
