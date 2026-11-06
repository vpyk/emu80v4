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


//##### Code Breakpoint class ####

CodeBreakpoint::~CodeBreakpoint()
{
    if (m_cpu)
        m_cpu->removeHook(this);
}


bool CodeBreakpoint::hookProc()
{
    ++m_hitCount;
    if (m_remaining > 0) {
        --m_remaining;
        return false;
    }
    // Actual stop — reset counters.
    m_remaining = m_skipCount;
    m_hitCount = 0;
    g_emulation->debugRequest(m_cpu);
    return true;
}



//##### Data Breakpoint class ####

DataBreakpoint::DataBreakpoint(uint16_t addr, BreakpointType type)
    : m_addr(addr), m_type(type)
{}

DataBreakpoint::~DataBreakpoint()
{
    if (m_cpu)
        m_cpu->removeDataBreakpoint(this);
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

    ++m_hitCount;
    if (m_remaining > 0) {
        --m_remaining;
        return false;
    }

    // Actual stop — reset counters.
    m_remaining = m_skipCount;
    m_hitCount = 0;
    g_emulation->debugRequest(m_cpu);
    return true;
}
