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

#ifndef CPUZ80_H
#define CPUZ80_H

#include "Cpu.h"


/* two sets of 16-bit registers */
struct ddregs {
    uint16_t bc;
    uint16_t de;
    uint16_t hl;
};

class CpuZ80 : public Cpu8080Compatible
{
    public:
        CpuZ80();

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

        /*int getA()  override {return 0;}
        int getB()   override {return 0;}
        int getC()  override {return 0;}
        int getD()  override {return 0;}
        int getE()  override {return 0;}
        int getH()  override {return 0;}
        int getL()  override {return 0;}*/

        void setAF(uint16_t value) override;
        void setBC(uint16_t value) override;
        void setDE(uint16_t value) override;
        void setHL(uint16_t value) override;
        void setSP(uint16_t value) override;
        void setPC(uint16_t value) override;
        void setIFF(bool iff) override;
        //void exec(int nCmds) override;

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
        uint8_t  getIFF();

        bool checkForStackOperation() override {return m_stackOperation;}

        static EmuObject* create(const EmuValuesList&) {return new CpuZ80();}

    private:
        /* Z80 registers */
        uint16_t af[2];         /* accumulator and flags (2 banks) */
        int af_sel;             /* bank select for af */

        struct ddregs regs[2];  /* bc,de,hl */
        int regs_sel;           /* bank select for ddregs */

        uint16_t ir;            /* other Z80 registers */
        uint16_t ix;
        uint16_t iy;
        uint16_t sp;
        uint16_t pc;
        uint16_t IFF;
        uint16_t IM;

        int m_iffPendingCnt = 0;
        bool m_stackOperation = false;

        unsigned cb_prefix(unsigned adr);
        unsigned dfd_prefix(uint16_t& IXY);
        unsigned simz80();

        inline void incR() {ir = (ir & 0xff80) | ((ir + 1) & 0x7f);}

        bool m_16bitPorts = false;
};

#endif // CPUZ80_H
