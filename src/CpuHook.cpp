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
#include "CpuHook.h"
#include "TapeRedirector.h"

using namespace std;


CpuHook::CpuHook(int addr)
{
    m_hookAddr = addr;
}


CpuHook::~CpuHook()
{
    //dtor
}


bool CpuHook::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "enabled") {
        if (values[0].asString() == "yes")
            m_isEnabled = true;
        else if (values[0].asString() == "no")
            m_isEnabled = false;
        return true;
    } else if (propertyName == "tapeRedirector") {
        setTapeRedirector(static_cast<TapeRedirector*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


string CpuHook::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "enabled")
        return m_isEnabled ? "yes" : "no";

    return "";
}
