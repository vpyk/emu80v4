/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2018-2024
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


#include "Globals.h"
#include "Emulation.h"
#include "EmuWindow.h"
#include "EmuConfig.h"

using namespace std;


// Keyboard event
void emuKeyboard(PalWindow* wnd, PalKeyCode key, bool isPressed, unsigned unicodeKey)
{
    g_emulation->processKey(static_cast<EmuWindow*>(wnd), key, isPressed, unicodeKey);
}


void emuResetKeys(PalWindow* wnd)
{
    g_emulation->resetKeys(static_cast<EmuWindow*>(wnd));
}


// System request
void emuSysReq(PalWindow* wnd, SysReq sr)
{
    g_emulation->sysReq(static_cast<EmuWindow*>(wnd), sr);
}


// Active window changed
void emuFocusWnd(PalWindow* wnd)
{
    g_emulation->setWndFocus(static_cast<EmuWindow*>(wnd));
}


// Drop files event
void emuDropFile(PalWindow* wnd, const char* fileName)
{
    g_emulation->dropFile(static_cast<EmuWindow*>(wnd), std::string(fileName));
}


// Main emulation procedure
void emuEmulationCycle()
{
    g_emulation->mainLoopCycle();
}


// Set emulation object's single property value
bool emuSetPropertyValue(const string& objName, const string& propName, const string& value)
{
    EmuObject* obj = g_emulation->findObject(objName);
    if (!obj)
        return false;
    return obj->setProperty(propName, value);
}


// Get emulation object's property string value
string emuGetPropertyValue(const string& objName, const string& propName)
{
    EmuObject* obj = g_emulation->findObject(objName);
    if (obj)
        return obj->getPropertyStringValue(propName);
    else
        return "";
}


// Get platform list
const std::vector<PlatformInfo>* emuGetPlatforms()
{
    EmuConfig* config = g_emulation->getConfig();
    return config->getPlatformInfos();
}


// Run specified platform
void emuSelectPlatform(const std::string& platform)
{
    g_emulation->newPlatform(platform);
}

// Returns current emulation speed factor
double emuGetEmulationSpeedFactor()
{
    if (g_emulation->getPausedState())
        return 0.;

    if (g_emulation->getFullThrottleState())
        return -1;

    return g_emulation->getSpeedUpFactor();
}


// Turns off fullscreen mode (for wasm version)
void emuExitFullscreenMode(PalWindow* wnd)
{
    static_cast<EmuWindow*>(wnd)->setFullScreen(false);
}
