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

#if defined(WASM_DBG) || defined(MCP_SERVER)

#include "Globals.h"
#include "ExternalDebugger.h"
#include "DebugSession.h"
#include "Platform.h"
#include "Pal.h"
#include "Cpu.h"

using namespace std;


//##### External Debugger class ####

ExternalDebugger::ExternalDebugger(Platform* platform) : IDebugger(platform)
{
    m_session = platform->getDebugSession();
    m_bps = m_session->getBreakpoints();
    m_cpu = m_session->getCpu();
    m_as = m_cpu->getAddrSpace();
}


void ExternalDebugger::startDebug()
{
    if (m_session->onStop())
        palDebugRequest();
}


void ExternalDebugger::dbgRun()
{
    m_session->run();
}


void ExternalDebugger::dbgRunFor(unsigned ms)
{
    m_session->runFor(ms);
}


void ExternalDebugger::dbgCleanupTimer()
{
    m_session->cleanupTimer();
}


bool ExternalDebugger::isTimerRunning()
{
    return m_session->isTimerRunning();
}


bool ExternalDebugger::isTimerFired()
{
    return m_session->isTimerFired();
}


void ExternalDebugger::dbgStepIn()
{
    m_session->stepInto();
}


void ExternalDebugger::dbgStepOver()
{
    m_session->stepOver();
}


void ExternalDebugger::dbgSetBreakpoints(const std::list<uint16_t> &breakpoints)
{
    for (auto& addr: breakpoints)
        m_bps->add(BT_EXEC, addr);
}


void ExternalDebugger::dbgDelBreakpoints(const std::list<uint16_t> &breakpoints)
{
    for (auto& addr: breakpoints)
        m_bps->remove(m_bps->find(BT_EXEC, addr));
}


void ExternalDebugger::dbgSetRegister(Register reg, uint16_t value)
{
    switch (reg) {
    case Register::af:
        m_cpu->setAF(value);
        break;
    case Register::bc:
        m_cpu->setBC(value);
        break;
    case Register::de:
        m_cpu->setDE(value);
        break;
    case Register::hl:
        m_cpu->setHL(value);
        break;
    case Register::sp:
        m_cpu->setSP(value);
        break;
    case Register::pc:
        m_cpu->setPC(value);
        break;
    case Register::iff:
        m_cpu->setIFF(value & 1);
        break;
    }
}


void ExternalDebugger::dbgWriteByte(uint16_t addr, uint8_t value)
{
    m_as->writeByte(addr, value);
}


void ExternalDebugger::dbgGetState(DbgCpuState& state)
{
    state.af = m_cpu->getAF();
    state.bc = m_cpu->getBC();
    state.de = m_cpu->getDE();
    state.hl = m_cpu->getHL();
    state.sp = m_cpu->getSP();
    state.pc = m_cpu->getPC();
    state.iff = m_cpu->getInte();
    for (int i = 0; i <= 0xffff; i++)
        state.mem[i] = m_as->readByte(i);

    vector<BreakpointDesc> bps;
    m_bps->list(bps);
    for (auto& bp: bps) {
        switch (getBreakpointClass(bp.type)) {
        case BC_EXEC:
            state.breakpoints.push_back({bp.addr, bp.hitCount, bp.skipCount, bp.remaining, bp.comment});
            break;
        case BC_MEM:
            state.dataBreakpoints.push_back({bp.addr, static_cast<int>(bp.type),
                bp.hitCount, bp.skipCount, bp.remaining, bp.comment});
            break;
        case BC_IO:
            state.portBreakpoints.push_back({bp.addr, static_cast<int>(bp.type),
                bp.hitCount, bp.skipCount, bp.remaining, bp.comment});
            break;
        }
    }
}


void ExternalDebugger::dbgSetDataBreakpoint(uint16_t addr, BreakpointType type)
{
    m_bps->add(type, addr);
}


void ExternalDebugger::dbgDelDataBreakpoint(uint16_t addr, BreakpointType type)
{
    m_bps->remove(m_bps->find(type, addr));
}


void ExternalDebugger::dbgClearDataBreakpoints()
{
    m_bps->clear(BC_MEM);
}


void ExternalDebugger::dbgSetExecSkipCount(uint16_t addr, int skipCount)
{
    m_bps->setSkipCount(m_bps->find(BT_EXEC, addr), skipCount);
}


void ExternalDebugger::dbgSetDataSkipCount(uint16_t addr, BreakpointType type, int skipCount)
{
    m_bps->setSkipCount(m_bps->find(type, addr), skipCount);
}


void ExternalDebugger::dbgSetExecComment(uint16_t addr, const std::string& comment)
{
    m_bps->setComment(m_bps->find(BT_EXEC, addr), comment);
}


void ExternalDebugger::dbgSetDataComment(uint16_t addr, BreakpointType type, const std::string& comment)
{
    m_bps->setComment(m_bps->find(type, addr), comment);
}


void ExternalDebugger::dbgSetPortBreakpoint(uint16_t addr, BreakpointType type)
{
    m_bps->add(type, addr);
}


void ExternalDebugger::dbgDelPortBreakpoint(uint16_t addr, BreakpointType type)
{
    m_bps->remove(m_bps->find(type, addr));
}


void ExternalDebugger::dbgClearPortBreakpoints()
{
    m_bps->clear(BC_IO);
}


void ExternalDebugger::dbgSetPortSkipCount(uint16_t addr, BreakpointType type, int skipCount)
{
    m_bps->setSkipCount(m_bps->find(type, addr), skipCount);
}


void ExternalDebugger::dbgSetPortComment(uint16_t addr, BreakpointType type, const std::string& comment)
{
    m_bps->setComment(m_bps->find(type, addr), comment);
}

#endif // WASM_DBG || MCP_SERVER
