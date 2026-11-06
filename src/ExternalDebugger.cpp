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
#include "Emulation.h"
#include "Platform.h"
#include "Pal.h"

#include "CpuZ80.h"
#include "Cpu8080dasm.h"
#include "CpuZ80dasm.h"

using namespace std;


//##### External Debugger class ####

ExternalDebugger::ExternalDebugger(Platform* platform) : IDebugger(platform)
{
    m_platform = platform;

    m_cpu = static_cast<Cpu8080Compatible*>(m_platform->getCpu());
    m_as = m_cpu->getAddrSpace();

    m_z80cpu = dynamic_cast<CpuZ80*>(m_cpu);
    m_z80Mode = m_z80cpu != nullptr;

    m_resetKeys = g_emulation->getDebuggerOptions().resetKeys;

    m_isRunning = true;
}


ExternalDebugger::~ExternalDebugger()
{
    for (auto* dbp : m_dataBpList)
        delete dbp;
    m_dataBpList.clear();

    for (auto* dbp : m_portBpList)
        delete dbp;
    m_portBpList.clear();

    dbgCleanupTimer();
}


int ExternalDebugger::getInstructionLength(uint16_t addr)
{
    uint8_t buf[4];
    buf[0] = memByte(addr);
    buf[1] = memByte(addr + 1); // Z80
    buf[2] = memByte(addr + 2); // Z80
    buf[3] = memByte(addr + 3); // Z80

    if (!m_z80Mode) {
        return i8080GetInstructionLength(buf);
    }

    unsigned length;
    STEP_FLAG flag;
    cpu_disassemble_z80(addr, buf, length, flag);
    return length;
}


bool ExternalDebugger::getInstructionOverFlag(uint16_t addr)
{
    if (!m_z80Mode) {
        uint8_t op = memByte(addr);
        return (op & 0xcf) == 0xcd /* CALL */ || (op & 0xc7) == 0xc7 /* RST */ || (op & 0xc7) == 0xc4 /* Cx */;
    }

    uint8_t buf[4];
    buf[0] = memByte(addr);
    buf[1] = memByte(addr + 1); // Z80
    buf[2] = memByte(addr + 2); // Z80
    buf[3] = memByte(addr + 3); // Z80

    unsigned length;
    STEP_FLAG flag;
    cpu_disassemble_z80(addr, buf, length, flag);
    return (flag == SF_OVER);
}


void ExternalDebugger::dbgPause()
{
    if (!m_isRunning)
        return;

    m_isRunning = false;

    if (m_tempBp) {
        delete m_tempBp;
        m_tempBp = nullptr;
    }

    palDebugRequest();
}


void ExternalDebugger::dbgRun()
{
    m_isRunning = true;

    checkForCurBreakpoint();
    g_emulation->debugRun();
}


void ExternalDebugger::dbgRunFor(unsigned ms)
{
    if (m_runForTimer)
        dbgCleanupTimer();
    m_runForTimer = new DebugElapsedTimer(m_cpu);
    m_runForTimer->start(ms);
    dbgRun();
}


void ExternalDebugger::dbgCleanupTimer()
{
    if (!m_runForTimer)
        return;
    m_runForTimer->stop();
    delete m_runForTimer;
    m_runForTimer = nullptr;
}


void ExternalDebugger::dbgStepIn()
{
    m_cpu->debugStepRequest();
    m_isRunning = true;
    checkForCurBreakpoint();
    g_emulation->debugRun();
}


void ExternalDebugger::dbgStepOver()
{
    unsigned pc = m_cpu->getPC();
    unsigned len = getInstructionLength(pc);
    bool over = getInstructionOverFlag(pc);

    if (over) {
        if (m_tempBp)
            return;
        m_tempBp = new CodeBreakpoint(pc + len);
        m_cpu->addHook(m_tempBp);

        m_isRunning = true;
        checkForCurBreakpoint();
        if (m_resetKeys)
            m_platform->resetKeys();
        g_emulation->debugRun();
    } else
        dbgStepIn();
}


void ExternalDebugger::checkForCurBreakpoint()
{
    uint16_t pc = m_cpu->getPC();
    for (auto it = m_bpList.begin(); it != m_bpList.end(); it++)
        if ((*it).type == BT_EXEC && (*it).addr == pc && (*it).codeBp)
            if ((*it).codeBp->getRemaining() == 0)
                (*it).codeBp->setRemaining(1);
}


void ExternalDebugger::addBreakpoint(uint16_t addr)
{
    // ищем точку останова
    bool found = false;
    BreakpointInfo bpInfo;
    list<BreakpointInfo>::iterator it;
    for (it = m_bpList.begin(); it != m_bpList.end(); it++)
        if ((*it).addr == addr) {
            found = true;
            bpInfo = *it;
            break;
        }

    if (!found) {
        CodeBreakpoint* cbp = new CodeBreakpoint(addr);
        m_cpu->addHook(cbp);
        bpInfo.addr = addr;
        bpInfo.type = BT_EXEC;
        bpInfo.codeBp = cbp;
        m_bpList.push_back(bpInfo);
    }
}


void ExternalDebugger::deleteBreakpoint(uint16_t addr)
{
    // ищем точку останова
    bool found = false;
    BreakpointInfo bpInfo;
    list<BreakpointInfo>::iterator it;
    for (it = m_bpList.begin(); it != m_bpList.end(); it++)
        if ((*it).addr == addr) {
            found = true;
            bpInfo = *it;
            break;
        }

    if (found) {
        // нашли, удаляем
        delete bpInfo.codeBp;
        m_bpList.erase(it);
    }
}


void ExternalDebugger::dbgSetBreakpoints(const std::list<uint16_t> &breakpoints)
{
    for (auto& addr: breakpoints)
        addBreakpoint(addr);
}


