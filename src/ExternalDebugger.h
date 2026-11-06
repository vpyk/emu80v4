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

#ifndef EXTERNALDEBUGGER_H
#define EXTERNALDEBUGGER_H

#if defined(WASM_DBG) || defined(MCP_SERVER)

#include <list>
#include <string>

#include "EmuObjects.h"
#include "Debugger.h"
#include "Breakpoints.h"

class Cpu;
class Cpu8080Compatible;
class CpuZ80;


struct DbgExecBreakpoint {
    uint16_t addr;
    int hitCount;
    int skipCount;
    int remaining;
    std::string comment;
};

struct DbgDataBreakpoint {
    uint16_t addr;
    int type;   // BreakpointType: BT_WRITE=1, BT_READ=2, BT_ACSESS=3
    int hitCount;
    int skipCount;
    int remaining;
    std::string comment;
};

struct DbgCpuState {
    uint16_t af;
    uint16_t bc;
    uint16_t de;
    uint16_t hl;
    uint16_t sp;
    uint16_t pc;
    bool iff;
    uint8_t mem[0x10000];
    std::list<DbgExecBreakpoint> breakpoints;
    std::list<DbgDataBreakpoint> dataBreakpoints;
    std::list<DbgDataBreakpoint> portBreakpoints;
};


class DebugElapsedTimer : public ElapsedTimer
{
public:
    DebugElapsedTimer(Cpu* cpu) : m_cpu(cpu) {}
    void onElapse() override;
private:
    Cpu* m_cpu;
};


class ExternalDebugger : public IDebugger
{
public:
    ExternalDebugger(Platform* platform);
    virtual ~ExternalDebugger();

    virtual void initDbgWindow() override {}
    virtual void setCaption(std::string) override {}
    virtual void update() override {}
    virtual void draw() override {}

    virtual void sendCmd(DebugCommand cmd) override {}
    virtual void startDebug() override {dbgPause();}

    enum class Register {
        af, bc, de, hl, sp, pc, iff
    };

    void dbgPause();
    void dbgRun();
    void dbgRunFor(unsigned ms);
    void dbgStepIn();
    void dbgStepOver();
    void dbgStepOut() {}
    void dbgSetBreakpoints(const std::list<uint16_t>& breakpoints);
    void dbgDelBreakpoints(const std::list<uint16_t>& breakpoints);
    void dbgSetDataBreakpoint(uint16_t addr, BreakpointType type);
    void dbgDelDataBreakpoint(uint16_t addr, BreakpointType type);
    void dbgClearDataBreakpoints();
    void dbgSetPortBreakpoint(uint16_t addr, BreakpointType type);
    void dbgDelPortBreakpoint(uint16_t addr, BreakpointType type);
    void dbgClearPortBreakpoints();
    void dbgSetExecSkipCount(uint16_t addr, int skipCount);
    void dbgSetDataSkipCount(uint16_t addr, BreakpointType type, int skipCount);
    void dbgSetPortSkipCount(uint16_t addr, BreakpointType type, int skipCount);
    void dbgSetExecComment(uint16_t addr, const std::string& comment);
    void dbgSetDataComment(uint16_t addr, BreakpointType type, const std::string& comment);
    void dbgSetPortComment(uint16_t addr, BreakpointType type, const std::string& comment);
    void dbgSetRegister(ExternalDebugger::Register reg, uint16_t value);
    void dbgWriteByte(uint16_t addr, uint8_t value);
    void dbgGetState(DbgCpuState& state);
    void dbgCleanupTimer();
    bool isTimerRunning() { return m_runForTimer != nullptr; }
    bool isTimerFired()   { return m_runForTimer && m_runForTimer->isPaused(); }

    //static IDebugger* create(Platform* platform, bool oldSchool = true);

private:
    std::list<BreakpointInfo> m_bpList;
    std::list<DataBreakpoint*> m_dataBpList;
    std::list<DataBreakpoint*> m_portBpList;
    DebugElapsedTimer* m_runForTimer = nullptr;
    CodeBreakpoint* m_tempBp = nullptr;
    AddressableDevice* m_as = nullptr;
    Cpu8080Compatible* m_cpu = nullptr;
    CpuZ80* m_z80cpu = nullptr;
    Platform* m_platform = nullptr;

    bool m_z80Mode = false;
    bool m_isRunning = true;

    bool m_resetKeys;

    inline uint8_t memByte(uint16_t addr) {return m_as->readByte(addr);}
    int getInstructionLength(uint16_t addr);
    bool getInstructionOverFlag(uint16_t addr);

    void checkForCurBreakpoint();
    void addBreakpoint(uint16_t addr);
    void deleteBreakpoint(uint16_t addr);
};

#endif // WASM_DBG || MCP_SERVER

#endif // EXTERNALDEBUGGER_H
