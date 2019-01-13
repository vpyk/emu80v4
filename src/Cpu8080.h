/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2018
 *
 *  Based on i8080 core object model code by:
 *  Alexander Demin <alexander@demin.ws> (https://github.com/begoon/i8080-core)
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

#ifndef CPU8080_H
#define CPU8080_H

#include "Cpu.h"


typedef union {
    struct {
        uint8_t l, h;
    } b;
    uint16_t w;
} reg_pair;

typedef struct {
    uint8_t carry_flag;
    uint8_t unused1;
    uint8_t parity_flag;
    uint8_t unused3;
    uint8_t half_carry_flag;
    uint8_t unused5;
    uint8_t zero_flag;
    uint8_t sign_flag;
} flag_reg;

struct i8080 {
    flag_reg f;
    reg_pair af, bc, de, hl;
    reg_pair sp, pc;
    uint16_t iff;
    uint16_t last_pc;
};

const int parity_table[] = {
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
};

const int half_carry_table[] = { 0, 0, 1, 0, 1, 0, 1, 1 };
const int sub_half_carry_table[] = { 0, 1, 1, 1, 0, 0, 0, 1 };

class Cpu8080 : public Cpu8080Compatible
{
    public:
        Cpu8080();

        CpuType getType() override {return Cpu::CPU_8080;}

        void reset() override;
        void operate() override;

        void intRst(int vect) override;
        void ret() override;

        uint16_t getAF() override;
        uint16_t getPC() override;
        uint16_t getBC() override;
        uint16_t getDE() override;
        uint16_t getHL() override;
        uint16_t getSP() override;
        //void exec(int nCmds) override;

        bool getInte() override;

        /*int getA() override;
        int getB() override;
        int getC() override;
        int getD() override;
        int getE() override;
        int getH() override;
        int getL() override;*/

        void setAF(uint16_t value) override;
        void setBC(uint16_t value) override;
        void setDE(uint16_t value) override;
        void setHL(uint16_t value) override;
        void setSP(uint16_t value) override;
        void setPC(uint16_t value) override;

        uint8_t getStatusWord() {return m_statusWord;}

        static EmuObject* create(const EmuValuesList&) {return new Cpu8080();}

private:
        struct i8080 cpu;

        void i8080_store_flags();
        void i8080_retrieve_flags();
        int i8080_execute(int opcode);

        uint8_t m_statusWord;
        int m_iffPendingCnt = 0;
};



class Cpu8080StatusWordSpace : public AddressableDevice
{
    public:
        Cpu8080StatusWordSpace(Cpu8080* cpu) {m_cpu = cpu;}
        void writeByte(int, uint8_t)  override {}
        uint8_t readByte(int)  override {return m_cpu->getStatusWord();}

        static EmuObject* create(const EmuValuesList& parameters) {return new Cpu8080StatusWordSpace(static_cast<Cpu8080*>(findObj(parameters[0].asString())));}

    protected:

    private:
        Cpu8080* m_cpu;
};

#endif // CPU8080_H
