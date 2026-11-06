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

#ifndef BREAKPOINTS_H
#define BREAKPOINTS_H

#include <string>

#include "CpuHook.h"

class Cpu;


class CodeBreakpoint : public CpuHook
{
    public:
        CodeBreakpoint(uint16_t addr) : CpuHook(addr) {}
        virtual ~CodeBreakpoint();
        bool hookProc() override;
        bool isBreakpoint() override {return true;}

        void setSkipCount(int n)  { m_skipCount = n; m_remaining = n; }
        int  getSkipCount() const { return m_skipCount; }
        int  getRemaining() const { return m_remaining; }
        void setRemaining(int n)  { m_remaining = n; }
        int  getHitCount() const  { return m_hitCount; }
        void setComment(const std::string& c) { m_comment = c; }
        const std::string& getComment() const { return m_comment; }

    private:
        int m_skipCount = 0;    // fixed property
        int m_remaining = 0;    // decremented each hit, reset from skipCount on stop
        int m_hitCount = 0;     // hits since last stop
        std::string m_comment;
};

enum BreakpointType {
    BT_EXEC,
    BT_WRITE,
    BT_READ,
    BT_ACSESS,
    BT_PORT_WRITE,
    BT_PORT_READ,
    BT_PORT_ACSESS
};

struct BreakpointInfo {
    uint16_t addr;
    BreakpointType type;
    CodeBreakpoint* codeBp = nullptr;
};


class DataBreakpoint : public EmuObject
{
public:
    DataBreakpoint(uint16_t addr, BreakpointType type);
    virtual ~DataBreakpoint();

    void setCpu(Cpu* cpu) { m_cpu = cpu; }
    void setSkipCount(int n)  { m_skipCount = n; m_remaining = n; }
    int  getSkipCount() const { return m_skipCount; }
    int  getRemaining() const { return m_remaining; }
    void setRemaining(int n)  { m_remaining = n; }
    uint16_t getAddr()  { return m_addr; }
    BreakpointType getType() { return m_type; }
    int  getHitCount() const { return m_hitCount; }
    void setComment(const std::string& c) { m_comment = c; }
    const std::string& getComment() const { return m_comment; }
    bool check(bool isWrite);

protected:
    Cpu*   m_cpu = nullptr;
    uint16_t m_addr;
    BreakpointType m_type;
    int    m_skipCount = 0;   // fixed property
    int    m_remaining = 0;   // decremented each hit, reset from skipCount on stop
    int    m_hitCount = 0;    // hits since last stop
    std::string m_comment;
};


#endif // BREAKPOINTS_H
