/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2017
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

#include <string>

#include "Pal.h"
#include "EmuWindow.h"

#include "Emulation.h"


Emulation* g_emulation = nullptr;

void keyboardCallback(PalWindow* wnd, PalKeyCode key, bool isPressed)
{
    g_emulation->processKey(static_cast<EmuWindow*>(wnd), key, isPressed);
}


void sysReqCallback(PalWindow* wnd, SysReq sr)
{
    g_emulation->sysReq(static_cast<EmuWindow*>(wnd), sr);
}


void focusWndCallback(PalWindow* wnd)
{
    g_emulation->setWndFocus(static_cast<EmuWindow*>(wnd));
}


void dropFileCallback(PalWindow* wnd, char* fileName)
{
    g_emulation->dropFile(static_cast<EmuWindow*>(wnd), std::string(fileName));
}


void emulationCycleCallback()
{
    g_emulation->mainLoopCycle();
}


int main (int argc, char** argv)
{
    if (!palInit(argc, argv))
        return 1;

    new Emulation(argc, argv); // g_emulation присваивается в конструкторе
    //Emulation emulation;
    //g_emulation = &emulation;

    palRegisterEmulationCycleCallbackFunc(emulationCycleCallback);
    palRegisterKbdCallbackFunc(keyboardCallback);
    palRegisterSysReqCallbackFunc(sysReqCallback);
    palRegisterFocusWndCallbackFunc(focusWndCallback);
    palRegisterDropFileCallbackFunc(dropFileCallback);

    palStart();

    palExecute();

    delete g_emulation;

    palQuit();
    return 0;
}

