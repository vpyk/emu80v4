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

#include "Pal.h"
#include "PalFile.h"
#include "AddrSpace.h"
#include "Memory.h"
#include "EmuWindow.h"
#include "Emulation.h"
#include "Platform.h"
#include "RamDisk.h"

using namespace std;


RamDisk::RamDisk(unsigned nPages, unsigned defPageSize)
{
    m_nPages = nPages;
    m_defPageSize = defPageSize;
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
    } else if (propertyName == "filter") {
        m_filter = values[0].asString();
        return true;
    }

    return false;
}


bool RamDisk::saveToFile()
{
    string fileName = palOpenFileDialog("Open RAM disk file", m_filter, true, m_platform->getWindow());
    g_emulation->restoreFocus();
    if (fileName == "")
        return false;

    PalFile file;
    file.open(fileName, "w");
    if (!file.isOpen())
        return false;

    for (unsigned i = 0; i < m_nPages; i++) {
        unsigned pageSize = m_defPageSize;

        Ram* ram = dynamic_cast<Ram*>(m_pages[i]);
        if (ram)
            pageSize = ram->getSize();

        for (unsigned pos = 0; pos < pageSize; pos++)
            file.write8(m_pages[i]->readByte(pos));
        if (pageSize < m_defPageSize)
            for (unsigned pos = 0; pos < m_defPageSize - pageSize; pos++)
                file.write8(0);
    }

    file.close();
    return true;
}


bool RamDisk::loadFromFile()
{
    string fileName = palOpenFileDialog("Save RAM disk file", m_filter, false, m_platform->getWindow());
    g_emulation->restoreFocus();
    if (fileName == "")
        return false;

    PalFile file;
    file.open(fileName, "r");
    if (!file.isOpen())
        return false;

    unsigned expectedSize = 0;
    for (unsigned i = 0; i < m_nPages; i++) {
        unsigned pageSize = m_defPageSize;

        Ram* ram = dynamic_cast<Ram*>(m_pages[i]);
        if (ram)
            pageSize = ram->getSize();

        if (pageSize < m_defPageSize)
            pageSize = m_defPageSize;

        expectedSize += m_defPageSize;
    }

    if (file.getSize() == expectedSize) {

        for (unsigned i = 0; i < m_nPages; i++) {
            unsigned pageSize = m_defPageSize;

            Ram* ram = dynamic_cast<Ram*>(m_pages[i]);
            if (ram)
                pageSize = ram->getSize();

            for (unsigned pos = 0; pos < pageSize; pos++)
                m_pages[i]->writeByte(pos, file.read8());

            if (pageSize < m_defPageSize)
                for (unsigned pos = 0; pos < m_defPageSize - pageSize; pos++)
                    file.read8();
        }
    } else {
        emuLog << "Invalid file size: " << fileName << "\n";
    }


    file.close();
    return true;
}
