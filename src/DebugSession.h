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
class Cpu;
class Cpu8080Compatible;


class DebugElapsedTimer : public ElapsedTimer
{
public:
    DebugElapsedTimer(Cpu* cpu) : m_cpu(cpu) {}
    void onElapse() override;
private:
    Cpu* m_cpu;
};


// Debugger core of a platform: run control and breakpoints. It doesn't depend on
// any UI; debugger front-ends (debug window, MCP server, wasm) use it.
// Must be used from the main (emulation) thread only.
class DebugSession
{
public:
    DebugSession(Platform* platform, Cpu8080Compatible* cpu);
    ~DebugSession();

    Platform* getPlatform() {return m_platform;}
    Cpu8080Compatible* getCpu() {return m_cpu;}
    BreakpointManager* getBreakpoints() {return &m_breakpoints;}

    bool isRunning() {return m_isRunning;}

    // Run control
    bool onStop(); // the emulation has stopped in the debugger; returns false if already stopped
    void run();
    void runFor(unsigned ms);
    void stepInto();
    void stepOver();

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

    void resume();
};


#endif // DEBUGSESSION_H
