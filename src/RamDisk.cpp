/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2018
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
#include "RamDisk.h"

using namespace std;


RamDisk::RamDisk(unsigned nPages, unsigned pageSize)
{
    m_nPages = nPages;
    m_pageSize = pageSize;
    m_pages = new AddressableDevice* [nPages];
    for (unsigned i = 0; i < nPages; i++)
        m_pages[i] = nullptr;
}


RamDisk::~RamDisk()
{
    delete[] m_pages;
}


void RamDisk::attachPage(unsigned pageNo, AddressableDevice* as)
{
    m_pages[pageNo] = as;
}


bool RamDisk::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "page" && values[0].isInt()) {
            attachPage(values[0].asInt(), static_cast<AddressableDevice*>(g_emulation->findObject(values[1].asString())));
            return true;
    }

    return false;
}


bool RamDisk::loadFromFile()
{
    //
    return false;
}


bool RamDisk::saveToFile()
{
    //
    return false;
}
