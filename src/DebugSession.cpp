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
#include "Memory.h"

#include <algorithm>
#include <cctype>


const char* getStopReasonName(DebugStopReason reason)
{
    switch (reason) {
    case DSR_BREAK:      return "break";
    case DSR_BREAKPOINT: return "breakpoint";
    case DSR_STEP:       return "step";
    case DSR_TIMER:      return "timer";
    case DSR_HALT:       return "halt";
    case DSR_ILLEGAL:    return "illegal";
    default:             return "unknown";
    }
}


void DebugElapsedTimer::onElapse()
{
    m_session->onTimerBreak(m_client);
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

    bool tempBpHit = false;
    if (m_tempBp) {
        tempBpHit = m_tempBp->getHitCount() > 0;
        delete m_tempBp;
        m_tempBp = nullptr;
    }

    // Several reasons may coincide; the more specific one is reported
    DebugStopInfo info;
    CpuStopReason cpuReason = m_cpu->takeDebugStopReason();
    BreakpointDesc bp;
    int bpId = m_breakpoints.takeStopBreakpoint();
    if (bpId >= 0 && m_breakpoints.get(bpId, bp)) {
        info.reason = DSR_BREAKPOINT;
        info.bpId = bpId;
    } else if (cpuReason == CSR_HALT)
        info.reason = DSR_HALT;
    else if (cpuReason == CSR_ILLEGAL)
        info.reason = DSR_ILLEGAL;
    else if (cpuReason == CSR_STEP || tempBpHit)
        info.reason = DSR_STEP;
    else if (m_timerBreak)
        info.reason = DSR_TIMER;
    else if (m_breakRequested)
        info.reason = DSR_BREAK;

    if (info.reason == DSR_BREAKPOINT)
        info.client = bp.owner;
    else if (m_breakRequested && (info.reason == DSR_BREAK || info.reason == DSR_TIMER))
        info.client = m_breakClient;
    else
        info.client = m_runClient;

    m_stopInfo = info;
    m_breakRequested = false;
    m_timerBreak = false;

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


void DebugSession::onTimerBreak(DebugClient client)
{
    if (!m_isRunning)
        return;
    m_timerBreak = true;
    requestBreak(client);
}


void DebugSession::resume(DebugClient client)
{
    if (m_isRunning)
        return;
    m_isRunning = true;
    m_runClient = client;
    m_breakRequested = false;
    m_timerBreak = false;
    m_cpu->takeDebugStopReason(); // a stale request, e.g. HLT while stopped by a breakpoint
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


const std::vector<DebugMemory>& DebugSession::getMemories()
{
    if (!m_memories.empty())
        return m_memories;

    AddressableDevice* cpuAs = m_cpu->getAddrSpace();
    if (cpuAs)
        m_memories.push_back({"cpu", cpuAs, 0x10000});

    for (EmuObject* obj : m_platform->getObjects()) {
        AddressableDevice* dev = dynamic_cast<AddressableDevice*>(obj);
        if (!dev || dev == cpuAs)
            continue;

        int size = dev->getDebugSize();
        if (!size) {
            if (Ram* ram = dynamic_cast<Ram*>(dev))
                size = ram->getSize();
            else if (Rom* rom = dynamic_cast<Rom*>(dev))
                size = rom->getSize();
            else if (!dev->getDebugName().empty())
                size = 0x10000;
        }
        if (size <= 0)
            continue;

        std::string name = dev->getDebugName().empty() ? dev->getName() : dev->getDebugName();
        if (name.empty())
            continue;
        m_memories.push_back({name, dev, unsigned(size)});
    }

    return m_memories;
}


std::vector<DebugDeviceInfo> DebugSession::getDevicesInfo()
{
    std::vector<DebugDeviceInfo> res;
    for (EmuObject* obj : m_platform->getObjects()) {
        std::string info = obj->getDebugInfo();
        if (!info.empty())
            res.push_back({obj->getName(), info});
    }
    return res;
}


int DebugSession::findMemory(const std::string& name)
{
    const std::vector<DebugMemory>& memories = getMemories();
    for (unsigned i = 0; i < memories.size(); i++)
        if (memories[i].name == name)
            return i;
    return -1;
}


uint8_t DebugSession::readMem(int memory, unsigned addr)
{
    const DebugMemory& mem = getMemories()[memory];
    return addr < mem.size ? debugReadByte(mem.device, addr) : 0xFF;
}


void DebugSession::writeMem(int memory, unsigned addr, uint8_t value)
{
    const DebugMemory& mem = getMemories()[memory];
    if (addr < mem.size) {
        mem.device->writeByte(addr, value);
        notifyModified();
    }
}


void DebugSession::setReg(int index, unsigned value)
{
    m_target->setReg(index, value);
    notifyModified();
}


void DebugSession::setPC(uint16_t value)
{
    m_target->setPC(value);
    notifyModified();
}


int DebugSession::getDefaultDisasmSyntax()
{
    // 8080 code in Z80 mnemonics
    if (g_emulation->getDebuggerOptions().forceZ80Mnemonics && m_target->getDisasmSyntaxCount() > 1)
        return 1;
    return 0;
}


void DebugSession::disassemble(uint16_t addr, DisasmInstr& instr, int syntax)
{
    if (syntax < 0)
        syntax = getDefaultDisasmSyntax();
    m_target->disassemble(addr, instr, syntax);

    // The disassemblers produce 8080 code in upper case and Z80 code in lower case
    const DebuggerOptions& opt = g_emulation->getDebuggerOptions();
    std::string syntaxName = m_target->getDisasmSyntaxName(syntax);
    if (syntaxName == "8080" && !opt.mnemo8080UpperCase)
        std::transform(instr.text.begin(), instr.text.end(), instr.text.begin(), ::tolower);
    else if (syntaxName == "z80" && opt.mnemoZ80UpperCase)
        std::transform(instr.text.begin(), instr.text.end(), instr.text.begin(), ::toupper);
}
