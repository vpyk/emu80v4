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

#include "Emulation.h"
#include "PlatformCore.h"
#include "EmuWindow.h"
#include "KbdLayout.h"

using namespace std;

PlatformCore::PlatformCore()
{
    //ctor
}



PlatformCore::~PlatformCore()
{
    //dtor
}



void PlatformCore::attachWindow(EmuWindow* win)
{
    m_window = win;
}



bool PlatformCore::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "window") {
        attachWindow(static_cast<EmuWindow*>(g_emulation->findObject(values[0].asString())));
        return true;
    }
    return false;
}
