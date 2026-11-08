/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2026
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

#include "DebugTarget.h"
#include "Cpu.h"
#include "CpuZ80.h"
#include "Cpu8080dasm.h"
#include "CpuZ80dasm.h"

using namespace std;


//##### CpuDebugTarget ####

CpuDebugTarget* CpuDebugTarget::create(Cpu* cpu, int id)
{
    Cpu8080Compatible* cpu8080 = dynamic_cast<Cpu8080Compatible*>(cpu);
    if (cpu8080)
        return new Cpu8080DebugTarget(cpu8080, id);
    return nullptr;
}


int CpuDebugTarget::findReg(const string& id)
{
    int n = getRegCount();
    for (int i = 0; i < n; i++)
        if (id == getRegInfo(i).id)
            return i;
    return -1;
}


uint8_t CpuDebugTarget::readMem(uint16_t addr)
{
    return debugReadByte(m_cpu->getAddrSpace(), addr);
}


void CpuDebugTarget::writeMem(uint16_t addr, uint8_t value)
{
    m_cpu->getAddrSpace()->writeByte(addr, value);
}


int CpuDebugTarget::getInstrLength(uint16_t addr)
{
    DisasmInstr instr;
    disassemble(addr, instr);
    return instr.length;
}


bool CpuDebugTarget::supportsBreakpoint(BreakpointType type)
{
    return getBreakpointClass(type) != BC_IO || m_cpu->getIoAddrSpace();
}


//##### Cpu8080DebugTarget ####

namespace {

enum Reg8080 {
    R_AF, R_BC, R_DE, R_HL, R_IX, R_IY, R_SP, R_PC,
    R_AF2, R_BC2, R_DE2, R_HL2,
    R_I, R_R, R_IM, R_IFF,
    R_FS, R_FZ, R_FH, R_FP, R_FN, R_FC
};

struct RegDesc {
    DebugRegInfo info;
    Reg8080 reg;
};

const RegDesc c_regs8080[] = {
    {{"af",  "AF",   16, DRG_MAIN},    R_AF},
    {{"bc",  "BC",   16, DRG_MAIN},    R_BC},
    {{"de",  "DE",   16, DRG_MAIN},    R_DE},
    {{"hl",  "HL",   16, DRG_MAIN},    R_HL},
    {{"sp",  "SP",   16, DRG_MAIN},    R_SP},
    {{"pc",  "PC",   16, DRG_MAIN},    R_PC},
    {{"iff", "INTE",  1, DRG_SPECIAL}, R_IFF},
    {{"s",   "S",     1, DRG_FLAG},    R_FS},
    {{"z",   "Z",     1, DRG_FLAG},    R_FZ},
    {{"ac",  "AC",    1, DRG_FLAG},    R_FH},
    {{"p",   "P",     1, DRG_FLAG},    R_FP},
    {{"c",   "C",     1, DRG_FLAG},    R_FC},
};

const RegDesc c_regsZ80[] = {
    {{"af",  "AF",   16, DRG_MAIN},    R_AF},
    {{"bc",  "BC",   16, DRG_MAIN},    R_BC},
    {{"de",  "DE",   16, DRG_MAIN},    R_DE},
    {{"hl",  "HL",   16, DRG_MAIN},    R_HL},
    {{"ix",  "IX",   16, DRG_MAIN},    R_IX},
    {{"iy",  "IY",   16, DRG_MAIN},    R_IY},
    {{"sp",  "SP",   16, DRG_MAIN},    R_SP},
    {{"pc",  "PC",   16, DRG_MAIN},    R_PC},
    {{"af2", "AF'",  16, DRG_ALT},     R_AF2},
    {{"bc2", "BC'",  16, DRG_ALT},     R_BC2},
    {{"de2", "DE'",  16, DRG_ALT},     R_DE2},
    {{"hl2", "HL'",  16, DRG_ALT},     R_HL2},
    {{"i",   "I",     8, DRG_SPECIAL}, R_I},
    {{"r",   "R",     8, DRG_SPECIAL}, R_R},
    {{"im",  "IM",    2, DRG_SPECIAL}, R_IM},
    {{"iff", "IFF",   1, DRG_SPECIAL}, R_IFF},
    {{"s",   "S",     1, DRG_FLAG},    R_FS},
    {{"z",   "Z",     1, DRG_FLAG},    R_FZ},
    {{"h",   "H",     1, DRG_FLAG},    R_FH},
    {{"p",   "P/V",   1, DRG_FLAG},    R_FP},
    {{"n",   "N",     1, DRG_FLAG},    R_FN},
    {{"c",   "C",     1, DRG_FLAG},    R_FC},
};

const int c_nRegs8080 = sizeof(c_regs8080) / sizeof(c_regs8080[0]);
const int c_nRegsZ80 = sizeof(c_regsZ80) / sizeof(c_regsZ80[0]);

// Flag bit in F
int flagBit(Reg8080 reg)
{
    switch (reg) {
    case R_FS: return 7;
    case R_FZ: return 6;
    case R_FH: return 4;
    case R_FP: return 2;
    case R_FN: return 1;
    default:   return 0; // R_FC
    }
}

} // namespace


