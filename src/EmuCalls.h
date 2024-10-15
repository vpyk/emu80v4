/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2024
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

// Emulation core rouninues to be called from platform abstraction layer (PAL) header file


#ifndef EMUCALLS_H
#define EMUCALLS_H

#include <vector>

#include "PalWindow.h"

void emuKeyboard(PalWindow* wnd, PalKeyCode key, bool isPressed, unsigned unicodeKey = 0);
void emuResetKeys(PalWindow* wnd);
void emuSysReq(PalWindow* wnd, SysReq sr);
void emuFocusWnd(PalWindow* wnd);
void emuDropFile(PalWindow* wnd, const char* fileName);
void emuEmulationCycle();
bool emuSetPropertyValue(const std::string& objName, const std::string& propName, const std::string& value);
std::string emuGetPropertyValue(const std::string& objName, const std::string& propName);
const std::vector<PlatformInfo>* emuGetPlatforms();
void emuSelectPlatform(const std::string& platform);
double emuGetEmulationSpeedFactor();

void emuExitFullscreenMode(PalWindow* wnd);

#endif // EMUCALLS_H