void ExternalDebugger::dbgDelBreakpoints(const std::list<uint16_t> &breakpoints)
{
    for (auto& addr: breakpoints)
        deleteBreakpoint(addr);
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
        state.mem[i] = memByte(i);

    for (auto& bp: m_bpList)
        state.breakpoints.push_back({bp.addr,
            bp.codeBp ? bp.codeBp->getHitCount()  : 0,
            bp.codeBp ? bp.codeBp->getSkipCount() : 0,
            bp.codeBp ? bp.codeBp->getRemaining() : 0,
            bp.codeBp ? bp.codeBp->getComment()   : ""});

    for (auto* dbp: m_dataBpList)
        state.dataBreakpoints.push_back({dbp->getAddr(), static_cast<int>(dbp->getType()),
            dbp->getHitCount(), dbp->getSkipCount(), dbp->getRemaining(),
            dbp->getComment()});

    for (auto* dbp: m_portBpList)
        state.portBreakpoints.push_back({dbp->getAddr(), static_cast<int>(dbp->getType()),
            dbp->getHitCount(), dbp->getSkipCount(), dbp->getRemaining(),
            dbp->getComment()});
}


void ExternalDebugger::dbgSetDataBreakpoint(uint16_t addr, BreakpointType type)
{
    // Replace an existing breakpoint of the same kind at this address;
    // breakpoints of other kinds stay.
    for (auto it = m_dataBpList.begin(); it != m_dataBpList.end(); ) {
        if ((*it)->getAddr() == addr && (*it)->getType() == type) {
            delete *it;
            it = m_dataBpList.erase(it);
        } else {
            ++it;
        }
    }

    DataBreakpoint* dbp = new DataBreakpoint(addr, type);
    if (m_cpu) {
        dbp->setCpu(dynamic_cast<Cpu*>(m_cpu));
        m_cpu->addDataBreakpoint(dbp);
    }
    m_dataBpList.push_back(dbp);
}


void ExternalDebugger::dbgDelDataBreakpoint(uint16_t addr, BreakpointType type)
{
    for (auto it = m_dataBpList.begin(); it != m_dataBpList.end(); ) {
        if ((*it)->getAddr() == addr && (*it)->getType() == type) {
            delete *it; // destructor calls removeDataBreakpoint on CPU
            it = m_dataBpList.erase(it);
        } else {
            ++it;
        }
    }
}


void DebugElapsedTimer::onElapse()
{
    g_emulation->debugRequest(m_cpu);
}


void ExternalDebugger::dbgClearDataBreakpoints()
{
    for (auto* dbp : m_dataBpList)
        delete dbp; // destructor calls removeDataBreakpoint on CPU
    m_dataBpList.clear();
}

void ExternalDebugger::dbgSetExecSkipCount(uint16_t addr, int skipCount)
{
    for (auto& bp : m_bpList)
        if (bp.addr == addr && bp.codeBp)
            bp.codeBp->setSkipCount(skipCount);
}

void ExternalDebugger::dbgSetDataSkipCount(uint16_t addr, BreakpointType type, int skipCount)
{
    for (auto* dbp : m_dataBpList)
        if (dbp->getAddr() == addr && dbp->getType() == type)
            dbp->setSkipCount(skipCount);
}

void ExternalDebugger::dbgSetExecComment(uint16_t addr, const std::string& comment)
{
    for (auto& bp : m_bpList)
        if (bp.addr == addr && bp.codeBp)
            bp.codeBp->setComment(comment);
}

void ExternalDebugger::dbgSetDataComment(uint16_t addr, BreakpointType type, const std::string& comment)
{
    for (auto* dbp : m_dataBpList)
        if (dbp->getAddr() == addr && dbp->getType() == type)
            dbp->setComment(comment);
}

void ExternalDebugger::dbgSetPortBreakpoint(uint16_t addr, BreakpointType type)
{
    // Replace an existing breakpoint of the same kind at this port
    for (auto it = m_portBpList.begin(); it != m_portBpList.end(); ) {
        if ((*it)->getAddr() == addr && (*it)->getType() == type) {
            delete *it;
            it = m_portBpList.erase(it);
        } else {
            ++it;
        }
    }
    DataBreakpoint* dbp = new DataBreakpoint(addr, type);
    if (m_cpu) {
        dbp->setCpu(dynamic_cast<Cpu*>(m_cpu));
        m_cpu->addPortBreakpoint(dbp);
    }
    m_portBpList.push_back(dbp);
}

void ExternalDebugger::dbgDelPortBreakpoint(uint16_t addr, BreakpointType type)
{
    for (auto it = m_portBpList.begin(); it != m_portBpList.end(); ) {
        if ((*it)->getAddr() == addr && (*it)->getType() == type) {
            delete *it;
            it = m_portBpList.erase(it);
        } else {
            ++it;
        }
    }
}

void ExternalDebugger::dbgClearPortBreakpoints()
{
    for (auto* dbp : m_portBpList)
        delete dbp;
    m_portBpList.clear();
}

void ExternalDebugger::dbgSetPortSkipCount(uint16_t addr, BreakpointType type, int skipCount)
{
    for (auto* dbp : m_portBpList)
        if (dbp->getAddr() == addr && dbp->getType() == type)
            dbp->setSkipCount(skipCount);
}

void ExternalDebugger::dbgSetPortComment(uint16_t addr, BreakpointType type, const std::string& comment)
{
    for (auto* dbp : m_portBpList)
        if (dbp->getAddr() == addr && dbp->getType() == type)
            dbp->setComment(comment);
}

#endif // WASM_DBG || MCP_SERVER
