/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017
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

#ifndef GENERICMODULES_H
#define GENERICMODULES_H

#include "EmuObjects.h"
#include "AddrSpace.h"

class PeriodicInt8080 : public AddressableDevice, public IActive
{
    public:
        PeriodicInt8080(Cpu8080Compatible* cpu, unsigned rst, unsigned freq);

        // derived from EmuObject
        void writeByte(int addr, uint8_t value) override;
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from ActiveDevice
        void operate() override;

    private:
        bool m_active = false;    // признак активности
        unsigned m_ticksPerInt;   // тактов на прерывание
        unsigned m_rst;           // номер вектора прерывания
        Cpu8080Compatible* m_cpu; // процессор
};


class PageSelector : public AddressableDevice
{
    public:
        PageSelector();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void attachAddrSpaceMapper(AddrSpaceMapper* addrSpaceMapper) {m_addrSpaceMapper = addrSpaceMapper;};

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

    private:
        AddrSpaceMapper* m_addrSpaceMapper = nullptr;
        uint8_t m_value = 0;
};


class Splitter : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void addDevice(AddressableDevice* device);

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

    private:
        std::vector<AddressableDevice*> m_deviceVector;
        AddressableDevice** m_devices;
        uint8_t m_value = 0;
        unsigned m_deviceCount = 0;
        bool m_readLastWritten = false;
        uint8_t m_readValue = 0xFF;
};


class Translator : public AddressableDevice
{
    public:
        Translator(AddressableDevice* device);

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

    private:
        AddressableDevice* m_device;

        unsigned m_writeLShift  = 0;
        unsigned m_writeRShift  = 0;
        uint8_t m_writeAndMask  = 0xFF;
        uint8_t m_writeOrMask   = 0x00;
        uint8_t m_writeXorMask  = 0x00;
        uint8_t m_writeAddValue = 0x00;
        uint8_t m_writeSubValue = 0x00;
        unsigned m_readLShift   = 0;
        unsigned m_readRShift   = 0;
        uint8_t m_readAndMask   = 0xFF;
        uint8_t m_readOrMask    = 0x00;
        uint8_t m_readXorMask   = 0x00;
        uint8_t m_readAddValue  = 0x00;
        uint8_t m_readSubValue  = 0x00;
        unsigned m_addrLShift   = 0;
        unsigned m_addrRShift   = 0;
        uint16_t m_addrAndMask   = 0xFFFF;
        uint16_t m_addrOrMask    = 0x0000;
        uint16_t m_addrXorMask   = 0x0000;
        uint16_t m_addrAddValue  = 0x0000;
        uint16_t m_addrSubValue  = 0x0000;
};

#endif // GENERICMODULES_H
