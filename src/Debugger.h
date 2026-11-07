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

#ifndef DEBUGGER_H
#define DEBUGGER_H


enum DebugCommand {
    DCMD_REPAINT,
    DCMD_HERE,
    DCMD_RUN,
    DCMD_STEP,
    DCMD_OVER,
    DCMD_BPOINT,
    DCMD_SKIP,
    DCMD_SAVEMEM,
    DCMD_MNEMO,
    DCMD_MINI,
    DCMD_CODE,
    DCMD_DATA,
    DCMD_REGS,
    DCMD_FLAGS,
    DCMD_ADDR,
    DCMD_EDIT
};


// Debugger UI (front-end) of a platform. It works with the platform's DebugSession;
// the platform notifies it of stops and calls it on the emulator's update and draw.
class IDebugger
{
public:
    virtual ~IDebugger() {}

    virtual void startDebug() = 0;      // stopped in the debugger by the UI: show the UI
    virtual void refreshDebug() {}      // stopped by another client: refresh the UI if it is open
    virtual void update() {}            // platform state changed (e.g. a file loaded)
    virtual void draw() {}
};


#endif // DEBUGGER_H
