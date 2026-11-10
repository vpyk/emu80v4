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

#ifndef DEBUGSESSION_H
#define DEBUGSESSION_H

#include "EmuObjects.h"
#include "Cpu.h"
#include "Breakpoints.h"
#include "DebugTarget.h"

#include <vector>

class Platform;
class DebugSession;


// Memory shown in debugger dumps
struct DebugMemory {
    std::string name;
    AddressableDevice* device;
    unsigned size;
};


// Why the emulation has stopped in the debugger
enum DebugStopReason {
    DSR_NONE,       // not stopped yet, or the reason is unknown
    DSR_BREAK,      // a client has requested a break
    DSR_BREAKPOINT, // a user breakpoint has fired
    DSR_STEP,       // step into, step over, step out or run to is done
    DSR_TIMER,      // run for: the time has elapsed
    DSR_HALT,       // HLT (debugOnHalt)
    DSR_ILLEGAL     // illegal instruction (debugOnIllegalCmd)
};

const char* getStopReasonName(DebugStopReason reason); // "break", "breakpoint", "step"...


struct DebugStopInfo {
    DebugStopReason reason = DSR_NONE;
    int bpId = -1;              // for DSR_BREAKPOINT
    DebugClient client = DC_UI; // who the stop belongs to
};


// Stops the emulation in the debugger after the given time, on behalf of a client
class DebugElapsedTimer : public ElapsedTimer
{
public:
    DebugElapsedTimer(DebugSession* session, DebugClient client) : m_session(session), m_client(client) {}
    void onElapse() override;
private:
    DebugSession* m_session;
    DebugClient m_client;
};


// Debugger core of a platform: run control and breakpoints. It doesn't depend on
// any UI; debugger front-ends (debug window, MCP server, web debugger in wasm)
// use it directly.
// Must be used from the main (emulation) thread only.
//
// Run control methods take the client that issues the command. When the emulation
// stops, the stop is attributed to a client: the owner of the breakpoint that has
// fired, else the client that requested the break, else the client that resumed
// the emulation last (step completion, step over, run for, HLT etc.).
class DebugSession
{
public:
    // Takes ownership of the target
    DebugSession(Platform* platform, CpuDebugTarget* target);
    ~DebugSession();

    Platform* getPlatform() {return m_platform;}
    CpuDebugTarget* getTarget() {return m_target;}
    Cpu* getCpu() {return m_cpu;}
    BreakpointManager* getBreakpoints() {return &m_breakpoints;}

    bool isRunning() {return m_isRunning;}

    // Called when the emulation has stopped in the debugger; returns false if
    // the session is already stopped (it is called repeatedly while stopped)
    bool onStop();
    DebugClient getStopClient() {return m_stopInfo.client;} // who the last stop belongs to
    const DebugStopInfo& getStopInfo() {return m_stopInfo;} // the last stop
    unsigned getStopCount() {return m_stopCount;}      // number of stops so far

    // Run control. The commands that resume the emulation do nothing while it is
    // running (runFor only starts its timer then).
    void requestBreak(DebugClient client);
    void run(DebugClient client);
    void runFor(unsigned ms, DebugClient client);
    void stepInto(DebugClient client);
    // Steps over a call; returns true if the emulation runs until the return
    // address, false if it was an ordinary step (or nothing was done)
    bool stepOver(DebugClient client);
    // Runs until the current subroutine returns: stops after a return instruction
    // that has taken the stack pointer above its value at the start
    void stepOut(DebugClient client);
    void runTo(uint16_t addr, DebugClient client);

    // Called by the run-for timer
    void onTimerBreak(DebugClient client);

    // Changes of the machine state by a debugger client (registers, memory, ports).
    // Front-ends change the state through these methods or call notifyModified(),
    // so that other front-ends can see the change by getModifyCount().
    void setReg(int index, unsigned value);
    void setPC(uint16_t value);
    void notifyModified() {++m_modifyCount;}
    unsigned getModifyCount() {return m_modifyCount;}

    // Disassembles with the debugger options applied (letter case, Z80 mnemonics
    // for 8080 code). syntax -1 means the one chosen by the options.
    void disassemble(uint16_t addr, DisasmInstr& instr, int syntax = -1);
    int getDefaultDisasmSyntax();

    // Memories for dumps: the CPU address space ("cpu") first, then Ram/Rom objects
    // and devices with debugName set in the platform configuration. Built on first use.
    const std::vector<DebugMemory>& getMemories();
    int findMemory(const std::string& name); // -1 if not found
    uint8_t readMem(int memory, unsigned addr);
    void writeMem(int memory, unsigned addr, uint8_t value);

    // Run-for timer
    void cleanupTimer();
    bool isTimerRunning() {return m_runForTimer != nullptr;}
    bool isTimerFired()   {return m_runForTimer && m_runForTimer->isPaused();}

private:
    class StepOutFilter : public CpuStepFilter
    {
    public:
        void start(CpuDebugTarget* target);
        bool isStepDone() override;
    private:
        CpuDebugTarget* m_target = nullptr;
        uint16_t m_startSp = 0;
        uint16_t m_lastPc = 0;
    };

    Platform* m_platform;
    std::vector<DebugMemory> m_memories;
    CpuDebugTarget* m_target;
    Cpu* m_cpu;
    BreakpointManager m_breakpoints;
    StepOutFilter m_stepOutFilter;

    bool m_isRunning = true;
    bool m_resetKeys;
    CodeBreakpoint* m_tempBp = nullptr;
    DebugElapsedTimer* m_runForTimer = nullptr;

    DebugClient m_runClient = DC_UI;   // who has resumed the emulation last
    DebugClient m_breakClient = DC_UI; // who has requested a break
    bool m_breakRequested = false;
    bool m_timerBreak = false;
    DebugStopInfo m_stopInfo;
    unsigned m_stopCount = 0;
    unsigned m_modifyCount = 0;

    void resume(DebugClient client);
};


#endif // DEBUGSESSION_H
