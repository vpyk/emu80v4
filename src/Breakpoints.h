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
#include <vector>

#include "CpuHook.h"

class Cpu;
class BreakpointManager;


// Debugger clients: the debugger UI of the build (debug window, or the web debugger
// in wasm) and external clients (MCP server). Used to tell who has set a breakpoint
// or stopped the emulation, so that the UI shows up on its own stops only.
enum DebugClient {
    DC_UI,
    DC_EXTERNAL
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


// Breakpoint classes: code, memory or I/O ports
enum BreakpointClass {
    BC_EXEC,
    BC_MEM,
    BC_IO
};

BreakpointClass getBreakpointClass(BreakpointType type);


// Hit counting common for all breakpoint kinds
class BreakpointCounters
{
public:
    void setSkipCount(int n)  { m_skipCount = n; m_remaining = n; }
    int  getSkipCount() const { return m_skipCount; }
    int  getRemaining() const { return m_remaining; }
    int  getHitCount() const  { return m_hitCount; }
    void setComment(const std::string& c) { m_comment = c; }
    const std::string& getComment() const { return m_comment; }

    // Breakpoint manager to report stops to (not set for temporary breakpoints)
    void setManager(BreakpointManager* manager, int id) { m_manager = manager; m_id = id; }

protected:
    // Counts a hit, returns true if the breakpoint should stop now
    bool countHit();

private:
    BreakpointManager* m_manager = nullptr;
    int m_id = 0;
    int m_skipCount = 0;    // stop on every (skipCount + 1)-th hit
    int m_remaining = 0;    // hits to skip before the next stop, reset from skipCount on stop
    int m_hitCount = 0;     // total number of hits since the breakpoint was set
    std::string m_comment;
};


class CodeBreakpoint : public CpuHook, public BreakpointCounters
{
    public:
        CodeBreakpoint(uint16_t addr) : CpuHook(addr) {}
        virtual ~CodeBreakpoint();
        bool hookProc() override;
        bool isBreakpoint() override {return true;}
};


struct BreakpointInfo {
    uint16_t addr;
    BreakpointType type;
    CodeBreakpoint* codeBp = nullptr;
};


class DataBreakpoint : public BreakpointCounters
{
public:
    DataBreakpoint(uint16_t addr, BreakpointType type);
    virtual ~DataBreakpoint();

    void setCpu(Cpu* cpu) { m_cpu = cpu; }
    uint16_t getAddr()  { return m_addr; }
    BreakpointType getType() { return m_type; }
    bool check(bool isWrite);

protected:
    Cpu*   m_cpu = nullptr;
    uint16_t m_addr;
    BreakpointType m_type;
};


// Breakpoint description for debugger front-ends
struct BreakpointDesc {
    int id;
    BreakpointType type;
    uint16_t addr;
    DebugClient owner;
    bool enabled;
    int skipCount;
    int remaining;
    int hitCount;
    std::string comment;
};


// Owns all user breakpoints of a CPU, attaches them to the CPU and detaches when
// they are disabled or removed. Breakpoints are identified by a stable id.
class BreakpointManager
{
public:
    BreakpointManager(Cpu* cpu);
    ~BreakpointManager();

    // Adds a breakpoint, returns its id. If a breakpoint of the same type
    // at the same address exists, returns its id and changes nothing.
    int add(BreakpointType type, uint16_t addr, DebugClient owner);
    int find(BreakpointType type, uint16_t addr) const; // returns id or -1
    bool remove(int id);
    void clear();
    void clear(BreakpointClass bpClass);

    bool setEnabled(int id, bool enabled);
    bool setSkipCount(int id, int skipCount);
    bool setComment(int id, const std::string& comment);

    bool get(int id, BreakpointDesc& desc) const;
    void list(std::vector<BreakpointDesc>& descs) const; // in creation order
    unsigned getChangeCount() const {return m_changeCount;} // incremented on every change of the list or settings

    // The first breakpoint that has stopped the emulation since the last call
    // (-1 if none); called on stop to find out the stop reason
    int takeStopBreakpoint();
    void resetStopBreakpoint() {m_stopBpId = -1;}

    void onBreakpointStop(int id) {if (m_stopBpId < 0) m_stopBpId = id;} // called by breakpoints

private:
    struct Entry {
        int id;
        BreakpointType type;
        uint16_t addr;
        DebugClient owner;
        bool enabled;
        CodeBreakpoint* codeBp; // for BT_EXEC
        DataBreakpoint* dataBp; // for other types
        BreakpointCounters* counters() const;
    };

    Cpu* m_cpu;
    std::vector<Entry> m_entries;
    int m_nextId = 1;
    int m_stopBpId = -1;
    unsigned m_changeCount = 0;

    Entry* findEntry(int id);
    const Entry* findEntry(int id) const;
    void attach(Entry& entry);
    void detach(Entry& entry);
    void destroy(Entry& entry);
    void fillDesc(const Entry& entry, BreakpointDesc& desc) const;
};


#endif // BREAKPOINTS_H
