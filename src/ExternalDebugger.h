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

#include "Debugger.h"
#include "Breakpoints.h"

class Cpu8080Compatible;
class AddressableDevice;


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


class DebugSession;


// Debugger interface for external front-ends (MCP server, wasm).
// A thin wrapper over the platform's DebugSession. In wasm it is the debugger UI
// (client DC_UI) and gets startDebug() on stops; for MCP it is an external client
// (DC_EXTERNAL) which polls the state and is never notified.
class ExternalDebugger : public IDebugger
{
public:
    ExternalDebugger(Platform* platform, DebugClient client);
    virtual ~ExternalDebugger() {}

    virtual void initDbgWindow() override {}
    virtual void setCaption(std::string) override {}
    virtual void update() override {}
    virtual void draw() override {}

    virtual void sendCmd(DebugCommand) override {}
    virtual void startDebug() override;

    enum class Register {
        af, bc, de, hl, sp, pc, iff
    };

    void dbgBreak();
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
    bool isTimerRunning();
    bool isTimerFired();

private:
    DebugSession* m_session;
    DebugClient m_client;
    BreakpointManager* m_bps;
    Cpu8080Compatible* m_cpu;
    AddressableDevice* m_as;
};

#endif // WASM_DBG || MCP_SERVER

#endif // EXTERNALDEBUGGER_H
