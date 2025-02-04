/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2025
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

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include <vector>

#include "EmuObjects.h"

/*struct DeviceItem
{
    AddressableDevice* addrDevice; // ссылка на устройство
    int firstAddress;              // начальный адрес устройства
    int itemSize;                  // размер области устройства в байтах
    int devFirstAddr;              // смещение в области памяти устройства
};*/

class AddrSpace : public AddressableDevice
{
    public:
        AddrSpace(uint8_t nullByte = 0xFF);

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        uint8_t readByte(int addr) override;
        void writeByte(int addr, uint8_t value) override;

        void addRange(int firstAddr, int lastAddr, AddressableDevice* addrDevice, int devFirstAddr = 0);
        virtual void addReadRange(int firstAddr, int lastAddr, AddressableDevice* addrDevice, int devFirstAddr = 0);
        virtual void addWriteRange(int firstAddr, int lastAddr, AddressableDevice* addrDevice, int devFirstAddr = 0);

        static EmuObject* create(const EmuValuesList&) {return new AddrSpace();}

private:
        uint8_t m_nullByte;          // байт, считываемый из нераспределенного пространства

        int m_itemCountR;            // количество элементов чтения
        std::vector<AddressableDevice*> m_devicesRVector; // вектор устройств для чтения
        std::vector<int> m_firstAddressesRVector;         // вектор начальных адресов устройств для чтения
        std::vector<int> m_itemSizesRVector;              // вектор размеров устройств для чтения в байтах
        std::vector<int> m_devFirstAddressesRVector;      // венктор смещений в области памяти устройств для чтения
        // указатели на области данных вышеуказанных векторов
        AddressableDevice** m_devicesR = nullptr;         // массив устройств для чтения
        int* m_firstAddressesR = nullptr;                 // массив начальных адресов устройств для чтения
        int* m_itemSizesR = nullptr;                      // массив размеров устройств для чтения в байтах
        int* m_devFirstAddressesR = nullptr;              // массив смещений в области памяти устройства для чтения

        int m_itemCountW;
        std::vector<AddressableDevice*> m_devicesWVector; // вектор устройств для записи
        std::vector<int> m_firstAddressesWVector;         // вектор начальных адресов устройств для записи
        std::vector<int> m_itemSizesWVector;              // вектор размеров устройств для записи в байтах
        std::vector<int> m_devFirstAddressesWVector;      // вектор смещений в области памяти устройств для записи
        // указатели на области данных вышеуказанных векторов
        AddressableDevice** m_devicesW = nullptr;         // массив устройств для чтения
        int* m_firstAddressesW = nullptr;                 // массив начальных адресов устройств для чтения
        int* m_itemSizesW = nullptr;                      // массив размеров устройств для чтения в байтах
        int* m_devFirstAddressesW = nullptr;              // массив смещений в области памяти устройства для чтения
};


class AddrSpaceMapper : public AddressableDevice
{
    public:
        AddrSpaceMapper(int nPages);
        ~AddrSpaceMapper();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        void reset() override {m_curPage = 0;}

        void attachPage(int page, AddressableDevice* as);
        void setCurPage(int page);

        void initConnections() override;

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        static EmuObject* create(const EmuValuesList& parameters) {return parameters[0].isInt() ? new AddrSpaceMapper(parameters[0].asInt()) : nullptr;}

protected:
        AddressableDevice** m_pages;
        int m_nPages;
        int m_curPage = 0;
};


class AddrSpaceShifter : public AddressableDevice
{
    public:
        AddrSpaceShifter(AddressableDevice* as, int shift);

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        static EmuObject* create(const EmuValuesList& parameters) {return parameters[1].isInt() ? new AddrSpaceShifter(static_cast<AddressableDevice*>(findObj(parameters[0].asString())), parameters[1].asInt()) : nullptr;}

private:
        AddressableDevice* m_as;
        int m_shift;
};


class AddrSpaceInverter : public AddressableDevice
{
    public:
        AddrSpaceInverter(AddressableDevice* as);

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        static EmuObject* create(const EmuValuesList& parameters) {return new AddrSpaceInverter(static_cast<AddressableDevice*>(findObj(parameters[0].asString())));}

private:
        AddressableDevice* m_as;
};


class AddrSpaceWriteSplitter : public AddressableDevice
{
    public:
        AddrSpaceWriteSplitter(AddressableDevice* as1, AddressableDevice* as2) : m_as1(as1), m_as2(as2) {}

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        static EmuObject* create(const EmuValuesList& parameters) {return new AddrSpaceWriteSplitter(static_cast<AddressableDevice*>(findObj(parameters[0].asString())),
                                                                                                     static_cast<AddressableDevice*>(findObj(parameters[1].asString())));}

private:
        AddressableDevice* m_as1;
        AddressableDevice* m_as2;
};


#endif // ADDRSPACE_H
