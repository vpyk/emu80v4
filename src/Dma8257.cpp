/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2021
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

// io_vt57.cpp

// Реализация контроллера DMA КР580ВТ57

#include <sstream>
#include <iomanip>
#include <cmath>

#include <stdint.h>
#include <stdlib.h>

#include "Dma8257.h"
#include "Cpu.h"
#include "Emulation.h"

using namespace std;

Dma8257::Dma8257()
{
    performReset();
}



void Dma8257::performReset()
{
    for (int ch=0; ch<4; ch++) {
        m_addr[ch] = 0;
        m_count[ch] = 0;
    }

    m_modeReg = 0;
    m_statusReg = 0;
    m_isLoByte=true;
}



void Dma8257::attachAddrSpace(AddressableDevice *as)
{
    m_addrSpace = as;
}



void Dma8257::attachCpu(Cpu *cpu)
{
    m_cpu = cpu;
}



void Dma8257::writeByte(int addr, uint8_t value)
{
    addr &= 0x0f;
    if (addr > 8)
        return;
    else if (addr == 8) {
        // Mode register
        m_modeReg = value;
        m_isLoByte = true;
    } else {
        int ch = addr >> 1;
        if (addr & 1) {
            // Termnal count register
            if (m_isLoByte)
                m_count[ch] = (m_count[ch] & 0xFF00) | value;
            else
                m_count[ch] = (m_count[ch] & 0xFF) | (value << 8);
            if ((ch == 2) && (m_modeReg & 0x80)) {
                // Autoload - dub to ch. 3
                if (m_isLoByte)
                    m_count[3] = (m_count[3] & 0xFF00) | value;
                else
                    m_count[3] = (m_count[3] & 0xFF) | (value << 8);
                }
        } else {
            // Address register
            if (m_isLoByte)
                m_addr[ch] = (m_addr[ch] & 0xFF00) | value;
            else
                m_addr[ch] = (m_addr[ch] & 0xFF) | (value << 8);
            if ((ch == 2) && (m_modeReg & 0x80)) {
                // Autoload - dub to ch. 3
                if (m_isLoByte)
                    m_addr[3] = (m_addr[3] & 0xFF00) | value;
                else
                    m_addr[3] = (m_addr[3] & 0xFF) | (value << 8);
                }
        }
        m_isLoByte = !m_isLoByte;

        // update initial couner values for debug info
        m_initAddr[ch] = m_addr[ch];
        m_initCount[ch] = m_count[ch] & 0x3FFF;
    }
}



uint8_t Dma8257::readByte(int addr)
{
    uint8_t value;
    addr &= 0x0f;
    if (addr > 8)
        return 0xFF;
    else if (addr == 8) {
        // Status register
        int res = m_statusReg;
        m_statusReg &= 0xf0;
        return res;
    } else {
        int ch= addr >> 1;
        if (addr & 1) {
            // Address register
            if (m_isLoByte)
                value = (m_addr[ch] & 0xFF);
            else
                value = (m_addr[ch] & 0xFF00) >> 8;
            m_isLoByte = !m_isLoByte;
        } else {
            // Termnal count register
            if (m_isLoByte)
                value = (m_count[ch] & 0xFF);
            else
                value = (m_count[ch] & 0xFF00) >> 8;
            m_isLoByte = !m_isLoByte;
        }
    }
    return value;
}



bool Dma8257::dmaRequest(int channel, uint8_t &value, uint64_t clock)
{
    //if (m_addr[channel] == 0) return false;
      //value = 5;
    if  (!(m_modeReg & (1 << channel)))
        return false;
    if ((channel == 2) && (m_modeReg & 0x80) && ((m_count[channel] & 0x3fff) == 0x3fff)) {
        // Autoload before new cycle after TC
        m_addr[2] = m_addr[3];
        m_count[2] = m_count[3];
    }
    m_count[channel] = (m_count[channel] & 0xc000) | (((m_count[channel] & 0x3fff) - 1) & 0x3fff);
    if ((m_count[channel] & 0xc000) == 0x4000) {
        // write
        if (m_swapRw)
            value = m_addrSpace->readByte(m_addr[channel]);
        else
            m_addrSpace->writeByte(m_addr[channel], value);
    } else if ((m_count[channel] & 0xc000) == 0x8000) {
        // read
        if (m_swapRw)
            m_addrSpace->writeByte(m_addr[channel], value);
        else
            value = m_addrSpace->readByte(m_addr[channel]);
    }
    m_addr[channel]++;
    if ((m_count[channel] & 0x3fff) == 0x3fff) {
        // End of block transfer
        m_statusReg |= (1 << channel);;
        if ((m_modeReg & 0x40) && !((channel == 2) && (m_modeReg & 0x80)))
            // TC Stop
            m_modeReg &= ~(1 << channel);
    }
    if (m_cpu) {
        int offs = 0;
        if (clock) {
            //uint64_t cpuClock = m_cpu->getClock();
            int cpuKDiv = m_cpu->getKDiv();
            offs = clock % cpuKDiv;
            if (offs != 0)
                offs = cpuKDiv - offs;
              /*offs /= 105;
              if (offs == 3)
                  offs = cpuKDiv;
              else
                  offs = 0;*/
        }
        int dmaClocks = 4 * m_kDiv + offs;
        m_cpu->hrq(dmaClocks);
        m_consDmaClocks += dmaClocks;
    }
    return true;
}



// Get Mode register !!
uint8_t Dma8257::getMR()
{
    return m_modeReg;
}



bool Dma8257::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "addrSpace") {
        attachAddrSpace(static_cast<AddressableDevice*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "cpu") {
        attachCpu(static_cast<Cpu*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "swapRW") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_swapRw = values[0].asString() == "yes";
            return true;
        }
        return true;
    }

    return false;
}


string Dma8257::getDebugInfo()
{
    stringstream ss;
    ss << "DMA i8257:" << "\n";
    for (int ch = 0; ch <= 3; ch++) {
        if (m_modeReg & (1 << ch)) {
            ss << "Ch " << ch << ": ";
            int mode = m_count[ch] & 0xC000;
            if (mode == 0x8000)
                ss << "R";
            else if (mode == 0x4000)
                ss << "W";
            if (ch == 2 && m_modeReg & 0x80)
                ss << " AL";
            if (m_modeReg & 0x40)
                ss << " S";
            ss << "\n";
            ss << "A:" << setw(4) << setfill('0') << uppercase << hex << m_addr[ch] << "/" <<
                  setw(4) << setfill('0') << uppercase << hex << m_initAddr[ch] << "\n";
            ss << "C:" << setw(4) << setfill('0') << uppercase << hex << (m_count[ch] & 0x3FFF) << "/" <<
                  setw(4) << setfill('0') << uppercase << hex << m_initCount[ch] << "\n";
        }
    }
    return ss.str();
}


void Dma8257::calcDmaPercentage()
{
    uint64_t cpuClocks = m_cpu->getClock() - m_consStartCpuClock;
    m_dmaPercentage = (m_consStartCpuClock && cpuClocks) ? m_consDmaClocks * 100. / cpuClocks : 0.0;
    m_consStartCpuClock = m_cpu->getClock();
    m_consDmaClocks = 0;
}


string Dma8257::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = AddressableDevice::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "percentage") {
        // average within 1.5 s (every 3 times)
        m_calcCnt = (m_calcCnt + 1) % 3;
        if (!m_calcCnt)
            calcDmaPercentage();
        ostringstream oss;
        oss << setprecision(2) << m_dmaPercentage;
        return m_dmaPercentage != 0.0 ? oss.str() : "";
    }

    return "";
}