Cpu8080DebugTarget::Cpu8080DebugTarget(Cpu8080Compatible* cpu, int id)
    : CpuDebugTarget(cpu, id), m_cpu8080(cpu), m_z80(dynamic_cast<CpuZ80*>(cpu))
{
}


const char* Cpu8080DebugTarget::getCpuName()
{
    return m_z80 ? "z80" : "8080";
}


int Cpu8080DebugTarget::getRegCount()
{
    return m_z80 ? c_nRegsZ80 : c_nRegs8080;
}


const DebugRegInfo& Cpu8080DebugTarget::getRegInfo(int index)
{
    return m_z80 ? c_regsZ80[index].info : c_regs8080[index].info;
}


unsigned Cpu8080DebugTarget::getReg(int index)
{
    Reg8080 reg = m_z80 ? c_regsZ80[index].reg : c_regs8080[index].reg;
    switch (reg) {
    case R_AF:  return m_cpu8080->getAF();
    case R_BC:  return m_cpu8080->getBC();
    case R_DE:  return m_cpu8080->getDE();
    case R_HL:  return m_cpu8080->getHL();
    case R_SP:  return m_cpu8080->getSP();
    case R_PC:  return m_cpu8080->getPC();
    case R_IFF: return m_cpu8080->getInte() ? 1 : 0;
    case R_IX:  return m_z80->getIX();
    case R_IY:  return m_z80->getIY();
    case R_AF2: return m_z80->getAF2();
    case R_BC2: return m_z80->getBC2();
    case R_DE2: return m_z80->getDE2();
    case R_HL2: return m_z80->getHL2();
    case R_I:   return m_z80->getI();
    case R_R:   return m_z80->getR();
    case R_IM:  return m_z80->getIM();
    default:    return (m_cpu8080->getAF() >> flagBit(reg)) & 1;
    }
}


void Cpu8080DebugTarget::setReg(int index, unsigned value)
{
    Reg8080 reg = m_z80 ? c_regsZ80[index].reg : c_regs8080[index].reg;
    uint16_t v = value;
    switch (reg) {
    case R_AF:  m_cpu8080->setAF(v); break;
    case R_BC:  m_cpu8080->setBC(v); break;
    case R_DE:  m_cpu8080->setDE(v); break;
    case R_HL:  m_cpu8080->setHL(v); break;
    case R_SP:  m_cpu8080->setSP(v); break;
    case R_PC:  m_cpu8080->setPC(v); break;
    case R_IFF: m_cpu8080->setIFF(v & 1); break;
    case R_IX:  m_z80->setIX(v); break;
    case R_IY:  m_z80->setIY(v); break;
    case R_AF2: m_z80->setAF2(v); break;
    case R_BC2: m_z80->setBC2(v); break;
    case R_DE2: m_z80->setDE2(v); break;
    case R_HL2: m_z80->setHL2(v); break;
    case R_I:   m_z80->setI(v & 0xFF); break;
    case R_R:   m_z80->setR(v & 0xFF); break;
    case R_IM:  m_z80->setIM(v & 0x03); break;
    default: {
        uint16_t mask = 1 << flagBit(reg);
        uint16_t af = m_cpu8080->getAF();
        m_cpu8080->setAF((v & 1) ? (af | mask) : (af & ~mask));
        break;
    }
    }
}


uint16_t Cpu8080DebugTarget::getPC()
{
    return m_cpu8080->getPC();
}


void Cpu8080DebugTarget::setPC(uint16_t value)
{
    m_cpu8080->setPC(value);
}


uint16_t Cpu8080DebugTarget::getSP()
{
    return m_cpu8080->getSP();
}


const char* Cpu8080DebugTarget::getDisasmSyntaxName(int syntax)
{
    return (m_z80 || syntax == 1) ? "z80" : "8080";
}


void Cpu8080DebugTarget::disassemble(uint16_t addr, DisasmInstr& instr, int syntax)
{
    uint8_t buf[4];
    for (int i = 0; i < 4; i++)
        buf[i] = readMem(uint16_t(addr + i));

    if (!m_z80 && syntax == 0) {
        instr.text = i8080GetInstructionMnemonic(buf);
        instr.length = i8080GetInstructionLength(buf);
        uint8_t op = buf[0];
        bool call = (op & 0xcf) == 0xcd /* CALL */ || (op & 0xc7) == 0xc7 /* RST */ || (op & 0xc7) == 0xc4 /* Cx */;
        instr.flow = call ? IFL_CALL : isReturn(addr) ? IFL_RETURN : IFL_NORMAL;
    } else {
        unsigned length;
        STEP_FLAG flag;
        instr.text = cpu_disassemble_z80(addr, buf, length, flag);
        instr.length = length;
        instr.flow = flag == SF_OVER ? IFL_CALL : isReturn(addr) ? IFL_RETURN : IFL_NORMAL;
    }
}


bool Cpu8080DebugTarget::isReturn(uint16_t addr)
{
    uint8_t op = readMem(addr);
    if (op == 0xc9 /* RET */ || (op & 0xc7) == 0xc0 /* Rcc */)
        return true;
    if (!m_z80)
        return op == 0xd9; // undocumented RET
    return op == 0xed && (readMem(uint16_t(addr + 1)) & 0xc7) == 0x45; // RETN/RETI
}
