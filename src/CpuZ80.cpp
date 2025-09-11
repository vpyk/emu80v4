/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2026
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

// Emu80 wrapper for Zilog Z80CPU Emulator by Manuel Sainz de Baranda y Goñi.

#include "Globals.h"
#include "CpuZ80.h"
#include "CpuHook.h"
#include "CpuWaits.h"
#include "Emulation.h"
#include "PlatformCore.h"

#define Z80_STATIC
#define Z80_WITH_LOCAL_HEADER
#define Z80_EXTERNAL_HEADER "zdefs.h"
#include "z80/Z80.h"


using namespace std;


CpuZ80::CpuZ80()
{
    m_z80 = new Z80;

    m_z80->context = this;
    m_z80->fetch_opcode = fetchOpcodeCb;
    m_z80->fetch = readCb;
    m_z80->read = readCb;
    m_z80->write = writeCb;
    m_z80->in = inCb;
    m_z80->out = outCb;
    m_z80->nop = readCb;

    m_z80->halt = nullptr;
    m_z80->nmia = nullptr;

    m_z80->inta = intaCb;
    m_z80->int_fetch = nullptr; // todo

    m_z80->ld_i_a = nullptr;
    m_z80->ld_r_a = nullptr;
    m_z80->reti = nullptr;
    m_z80->retn = nullptr;
    m_z80->hook = nullptr;
    m_z80->illegal = nullptr;

    m_z80->options = 0;

    z80_power(m_z80, true);
}


CpuZ80::~CpuZ80()
{
    delete m_z80;
}


unsigned CpuZ80::simz80()
{
    int cycles = z80_run(m_z80, 1);

    if (m_z80->iff1 != m_inte) {
        m_inte = m_z80->iff1;
        if (m_core)
            m_core->inte(m_inte);
    }

    return cycles;
}


void CpuZ80::operate()
{
    if (!m_hooksDisabled) {
        bool retFlag = false;
        list<CpuHook*>* hookList = m_hookArray[getPC()/*m_z80->pc.uint16_value*/];
        if (hookList) {
            for (auto it = hookList->begin(); it != hookList->end(); it++)
                retFlag = retFlag || (*it)->hookProc();
        }
        if (retFlag)
            return;
    }

    if (m_waits) {
        int tag;
        int opcode = m_addrSpace->readByteEx(m_z80->pc.uint16_value, tag) + (m_addrSpace->readByte(uint16_t(m_z80->pc.uint16_value + 1)) << 8); // 2 bytes for z80
        int clocks = simz80();
        m_curClock += m_kDiv * (clocks + m_waits->getCpuWaitStates(tag, opcode, clocks));
    } else
        m_curClock += m_kDiv * simz80();

    if (m_stepReq) {
        m_stepReq = false;
        g_emulation->debugRequest(this);
    }
}


void CpuZ80::reset()
{
    m_z80->af.uint16_value = 0;
    m_z80->bc.uint16_value = 0;
    m_z80->de.uint16_value = 0;
    m_z80->hl.uint16_value = 0;
    m_z80->af_.uint16_value = 0;
    m_z80->bc_.uint16_value = 0;
    m_z80->de_.uint16_value = 0;
    m_z80->hl_.uint16_value = 0;
    m_z80->ix_iy[0].uint16_value = 0;
    m_z80->ix_iy[1].uint16_value = 0;
    m_z80->sp.uint16_value = 0;

    z80_instant_reset(m_z80);

    m_z80->pc.uint16_value = m_startAddr;

    m_m1Status = false;

    if (m_core)
        m_core->inte(false);
}


void CpuZ80::intRst(int vect)
{
    m_rstNum = vect;
    z80_int(m_z80, true);
}


void CpuZ80::intCall(uint16_t addr)
{
    // to be implemented
}


void CpuZ80::ret() {
    m_performRet = true;
    simz80();
}


uint16_t CpuZ80::getAF()
{
    return m_z80->af.uint16_value;
}


uint16_t CpuZ80::getBC()
{
    return m_z80->bc.uint16_value;
}


uint16_t CpuZ80::getDE()
{
    return m_z80->de.uint16_value;
}


uint16_t CpuZ80::getHL()
{
    return m_z80->hl.uint16_value;
}


uint16_t CpuZ80::getSP()
{
    return m_z80->sp.uint16_value;
}


uint16_t CpuZ80::getPC()
{
    if (!m_z80->halt_line)
        return m_z80->pc.uint16_value;
    else
        return m_z80->pc.uint16_value - 1;
}


bool CpuZ80::getInte()
{
    return m_z80->iff1;
}


void CpuZ80::setBC(uint16_t value)
{
    m_z80->bc.uint16_value = value;
}


void CpuZ80::setDE(uint16_t value)
{
    m_z80->de.uint16_value = value;
}


