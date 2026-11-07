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
#include "Breakpoints.h"
#include "Emulation.h"
#include "Cpu.h"

using namespace std;


BreakpointClass getBreakpointClass(BreakpointType type)
{
    switch (type) {
        case BT_EXEC:
            return BC_EXEC;
        case BT_PORT_WRITE:
        case BT_PORT_READ:
        case BT_PORT_ACSESS:
            return BC_IO;
        default:
            return BC_MEM;
    }
}


//##### Breakpoint counters ####

bool BreakpointCounters::countHit()
{
    ++m_hitCount;
    if (m_remaining > 0) {
        --m_remaining;
        return false;
    }
    // Actual stop: skip the next skipCount hits again
    m_remaining = m_skipCount;
    if (m_manager)
        m_manager->onBreakpointStop(m_id);
    return true;
}


//##### Code Breakpoint class ####

CodeBreakpoint::~CodeBreakpoint()
{
    if (m_cpu)
        m_cpu->removeHook(this);
}


bool CodeBreakpoint::hookProc()
{
    if (!countHit())
        return false;
    g_emulation->debugRequest(m_cpu);
    return true;
}



//##### Data Breakpoint class ####

DataBreakpoint::DataBreakpoint(uint16_t addr, BreakpointType type)
    : m_addr(addr), m_type(type)
{}

DataBreakpoint::~DataBreakpoint()
{
    if (m_cpu) {
        if (getBreakpointClass(m_type) == BC_IO)
            m_cpu->removePortBreakpoint(this);
        else
            m_cpu->removeDataBreakpoint(this);
    }
    m_cpu = nullptr;
}

bool DataBreakpoint::check(bool isWrite)
{
    // BT_ACSESS/BT_PORT_ACSESS fire on any access; BT_WRITE/BT_PORT_WRITE only on
    // writes; BT_READ/BT_PORT_READ only on reads. Accesses of other kinds are
    // ignored and do not affect the hit and skip counters.
    bool matches = (m_type == BT_ACSESS || m_type == BT_PORT_ACSESS)
                || ((m_type == BT_WRITE || m_type == BT_PORT_WRITE) && isWrite)
                || ((m_type == BT_READ  || m_type == BT_PORT_READ)  && !isWrite);
    if (!matches)
        return false;

    if (!countHit())
        return false;
    g_emulation->debugRequest(m_cpu);
    return true;
}



//##### Breakpoint Manager class ####

BreakpointCounters* BreakpointManager::Entry::counters() const
{
    if (codeBp)
        return codeBp;
    return dataBp;
}


BreakpointManager::BreakpointManager(Cpu* cpu) : m_cpu(cpu)
{
}


BreakpointManager::~BreakpointManager()
{
    clear();
}


int BreakpointManager::add(BreakpointType type, uint16_t addr, DebugClient owner)
{
    int id = find(type, addr);
    if (id >= 0)
        return id;

    Entry entry;
    entry.id = m_nextId++;
    entry.type = type;
    entry.addr = addr;
    entry.owner = owner;
    entry.enabled = true;
    entry.codeBp = nullptr;
    entry.dataBp = nullptr;
    if (type == BT_EXEC)
        entry.codeBp = new CodeBreakpoint(addr);
    else
        entry.dataBp = new DataBreakpoint(addr, type);
    entry.counters()->setManager(this, entry.id);
    attach(entry);
    m_entries.push_back(entry);
    ++m_changeCount;
    return entry.id;
}


int BreakpointManager::find(BreakpointType type, uint16_t addr) const
{
    for (const Entry& entry : m_entries)
        if (entry.type == type && entry.addr == addr)
            return entry.id;
    return -1;
}


bool BreakpointManager::remove(int id)
{
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
        if (it->id == id) {
            destroy(*it);
            m_entries.erase(it);
            ++m_changeCount;
            return true;
        }
    return false;
}


void BreakpointManager::clear()
{
    for (Entry& entry : m_entries)
        destroy(entry);
    m_entries.clear();
    ++m_changeCount;
}


