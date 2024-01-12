/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2018
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

#ifndef MEM_H
#define MEM_H

#include <string>

#include "EmuObjects.h"


class Ram : public AddressableDevice
{
    public:
        //Ram();
        Ram(unsigned memSize);
        Ram(uint8_t* buf, unsigned memSize);
        //Ram(int memSize, std::string fileName);
        virtual ~Ram();
        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;
        /*const*/ uint8_t* getDataPtr() {return m_buf ? m_buf : m_extBuf;}
        uint8_t& operator[](int nAddr) {return m_buf[nAddr];} // no check for borders, use with caution
        int getSize() {return m_size;}

        static EmuObject* create(const EmuValuesList& parameters) {return parameters[0].isInt() ? new Ram(parameters[0].asInt()) : nullptr;}

    protected:

    private:
        int m_size;
        uint8_t* m_buf = nullptr;
        uint8_t* m_extBuf = nullptr;
};



class Rom : public AddressableDevice
{
    public:
        Rom();
        Rom(unsigned memSize, std::string fileName);
        virtual ~Rom();
        void writeByte(int, uint8_t)  override {}
        uint8_t readByte(int addr) override;
        virtual const uint8_t* getDataPtr() {return m_buf;}
        virtual const uint8_t& operator[](int nAddr) {return m_buf[nAddr];} // no check for borders, use with caution

        static EmuObject* create(const EmuValuesList& parameters) {return parameters[1].isInt() ? new Rom(parameters[1].asInt(), parameters[0].asString()) : nullptr;}

    protected:
        int m_size;
        uint8_t* m_buf = nullptr;
};



class NullSpace : public AddressableDevice
{
    public:
        NullSpace(uint8_t nullByte = 0xFF) {m_nullByte = nullByte;}
        void writeByte(int, uint8_t)  override {}
        uint8_t readByte(int)  override {return m_nullByte;}

        static EmuObject* create(const EmuValuesList& parameters) {return parameters[0].isInt() ? new NullSpace(parameters[0].asInt()) : nullptr;}

    protected:

    private:
        uint8_t m_nullByte = 0xFF;
};


#endif // MEM_H
