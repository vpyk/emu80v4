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
#include "Breakpoints.h"

class Platform;
class Cpu8080Compatible;
class DebugSession;


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
    DebugSession(Platform* platform, Cpu8080Compatible* cpu);
    ~DebugSession();

    Platform* getPlatform() {return m_platform;}
    Cpu8080Compatible* getCpu() {return m_cpu;}
    BreakpointManager* getBreakpoints() {return &m_breakpoints;}

    bool isRunning() {return m_isRunning;}

    // Called when the emulation has stopped in the debugger; returns false if
    // the session is already stopped (it is called repeatedly while stopped)
    bool onStop();
    DebugClient getStopClient() {return m_stopClient;} // who the last stop belongs to
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
    void runTo(uint16_t addr, DebugClient client);

    // Run-for timer
    void cleanupTimer();
    bool isTimerRunning() {return m_runForTimer != nullptr;}
    bool isTimerFired()   {return m_runForTimer && m_runForTimer->isPaused();}

    int getInstructionLength(uint16_t addr);
    bool getInstructionOverFlag(uint16_t addr);

private:
    Platform* m_platform;
    Cpu8080Compatible* m_cpu;
    bool m_z80Mode;
    BreakpointManager m_breakpoints;

    bool m_isRunning = true;
    bool m_resetKeys;
    CodeBreakpoint* m_tempBp = nullptr;
    DebugElapsedTimer* m_runForTimer = nullptr;

    DebugClient m_runClient = DC_UI;   // who has resumed the emulation last
    DebugClient m_breakClient = DC_UI; // who has requested a break
    bool m_breakRequested = false;
    DebugClient m_stopClient = DC_UI;
    unsigned m_stopCount = 0;

    void resume(DebugClient client);
};


#endif // DEBUGSESSION_H