void BreakpointManager::clear(BreakpointClass bpClass)
{
    for (auto it = m_entries.begin(); it != m_entries.end(); ) {
        if (getBreakpointClass(it->type) == bpClass) {
            destroy(*it);
            it = m_entries.erase(it);
        } else
            ++it;
    }
    ++m_changeCount;
}


bool BreakpointManager::setEnabled(int id, bool enabled)
{
    Entry* entry = findEntry(id);
    if (!entry)
        return false;
    if (entry->enabled != enabled) {
        if (enabled)
            attach(*entry);
        else
            detach(*entry);
        entry->enabled = enabled;
        ++m_changeCount;
    }
    return true;
}


bool BreakpointManager::setSkipCount(int id, int skipCount)
{
    Entry* entry = findEntry(id);
    if (!entry)
        return false;
    entry->counters()->setSkipCount(skipCount);
    ++m_changeCount;
    return true;
}


bool BreakpointManager::setComment(int id, const std::string& comment)
{
    Entry* entry = findEntry(id);
    if (!entry)
        return false;
    entry->counters()->setComment(comment);
    ++m_changeCount;
    return true;
}


bool BreakpointManager::get(int id, BreakpointDesc& desc) const
{
    const Entry* entry = findEntry(id);
    if (!entry)
        return false;
    fillDesc(*entry, desc);
    return true;
}


void BreakpointManager::list(std::vector<BreakpointDesc>& descs) const
{
    descs.clear();
    descs.reserve(m_entries.size());
    for (const Entry& entry : m_entries) {
        BreakpointDesc desc;
        fillDesc(entry, desc);
        descs.push_back(desc);
    }
}


int BreakpointManager::takeStopBreakpoint()
{
    int id = m_stopBpId;
    m_stopBpId = -1;
    return id;
}


BreakpointManager::Entry* BreakpointManager::findEntry(int id)
{
    for (Entry& entry : m_entries)
        if (entry.id == id)
            return &entry;
    return nullptr;
}


const BreakpointManager::Entry* BreakpointManager::findEntry(int id) const
{
    for (const Entry& entry : m_entries)
        if (entry.id == id)
            return &entry;
    return nullptr;
}


void BreakpointManager::attach(Entry& entry)
{
    if (!m_cpu)
        return;

    if (entry.codeBp)
        m_cpu->addHook(entry.codeBp); // sets the hook's CPU
    else {
        entry.dataBp->setCpu(m_cpu);
        if (getBreakpointClass(entry.type) == BC_IO)
            m_cpu->addPortBreakpoint(entry.dataBp);
        else
            m_cpu->addDataBreakpoint(entry.dataBp);
    }
}


void BreakpointManager::detach(Entry& entry)
{
    if (!m_cpu)
        return;

    if (entry.codeBp) {
        m_cpu->removeHook(entry.codeBp);
        entry.codeBp->setCpu(nullptr); // so that the destructor doesn't remove it once more
    } else {
        if (getBreakpointClass(entry.type) == BC_IO)
            m_cpu->removePortBreakpoint(entry.dataBp);
        else
            m_cpu->removeDataBreakpoint(entry.dataBp);
        entry.dataBp->setCpu(nullptr);
    }
}


void BreakpointManager::destroy(Entry& entry)
{
    // Destructors detach enabled breakpoints from the CPU
    delete entry.codeBp;
    delete entry.dataBp;
    entry.codeBp = nullptr;
    entry.dataBp = nullptr;
}


void BreakpointManager::fillDesc(const Entry& entry, BreakpointDesc& desc) const
{
    const BreakpointCounters* counters = entry.counters();
    desc.id = entry.id;
    desc.type = entry.type;
    desc.addr = entry.addr;
    desc.owner = entry.owner;
    desc.enabled = entry.enabled;
    desc.skipCount = counters->getSkipCount();
    desc.remaining = counters->getRemaining();
    desc.hitCount = counters->getHitCount();
    desc.comment = counters->getComment();
}
