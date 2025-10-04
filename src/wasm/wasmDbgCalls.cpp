/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2025
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

#include <cstdint>

#include "wasmDbgCalls.h"
#include "wasmEmuCalls.h"

#include "../Debugger.h"
#include "../EmuCalls.h"
#include "../DbgCalls.h"

using namespace std;

static ExternalDebugger* debugger = nullptr;


void wasmDbgPause()
{
    PalWindow* window = (PalWindow*)wasmGetWindowId();
    emuSysReq(window, SR_DEBUG);
}


void wasmDbgRun()
{
    if (!debugger)
        return;

    debugger->dbgRun();
}


void wasmDbgStepIn()
{
    if (!debugger)
        return;

    debugger->dbgStepIn();
}


void wasmDbgStepOver()
{
    if (!debugger)
        return;

    debugger->dbgStepOver();
}

void wasmDbgStepOut()
{
    if (!debugger)
        return;

    debugger->dbgStepOut();
}


void wasmDbgSetBreakpoints(int* bps, int size)
{
    if (!debugger) {
        PalWindow* window = (PalWindow*)wasmGetWindowId();
        emuCreateDebugger(window);
        debugger = static_cast<ExternalDebugger*>(emuGetDebugger(window));
    }

    list<uint16_t> breakpoints;
    for (int i = 0; i < size; i++)
       breakpoints.push_back(*bps++);

    debugger->dbgSetBreakpoints(breakpoints);
}


void wasmDbgDelBreakpoints(int* bps, int size)
{
    if (!debugger)
        return;

    list<uint16_t> breakpoints;
    for (int i = 0; i < size; i++)
       breakpoints.push_back(*bps++);

    debugger->dbgDelBreakpoints(breakpoints);
}

void wasmDbgSetRegister(int regId, int value)
{
    if (!debugger)
        return;

    auto reg = static_cast<ExternalDebugger::Register>(regId);
    debugger->dbgSetRegister(reg, value);
}


void wasmDbgWriteByte(int addr, int value)
{
    if (!debugger)
        return;

    debugger->dbgWriteByte(addr, value);
}


static uint16_t* breakpoints = nullptr;
static uint8_t* mem = nullptr;

static uint16_t regs[7]; // af, bc, de, hl, sp, pc, iff
static void* state[3] = {regs, nullptr, nullptr};


void* wasmDbgGetState()
{
    if (!debugger)
        return nullptr;

    DbgCpuState* dbgState = new DbgCpuState; // don't allocate on stack for its limited size while using wasm
    debugger->dbgGetState(*dbgState);

    regs[0] = dbgState->af;
    regs[1] = dbgState->bc;
    regs[2] = dbgState->de;
    regs[3] = dbgState->hl;
    regs[4] = dbgState->sp;
    regs[5] = dbgState->pc;
    regs[6] = dbgState->iff;

    int nBps = dbgState->breakpoints.size();
    breakpoints = static_cast<uint16_t*>(malloc((nBps + 1) * sizeof(uint16_t)));
    breakpoints[0] = nBps;
    std::copy(dbgState->breakpoints.begin(), dbgState->breakpoints.end(), breakpoints + 1);
    state[1] = breakpoints;

    mem = static_cast<uint8_t*>(malloc(0x10000));
    memcpy(mem, dbgState->mem, 0x10000);
    state[2] = mem;

    delete dbgState;

    return state; // mem and breakpoints must be freed by calling wasmDbgFreeState()
}


void wasmDbgFreeState()
{
    if (breakpoints)
        free(breakpoints);

    if (mem)
        free(mem);

    breakpoints = nullptr;
    mem = nullptr;
}


EM_JS(void, jsDebugRequest, (), {
    if (typeof window.top.debugRequest === "function")
        window.top.debugRequest();
});


void palDebugRequest()
{
    if (!debugger) {
        PalWindow* window = (PalWindow*)wasmGetWindowId();
        debugger = static_cast<ExternalDebugger*>(emuGetDebugger(window));
    }

    jsDebugRequest();
}
