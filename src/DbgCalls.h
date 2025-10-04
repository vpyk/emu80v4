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

// Debugger rouninues to be called from platform abstraction layer (PAL) header file


#ifndef DBGCALLS_H
#define DBGCALLS_H

class Platform;
class PalWindow;
class DebugWindow;

Platform* emuGetPlatform(PalWindow* wnd);
IDebugger* emuGetDebugger(PalWindow* wnd);
void emuCreateDebugger(PalWindow* wnd);

#endif // EMUCALLS_H
