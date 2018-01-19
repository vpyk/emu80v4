/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2018
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

// GenericModules.cpp
// Реализация модулей общего назначения для быстрого прототипирования раздичных конфигураций

#include "Cpu.h"
#include "Emulation.h"
#include "GenericModules.h"

using namespace std;


PeriodicInt8080::PeriodicInt8080(Cpu8080Compatible* cpu, unsigned rst, unsigned freq)
{
    m_cpu = cpu;
    m_rst = rst;
    m_ticksPerInt = g_emulation->getFrequency() / freq;
}


void PeriodicInt8080::writeByte(int, uint8_t value)
{
    m_active = value & 1;
}


void PeriodicInt8080::operate()
{
    m_curClock += m_ticksPerInt;
    if (m_active)
        m_cpu->intRst(m_rst);
}


bool PeriodicInt8080::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "active") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_active = values[0].asString() == "yes";
            return true;
        }
    }
    return false;
}



PageSelector::PageSelector()
{
    m_value = 0;
}


void PageSelector::writeByte(int, uint8_t value)
{
    m_value = value;
    m_addrSpaceMapper->setCurPage(value);
}


uint8_t PageSelector::readByte(int)
{
    return m_value;
}


bool PageSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "mapper") {
        attachAddrSpaceMapper(static_cast<AddrSpaceMapper*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


void Splitter::addDevice(AddressableDevice* device)
{
    m_deviceVector.push_back(device);
    m_deviceCount++;
    m_devices = m_deviceVector.data();
}


void Splitter::writeByte(int addr, uint8_t value)
{
    m_value = value;
    for (unsigned i = 0;  i < m_deviceCount; i++)
        m_devices[i]->writeByte(addr, value);
}


uint8_t Splitter::readByte(int)
{
    if (m_readLastWritten)
        return m_value;
    else
        return m_readValue;
}


bool Splitter::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "device") {
        addDevice(static_cast<AddressableDevice*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "readValue") {
        m_readValue = values[0].asInt();
        return true;
    } else if (propertyName == "readLastWritten") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_readLastWritten = values[0].asString() == "yes";
            return true;
        }
    }
    return false;
}


Translator::Translator(AddressableDevice* device)
{
    m_device = device;
}

void Translator::writeByte(int addr, uint8_t value)
{
    addr >>= m_addrRShift;
    addr <<= m_addrLShift;
    addr &= m_addrAndMask;
    addr |= m_addrOrMask;
    addr ^= m_addrXorMask;
    addr += m_addrAddValue;
    addr -= m_addrAddValue;

    value >>= m_writeRShift;
    value <<= m_writeLShift;
    value &= m_writeAndMask;
    value |= m_writeOrMask;
    value ^= m_writeXorMask;
    value += m_writeAddValue;
    value -= m_writeAddValue;

    m_device->writeByte(addr, value);
}


uint8_t Translator::readByte(int addr)
{
    addr >>= m_addrRShift;
    addr <<= m_addrLShift;
    addr &= m_addrAndMask;
    addr |= m_addrOrMask;
    addr ^= m_addrXorMask;
    addr += m_addrAddValue;
    addr -= m_addrAddValue;

    uint8_t value = m_device->readByte(addr);

    value >>= m_readRShift;
    value <<= m_readLShift;
    value &= m_readAndMask;
    value |= m_readOrMask;
    value ^= m_readXorMask;
    value += m_readAddValue;
    value -= m_readAddValue;

    return value;
}


bool Translator::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "addrRShift") {
        m_addrRShift = values[0].asInt();
        return true;
    } else if (propertyName == "addrLShift") {
        m_addrLShift = values[0].asInt();
        return true;
    } else if (propertyName == "addrAndMask") {
        m_addrAndMask = values[0].asInt();
        return true;
    } else if (propertyName == "addrOrMask") {
        m_addrOrMask = values[0].asInt();
        return true;
    } else if (propertyName == "addrXorMask") {
        m_addrXorMask = values[0].asInt();
        return true;
    } else if (propertyName == "addrAddValue") {
        m_addrAddValue = values[0].asInt();
        return true;
    } else if (propertyName == "addrSubValue") {
        m_addrSubValue = values[0].asInt();
        return true;

    } else if (propertyName == "writeRShift") {
        m_writeRShift = values[0].asInt();
        return true;
    } else if (propertyName == "writeLShift") {
        m_writeLShift = values[0].asInt();
        return true;
    } else if (propertyName == "writeAndMask") {
        m_writeAndMask = values[0].asInt();
        return true;
    } else if (propertyName == "writeOrMask") {
        m_writeOrMask = values[0].asInt();
        return true;
    } else if (propertyName == "writeXorMask") {
        m_writeXorMask = values[0].asInt();
        return true;
    } else if (propertyName == "writeAddValue") {
        m_writeAddValue = values[0].asInt();
        return true;
    } else if (propertyName == "writeSubValue") {
        m_writeSubValue = values[0].asInt();
        return true;

    } else if (propertyName == "readRShift") {
        m_readRShift = values[0].asInt();
        return true;
    } else if (propertyName == "readLShift") {
        m_readLShift = values[0].asInt();
        return true;
    } else if (propertyName == "readAndMask") {
        m_readAndMask = values[0].asInt();
        return true;
    } else if (propertyName == "readOrMask") {
        m_readOrMask = values[0].asInt();
        return true;
    } else if (propertyName == "readXorMask") {
        m_readXorMask = values[0].asInt();
        return true;
    } else if (propertyName == "readAddValue") {
        m_readAddValue = values[0].asInt();
        return true;
    } else if (propertyName == "readSubValue") {
        m_readSubValue = values[0].asInt();
        return true;
    }
    return false;
}
