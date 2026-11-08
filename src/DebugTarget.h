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

#ifndef DEBUGTARGET_H
#define DEBUGTARGET_H

#include <string>

#include "EmuObjects.h"
#include "Breakpoints.h"

class Cpu;
class Cpu8080Compatible;
class CpuZ80;


// The only way debugger code reads memory and devices. It is meant to have no side
// effects on the emulated machine; for now it is an ordinary readByte(), later it
// becomes a side-effect-free read (peekByte) in this single place.
inline uint8_t debugReadByte(AddressableDevice* device, int addr) {return device->readByte(addr);}


// Register groups, for front-ends to lay out registers
enum DebugRegGroup {
    DRG_MAIN,       // main registers, PC, SP
    DRG_ALT,        // alternate register set
    DRG_SPECIAL,    // interrupt state, refresh register etc.
    DRG_FLAG        // single flags (bits of a flag register)
};


struct DebugRegInfo {
    const char* id;     // lower case identifier: "af", "pc", "af2", "ix", "z"...
    const char* name;   // display name: "AF", "PC", "AF'", "IX", "Z"...
    int bits;           // 16, 8, 2, 1 (flags)
    DebugRegGroup group;
};


// Control flow kind of an instruction, for step over and step out
enum InstrFlow {
    IFL_NORMAL,
    IFL_CALL,       // call-like: step over runs until the next instruction
    IFL_RETURN      // return (possibly conditional)
};


struct DisasmInstr {
    int length;
    std::string text;
    InstrFlow flow;
};


// Debugger view of a CPU: registers, PC/SP, its address space, disassembler and
// supported breakpoint types. Front-ends use it to show any supported CPU with the
// same code. Each target has an id (0 for the platform CPU) to allow several CPUs
// per platform later.
class CpuDebugTarget
{
public:
    CpuDebugTarget(Cpu* cpu, int id) : m_cpu(cpu), m_id(id) {}
    virtual ~CpuDebugTarget() {}

    // Creates the target for a CPU; nullptr if the CPU is not supported by the debugger
    static CpuDebugTarget* create(Cpu* cpu, int id = 0);

    int getId() {return m_id;}
    Cpu* getCpu() {return m_cpu;}
    virtual const char* getCpuName() = 0; // "8080", "z80"

    // Registers
    virtual int getRegCount() = 0;
    virtual const DebugRegInfo& getRegInfo(int index) = 0;
    virtual unsigned getReg(int index) = 0;
    virtual void setReg(int index, unsigned value) = 0;
    int findReg(const std::string& id); // -1 if not found

    virtual uint16_t getPC() = 0;
    virtual void setPC(uint16_t value) = 0;
    virtual uint16_t getSP() = 0;

    // CPU address space
    uint8_t readMem(uint16_t addr);
    void writeMem(uint16_t addr, uint8_t value);

    // Disassembler. A target may support several syntaxes (e.g. 8080 code in Z80
    // mnemonics); syntax 0 is the native one.
    virtual int getDisasmSyntaxCount() {return 1;}
    virtual const char* getDisasmSyntaxName(int syntax) = 0;
    virtual void disassemble(uint16_t addr, DisasmInstr& instr, int syntax = 0) = 0;
    int getInstrLength(uint16_t addr);
    // Fast check without disassembling, used after every instruction in step out
    virtual bool isReturn(uint16_t addr) = 0;

    virtual bool supportsBreakpoint(BreakpointType type);

protected:
    Cpu* m_cpu;
    int m_id;
};


// 8080 and Z80
class Cpu8080DebugTarget : public CpuDebugTarget
{
public:
    Cpu8080DebugTarget(Cpu8080Compatible* cpu, int id);

    const char* getCpuName() override;

    int getRegCount() override;
    const DebugRegInfo& getRegInfo(int index) override;
    unsigned getReg(int index) override;
    void setReg(int index, unsigned value) override;

    uint16_t getPC() override;
    void setPC(uint16_t value) override;
    uint16_t getSP() override;

    int getDisasmSyntaxCount() override {return m_z80 ? 1 : 2;}
    const char* getDisasmSyntaxName(int syntax) override;
    void disassemble(uint16_t addr, DisasmInstr& instr, int syntax = 0) override;
    bool isReturn(uint16_t addr) override;

private:
    Cpu8080Compatible* m_cpu8080;
    CpuZ80* m_z80;
};


#endif // DEBUGTARGET_H
