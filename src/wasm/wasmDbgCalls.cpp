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
#include "../Cpu.h"
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

    Cpu8080Compatible* cpu = session->getCpu();
    switch (regId) { // af, bc, de, hl, sp, pc, iff
    case 0:
        cpu->setAF(value);
        break;
    case 1:
        cpu->setBC(value);
        break;
    case 2:
        cpu->setDE(value);
        break;
    case 3:
        cpu->setHL(value);
        break;
    case 4:
        cpu->setSP(value);
        break;
    case 5:
        cpu->setPC(value);
        break;
    case 6:
        cpu->setIFF(value & 1);
        break;
    }
}


void wasmDbgWriteByte(int addr, int value)
{
    DebugSession* session = getSession();
    if (!session)
        return;

    session->getCpu()->getAddrSpace()->writeByte(addr, value);
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

    Cpu8080Compatible* cpu = session->getCpu();
    regs[0] = cpu->getAF();
    regs[1] = cpu->getBC();
    regs[2] = cpu->getDE();
    regs[3] = cpu->getHL();
    regs[4] = cpu->getSP();
    regs[5] = cpu->getPC();
    regs[6] = cpu->getInte();

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

    AddressableDevice* as = cpu->getAddrSpace();
    mem = static_cast<uint8_t*>(malloc(0x10000));
    for (int i = 0; i <= 0xffff; i++)
        mem[i] = as->readByte(i);
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
