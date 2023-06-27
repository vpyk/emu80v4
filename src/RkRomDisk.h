/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2023
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

#ifndef RKROMDISK_H
#define RKROMDISK_H

#include "Ppi8255Circuit.h"


class RkRomDisk : public Ppi8255Circuit
{
    public:
        //RkRomDisk() {} // явно не использовать, для производных классов
        RkRomDisk(std::string romDiskName);
        virtual ~RkRomDisk();

        uint8_t getPortA() override;
        uint8_t getPortB() override {return 0xff;}
        uint8_t getPortC() override {return 0xff;}
        void setPortA(uint8_t) override {}
        void setPortB(uint8_t) override;
        void setPortC(uint8_t) override;

        static EmuObject* create(const EmuValuesList& parameters) {return new RkRomDisk(parameters[0].asString());}

    protected:
        uint8_t* m_romDisk = nullptr;
        int m_fileSize = 0;
        unsigned m_curAddr = 0;
};


class ExtRkRomDisk : public RkRomDisk
{
    public:
        ExtRkRomDisk(std::string romDiskName) : RkRomDisk(romDiskName) {}

        void reset() override;

        void setPage(int page);

        static EmuObject* create(const EmuValuesList& parameters) {return new ExtRkRomDisk(parameters[0].asString());}
};


class RomDiskPageSelector : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void writeByte(int addr, uint8_t value) override;

        static EmuObject* create(const EmuValuesList&) {return new RomDiskPageSelector();}

    private:
        ExtRkRomDisk* m_romDisk = nullptr;
        uint8_t m_mask = 0;
};


#endif // RKROMDISK_H
