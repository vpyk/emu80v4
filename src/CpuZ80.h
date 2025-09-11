/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2026
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

#ifndef CPUZ80_H
#define CPUZ80_H

#include "Cpu.h"


struct Z80;

uint8_t fetchOpcodeCb(void* ctx, uint16_t address);
uint8_t readCb(void* ctx, uint16_t address);
void writeCb(void* ctx, uint16_t address, uint8_t value);
uint8_t inCb(void* ctx, uint16_t address);
void outCb(void* ctx, uint16_t address, uint8_t value);
uint8_t intaCb(void* ctx, uint16_t address);

class CpuZ80 : public Cpu8080Compatible
{
    friend uint8_t fetchOpcodeCb(void* ctx, uint16_t address);
    friend uint8_t readCb(void* ctx, uint16_t address);
    friend void writeCb(void* ctx, uint16_t address, uint8_t value);
    friend uint8_t inCb(void* ctx, uint16_t address);
    friend void outCb(void* ctx, uint16_t address, uint8_t value);
    friend uint8_t intaCb(void* ctx, uint16_t address);

    public:
        CpuZ80();
        ~CpuZ80();

        CpuType getType() override {return Cpu::CPU_Z80;}

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void operate() override;
        void reset() override;

        void intRst(int vect) override;
        void intCall(uint16_t addr) override;
        void ret() override;

        uint16_t getAF() override;
        uint16_t getBC() override;
        uint16_t getDE() override;
        uint16_t getHL() override;
        uint16_t getSP() override;
        uint16_t getPC() override;

        void setAF(uint16_t value) override;
        void setBC(uint16_t value) override;
        void setDE(uint16_t value) override;
        void setHL(uint16_t value) override;
        void setSP(uint16_t value) override;
        void setPC(uint16_t value) override;
        void setIFF(bool iff) override;

        bool getInte() override; // у Z80 нет inte, сохранено для эмуляции Z80-Card

        uint16_t getAF2();
        uint16_t getBC2();
        uint16_t getDE2();
        uint16_t getHL2();
        uint16_t getIX();
        uint16_t getIY();

        void setAF2(uint16_t value);
        void setBC2(uint16_t value);
        void setDE2(uint16_t value);
        void setHL2(uint16_t value);
        void setIX(uint16_t value);
        void setIY(uint16_t value);

        uint8_t  getIM();
        uint8_t  getI();
        uint8_t  getR();
        uint8_t  getIFF();

        void setIM(uint8_t value);
        void setI(uint8_t value);
        void setR(uint8_t value);

        bool checkForStackOperation() override;
        int getCurIoInstructionDuration() override;
        bool getM1Status();

        static EmuObject* create(const EmuValuesList&) {return new CpuZ80();}

    private:
        Z80* m_z80 = nullptr;

        bool m_m1Status = false;
        bool m_16bitPorts = false;
        bool m_stackOperation = false;
        int m_rstNum = 7;
        bool m_inte = false;
        bool m_performRet = false;

        unsigned simz80();

        uint8_t fetchOpcode(uint16_t address);
        uint8_t read(uint16_t address);
        void write(uint16_t address, uint8_t value);
        uint8_t in(uint16_t address);
        void out(uint16_t address, uint8_t value);
        uint8_t fetchIntOpcode(uint16_t address);
};

#endif // CPUZ80_H
