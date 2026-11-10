/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2025-2026
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
#include <cstdlib>
#include <cstring>
#include <vector>

#include "wasmDbgCalls.h"
#include "wasmEmuCalls.h"

#include "../DebugSession.h"
#include "../Platform.h"
#include "../EmuCalls.h"
#include "../DbgCalls.h"

using namespace std;


// The web debugger is the debugger UI of the wasm build (client DC_UI). It works
// with the debug session of the current platform directly.
static DebugSession* getSession()
{
    Platform* platform = emuGetPlatform((PalWindow*)wasmGetWindowId());
    return platform ? platform->getDebugSession() : nullptr;
}


// Register ids of the web debugger's register numbers
static const char* const c_regIds[] = {"af", "bc", "de", "hl", "sp", "pc", "iff"};


// Debug session stopped in the debugger, nullptr if running or not available
static DebugSession* getStoppedSession()
{
    DebugSession* session = getSession();
    return session && !session->isRunning() ? session : nullptr;
}


void wasmDbgPause()
{
    PalWindow* window = (PalWindow*)wasmGetWindowId();
    emuSysReq(window, SR_DEBUG);
}


void wasmDbgRun()
{
    if (DebugSession* session = getStoppedSession())
        session->run(DC_UI);
}


void wasmDbgStepIn()
{
    if (DebugSession* session = getStoppedSession())
        session->stepInto(DC_UI);
}


void wasmDbgStepOver()
{
    if (DebugSession* session = getStoppedSession())
        session->stepOver(DC_UI);
}

void wasmDbgStepOut()
{
    // not implemented yet
}


void wasmDbgSetBreakpoints(int* bps, int size)
{
    DebugSession* session = getSession();
    if (!session)
        return;

    for (int i = 0; i < size; i++)
        session->getBreakpoints()->add(BT_EXEC, bps[i], DC_UI);
}


void wasmDbgDelBreakpoints(int* bps, int size)
{
    DebugSession* session = getSession();
    if (!session)
        return;

    BreakpointManager* bpm = session->getBreakpoints();
    for (int i = 0; i < size; i++)
        bpm->remove(bpm->find(BT_EXEC, bps[i]));
}

void wasmDbgSetRegister(int regId, int value)
{
    DebugSession* session = getSession();
    if (!session)
        return;

    if (regId < 0 || regId >= 7)
        return;
    CpuDebugTarget* target = session->getTarget();
    int index = target->findReg(c_regIds[regId]);
    if (index >= 0)
        session->setReg(index, value);
}


void wasmDbgWriteByte(int addr, int value)
{
    DebugSession* session = getSession();
    if (!session)
        return;

    session->writeMem(0, addr & 0xFFFF, value); // memory 0 is the CPU address space
}


static uint16_t* breakpoints = nullptr;
static uint8_t* mem = nullptr;

static uint16_t regs[7]; // af, bc, de, hl, sp, pc, iff
static void* state[3] = {regs, nullptr, nullptr};


void* wasmDbgGetState()
{
    DebugSession* session = getSession();
    if (!session)
        return nullptr;

    wasmDbgFreeState();

    CpuDebugTarget* target = session->getTarget();
    for (int i = 0; i < 7; i++) {
        int index = target->findReg(c_regIds[i]);
        regs[i] = index >= 0 ? target->getReg(index) : 0;
    }

    // the web debugger shows exec breakpoints only
    vector<BreakpointDesc> bps;
    session->getBreakpoints()->list(bps);
    vector<uint16_t> execBps;
    for (auto& bp : bps)
        if (bp.type == BT_EXEC)
            execBps.push_back(bp.addr);

    int nBps = execBps.size();
    breakpoints = static_cast<uint16_t*>(malloc((nBps + 1) * sizeof(uint16_t)));
    breakpoints[0] = nBps;
    for (int i = 0; i < nBps; i++)
        breakpoints[i + 1] = execBps[i];
    state[1] = breakpoints;

    mem = static_cast<uint8_t*>(malloc(0x10000));
    for (int i = 0; i <= 0xffff; i++)
        mem[i] = target->readMem(i);
    state[2] = mem;

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


// Called on stops of the web debugger (DC_UI), see Platform::createDebugger()
void palDebugRequest()
{
    jsDebugRequest();
}