void CpuZ80::setHL(uint16_t value)
{
    m_z80->hl.uint16_value = value;
}


void CpuZ80::setSP(uint16_t value)
{
    m_z80->sp.uint16_value = value;
}


void CpuZ80::setPC(uint16_t value)
{
    m_z80->pc.uint16_value = value;
}



void CpuZ80::setAF(uint16_t value)
{
    m_z80->af.uint16_value = value;
}


uint16_t CpuZ80::getAF2()
{
    return m_z80->af_.uint16_value;
}


uint16_t CpuZ80::getBC2()
{
    return m_z80->bc_.uint16_value;
}


uint16_t CpuZ80::getDE2()
{
    return m_z80->de_.uint16_value;
}


uint16_t CpuZ80::getHL2()
{
    return m_z80->hl_.uint16_value;
}


uint16_t CpuZ80::getIX()
{
    return m_z80->ix_iy[0].uint16_value;
}


uint16_t CpuZ80::getIY()
{
    return m_z80->ix_iy[1].uint16_value;
}


uint8_t CpuZ80::getIM()
{
    return m_z80->im;
}


uint8_t CpuZ80::getI()
{
    return m_z80->i;
}


uint8_t CpuZ80::getR()
{
    return m_z80->r;
}


uint8_t CpuZ80::getIFF()
{
    return m_z80->iff1;
}


bool CpuZ80::checkForStackOperation()
{
    return m_stackOperation;
}


int CpuZ80::getCurIoInstructionDuration()
{
    return z80_out_cycle(m_z80) + 4;
}


bool CpuZ80::getM1Status()
{
    return m_m1Status;
}


void CpuZ80::setIM(uint8_t value) {
    m_z80->im = value;
}


void CpuZ80::setI(uint8_t value) {
    m_z80->i = value;
}


void CpuZ80::setR(uint8_t value) {
    m_z80->r = value;
}


void CpuZ80::setAF2(uint16_t value)
{
    m_z80->af_.uint16_value = value;
}


void CpuZ80::setBC2(uint16_t value)
{
    m_z80->bc_.uint16_value = value;
}


void CpuZ80::setDE2(uint16_t value)
{
    m_z80->de_.uint16_value = value;
}


void CpuZ80::setHL2(uint16_t value)
{
    m_z80->hl_.uint16_value = value;
}


void CpuZ80::setIX(uint16_t value)
{
    m_z80->ix_iy[0].uint16_value = value;
}


void CpuZ80::setIY(uint16_t value)
{
    m_z80->ix_iy[1].uint16_value = value;
}


void CpuZ80::setIFF(bool iff)
{
    m_z80->iff1 = iff;
}


bool CpuZ80::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (Cpu8080Compatible::setProperty(propertyName, values))
        return true;

    if (propertyName == "16bitPorts")
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_16bitPorts = values[0].asString() == "yes";
            return true;
        }

    return false;
}


uint8_t CpuZ80::fetchOpcode(uint16_t address)
{
    if (!m_performRet) {
        m_m1Status = true;
        uint8_t opcode = as_input(address);
        m_stackOperation = ((opcode & 0xc7) == 0xc5 || (opcode & 0xc7) == 0xc1);
        return opcode;
    } else {
        m_performRet = false;
        return 0xc9;
    }
}


uint8_t CpuZ80::read(uint16_t address)
{
    m_m1Status = false;
    return as_input(address);
}


void CpuZ80::write(uint16_t address, uint8_t value)
{
    as_output(address, value);
}


uint8_t CpuZ80::in(uint16_t address)
{
    return io_input(m_16bitPorts ? address : address & 0xFF);
}


void CpuZ80::out(uint16_t address, uint8_t value)
{
    io_output(m_16bitPorts ? address : address & 0xFF, value);
}


uint8_t CpuZ80::fetchIntOpcode(uint16_t address)
{
    m_m1Status = true;
    z80_int(m_z80, false); // !!! temporarily
    return (m_rstNum << 3) | 0xe7;
}


// Z80 Callbacks

uint8_t fetchOpcodeCb(void* ctx, uint16_t address)
{
    return static_cast<CpuZ80*>(ctx)->fetchOpcode(address);
}


uint8_t readCb(void* ctx, uint16_t address)
{
    return static_cast<CpuZ80*>(ctx)->read(address);
}


void writeCb(void* ctx, uint16_t address, uint8_t value)
{
    static_cast<CpuZ80*>(ctx)->write(address, value);
}


uint8_t inCb(void* ctx, uint16_t address)
{
    return static_cast<CpuZ80*>(ctx)->in(address);
}


void outCb(void* ctx, uint16_t address, uint8_t value)
{
    static_cast<CpuZ80*>(ctx)->out(address, value);
}


uint8_t intaCb(void* ctx, uint16_t address)
{
    return static_cast<CpuZ80*>(ctx)->fetchIntOpcode(address);
}
