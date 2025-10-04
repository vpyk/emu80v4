/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2023-2025
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

// Emulation core rouninues to be called from platform abstraction layer (PAL)


#include "Debugger.h"
#include "Platform.h"

using namespace std;


Platform* emuGetPlatform(PalWindow* wnd)
{
    return wnd ? static_cast<EmuWindow*>(wnd)->getPlatform() : nullptr;
}


IDebugger* emuGetDebugger(PalWindow* wnd)
{
    if (!wnd)
        return nullptr;

    IDebugger* dbg = dynamic_cast<DebugWindow*>(wnd);
    if (!dbg) {
        Platform* platform = emuGetPlatform(wnd);
        if (platform)
            dbg = platform->getDebugger();
    }

    return dbg;
}


void emuCreateDebugger(PalWindow* wnd)
{
    if (!wnd)
        return;

    Platform* platform = emuGetPlatform(wnd);
    if (platform)
        platform->createDebugger();
}
