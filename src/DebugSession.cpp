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

#include "Globals.h"
#include "DebugSession.h"
#include "Emulation.h"
#include "Platform.h"

#include "CpuZ80.h"
#include "Cpu8080dasm.h"
#include "CpuZ80dasm.h"


void DebugElapsedTimer::onElapse()
{
    m_session->requestBreak(m_client);
}


DebugSession::DebugSession(Platform* platform, Cpu8080Compatible* cpu)
    : m_platform(platform), m_cpu(cpu), m_z80Mode(dynamic_cast<CpuZ80*>(cpu) != nullptr),
      m_breakpoints(cpu)
{
    m_resetKeys = g_emulation->getDebuggerOptions().resetKeys;
}


DebugSession::~DebugSession()
{
    cleanupTimer();
    delete m_tempBp;
}


bool DebugSession::onStop()
{
    if (!m_isRunning)
        return false;

    m_isRunning = false;
    ++m_stopCount;

    if (m_tempBp) {
        delete m_tempBp;
        m_tempBp = nullptr;
    }

    BreakpointDesc bp;
    int bpId = m_breakpoints.takeStopBreakpoint();
    if (bpId >= 0 && m_breakpoints.get(bpId, bp))
        m_stopClient = bp.owner;
    else if (m_breakRequested)
        m_stopClient = m_breakClient;
    else
        m_stopClient = m_runClient;
    m_breakRequested = false;

    return true;
}


void DebugSession::requestBreak(DebugClient client)
{
    if (!m_isRunning)
        return;
    if (!m_breakRequested) {
        m_breakRequested = true;
        m_breakClient = client;
    }
    g_emulation->debugRequest(m_cpu);
}


void DebugSession::resume(DebugClient client)
{
    m_isRunning = true;
    m_runClient = client;
    m_breakRequested = false;
    m_breakpoints.resetStopBreakpoint();
    m_breakpoints.skipCurrentPc(m_cpu->getPC());
    g_emulation->debugRun();
}


void DebugSession::run(DebugClient client)
{
    resume(client);
}


void DebugSession::runFor(unsigned ms, DebugClient client)
{
    if (m_runForTimer)
        cleanupTimer();
    m_runForTimer = new DebugElapsedTimer(this, client);
    m_runForTimer->start(ms);
    run(client);
}


void DebugSession::cleanupTimer()
{
    if (!m_runForTimer)
        return;
    m_runForTimer->stop();
    delete m_runForTimer;
    m_runForTimer = nullptr;
}


void DebugSession::stepInto(DebugClient client)
{
    m_cpu->debugStepRequest();
    resume(client);
}


bool DebugSession::stepOver(DebugClient client)
{
    unsigned pc = m_cpu->getPC();
    unsigned len = getInstructionLength(pc);
    bool over = getInstructionOverFlag(pc);

    if (over) {
        // previous step over is not completed yet
        if (m_tempBp)
            return false;
        m_tempBp = new CodeBreakpoint(pc + len);
        m_cpu->addHook(m_tempBp);

        if (m_resetKeys)
            m_platform->resetKeys();
        resume(client);
        return true;
    }

    stepInto(client);
    return false;
}


void DebugSession::runTo(uint16_t addr, DebugClient client)
{
    // previous step over or run to is not completed yet
    if (m_tempBp)
        return;
    m_tempBp = new CodeBreakpoint(addr);
    m_cpu->addHook(m_tempBp);

    // running to the current address means a full loop
    if (addr == m_cpu->getPC())
        m_tempBp->setSkipCount(1);

    resume(client);
}


int DebugSession::getInstructionLength(uint16_t addr)
{
    AddressableDevice* as = m_cpu->getAddrSpace();
    uint8_t buf[4];
    buf[0] = as->readByte(addr);
    buf[1] = as->readByte(uint16_t(addr + 1)); // Z80
    buf[2] = as->readByte(uint16_t(addr + 2)); // Z80
    buf[3] = as->readByte(uint16_t(addr + 3)); // Z80

    if (!m_z80Mode)
        return i8080GetInstructionLength(buf);

    unsigned length;
    STEP_FLAG flag;
    cpu_disassemble_z80(addr, buf, length, flag);
    return length;
}


bool DebugSession::getInstructionOverFlag(uint16_t addr)
{
    AddressableDevice* as = m_cpu->getAddrSpace();

    if (!m_z80Mode) {
        uint8_t op = as->readByte(addr);
        return (op & 0xcf) == 0xcd /* CALL */ || (op & 0xc7) == 0xc7 /* RST */ || (op & 0xc7) == 0xc4 /* Cx */;
    }

    uint8_t buf[4];
    buf[0] = as->readByte(addr);
    buf[1] = as->readByte(uint16_t(addr + 1)); // Z80
    buf[2] = as->readByte(uint16_t(addr + 2)); // Z80
    buf[3] = as->readByte(uint16_t(addr + 3)); // Z80

    unsigned length;
    STEP_FLAG flag;
    cpu_disassemble_z80(addr, buf, length, flag);
    return (flag == SF_OVER);
}
