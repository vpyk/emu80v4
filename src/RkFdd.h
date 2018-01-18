/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2017
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

#ifndef RKFDD_H
#define RKFDD_H

#include "EmuObjects.h"
#include "Ppi8255Circuit.h"

class FdImage;


class RkFddController;

class RkFddRegister : public AddressableDevice
{
    public:
        uint8_t readByte(int addr) override;
        void writeByte(int, uint8_t) override {};

        friend RkFddController;

    private:
        uint8_t m_value;
        RkFddController* m_fdd;

        void setValue(uint8_t value) {m_value = value;};
};

class RkFddController : public Ppi8255Circuit
{
    public:
        RkFddController();
        virtual ~RkFddController();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        uint8_t getPortA()  override {return 0xff;};
        uint8_t getPortC()  override {return 0xff;};
        uint8_t getPortB() override;
        void setPortA(uint8_t value) override;
        void setPortB(uint8_t) override {};
        void setPortC(uint8_t value) override;

        void attachRkFddRegister(RkFddRegister* reg) {m_fddReg = reg; reg->m_fdd = this;};
        uint8_t readCurByte();
        void writeCurByte(uint8_t bt);

        // Подключение образа диска
        void attachFdImage(int driveNum, FdImage* image);

        friend RkFddRegister;

    private:
        RkFddRegister* m_fddReg = nullptr;
        FdImage* m_images[2];   // Disk images

        int m_ticksPerByte;

        int m_drive;
        int m_track;
        int m_side;
        int m_pos;
        bool m_write = false;

        uint64_t m_prevClock;

        bool m_nextByteReady;
        bool m_index;
        bool m_ready;
        bool m_dirFwd;

        bool m_step = false;;
        bool m_prevStep = false;

        void updateState();
};

#endif // RKFDD_H
