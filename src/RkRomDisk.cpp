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

#include <string.h>

#include "Pal.h"
#include "Ppi8255Circuit.h"
#include "RkRomDisk.h"

using namespace std;


RkRomDisk::RkRomDisk(string romDiskName)
{
    m_romDisk = new uint8_t[65536];
    memset(m_romDisk, 0xFF, 65536);
    palReadFromFile(romDiskName, 0, 65536, m_romDisk);
}


RkRomDisk::~RkRomDisk()
{
    delete[] m_romDisk;
}


uint8_t RkRomDisk::getPortA()
{
    return m_romDisk[m_curAddr];
}


void RkRomDisk::setPortB(uint8_t value)
{
    m_curAddr = (m_curAddr & ~0xff) | value;
}


void RkRomDisk::setPortC(uint8_t value)
{
    m_curAddr = (m_curAddr & ~0xff00) | (value << 8);
}


/*bool RkRomDisk::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "romDisk") {
        //attachRkFddRegister(static_cast<RkFddRegister*>(g_emulation->findObject(values[0].asString())));
        return true;
    }
    return false;
}*/
