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


void DebugElapsedTimer::onElapse()
{
    m_session->requestBreak(m_client);
}


DebugSession::DebugSession(Platform* platform, CpuDebugTarget* target)
    : m_platform(platform), m_target(target), m_cpu(target->getCpu()),
      m_breakpoints(target->getCpu())
{
    m_resetKeys = g_emulation->getDebuggerOptions().resetKeys;
    m_breakpoints.setIoSupported(target->supportsBreakpoint(BT_PORT_ACSESS));
}


DebugSession::~DebugSession()
{
    m_cpu->cancelDebugStep();
    cleanupTimer();
    delete m_tempBp;
    delete m_target;
}


bool DebugSession::onStop()
{
    if (!m_isRunning)
        return false;

    m_isRunning = false;
    ++m_stopCount;
    m_cpu->cancelDebugStep(); // a step (out) may be interrupted by a breakpoint

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
    if (m_isRunning)
        return;
    m_isRunning = true;
    m_runClient = client;
    m_breakRequested = false;
    m_breakpoints.resetStopBreakpoint();
    // let execution leave the current address
    m_cpu->skipBreakpointsOnce(m_target->getPC());
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
    if (m_isRunning)
        return;
    m_cpu->debugStepRequest();
    resume(client);
}


bool DebugSession::stepOver(DebugClient client)
{
    if (m_isRunning)
        return false;
    uint16_t pc = m_target->getPC();
    DisasmInstr instr;
    m_target->disassemble(pc, instr);

    if (instr.flow == IFL_CALL) {
        // previous step over is not completed yet
        if (m_tempBp)
            return false;
        m_tempBp = new CodeBreakpoint(uint16_t(pc + instr.length));
        m_cpu->addHook(m_tempBp);

        if (m_resetKeys)
            m_platform->resetKeys();
        resume(client);
        return true;
    }

    stepInto(client);
    return false;
}


void DebugSession::stepOut(DebugClient client)
{
    if (m_isRunning)
        return;
    m_stepOutFilter.start(m_target);
    m_cpu->debugStepRequest(&m_stepOutFilter);
    if (m_resetKeys)
        m_platform->resetKeys();
    resume(client);
}


void DebugSession::StepOutFilter::start(CpuDebugTarget* target)
{
    m_target = target;
    m_startSp = target->getSP();
    m_lastPc = target->getPC();
}


bool DebugSession::StepOutFilter::isStepDone()
{
    // m_lastPc is the address of the instruction just executed
    bool ret = m_target->isReturn(m_lastPc);
    m_lastPc = m_target->getPC();
    if (!ret)
        return false;
    // returned above the start frame (not from a nested call or an interrupt)
    uint16_t diff = m_target->getSP() - m_startSp;
    return diff != 0 && diff < 0x8000;
}


void DebugSession::runTo(uint16_t addr, DebugClient client)
{
    // previous step over or run to is not completed yet
    if (m_isRunning || m_tempBp)
        return;
    m_tempBp = new CodeBreakpoint(addr);
    m_cpu->addHook(m_tempBp);

    // running to the current address means a full loop, as the breakpoints
    // at the current address are skipped on resume
    resume(client);
}
