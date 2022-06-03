/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2018-2022
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
    } else if (propertyName == "label") {
        m_label = values[0].asString();
        return true;
    } else if (propertyName == "fileName" || propertyName == "permanentFileName") {
        m_fileName = values[0].asString();
        if (!m_fileName.empty() && propertyName == "fileName")
            loadFromFile();
        return true;
    } else if (propertyName == "autoLoad") {
        if (values[0].asString() == "yes")
            m_autoLoad = true;
        else if (values[0].asString() == "no")
            m_autoLoad = false;
    } else if (propertyName == "autoSave") {
        if (values[0].asString() == "yes")
            m_autoSave = true;
        else if (values[0].asString() == "no")
            m_autoSave = false;
    }

    return false;
}


string RamDisk::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "fileName")
        return m_fileName;
    else if (propertyName == "permanentFileName")
        return m_autoLoad ? m_fileName : "";
    else if (propertyName == "label")
        return m_label;
    else if (propertyName == "autoLoad")
        return m_autoLoad ? "yes" : "no";
    else if (propertyName == "autoSave")
        return m_autoSave ? "yes" : "no";

    return "";
}


void RamDisk::saveFileAs()
{
    string oldFileName = m_fileName;
    m_fileName = "";
    saveToFile();
    if (m_fileName.empty())
        m_fileName = oldFileName;
}


void RamDisk::saveToFile()
{
    if (m_fileName.empty()) {
        m_fileName = palOpenFileDialog("Save RAM disk file", m_filter, true, m_platform->getWindow());
        g_emulation->restoreFocus();
        if (m_fileName == "")
            return;
    }

    PalFile file;
    file.open(m_fileName, "w");
    if (!file.isOpen())
        return;

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
}


void RamDisk::openFile()
{
    string oldFileName = m_fileName;
    m_fileName = "";
    loadFromFile();
    if (m_fileName.empty())
        m_fileName = oldFileName;
}


void RamDisk::loadFromFile()
{
    if (m_fileName.empty()) {
        m_fileName = palOpenFileDialog("Load RAM disk file", m_filter, false, m_platform->getWindow());
        g_emulation->restoreFocus();
        if (m_fileName == "")
            return;
    }

    PalFile file;
    file.open(m_fileName, "r");
    if (!file.isOpen())
        return;

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
        emuLog << "Invalid file size: " << m_fileName << "\n";
    }

    file.close();
}


void RamDisk::init()
{
    if (m_autoLoad && !m_fileName.empty())
        loadFromFile();
}


void RamDisk::shutdown()
{
    if (m_autoSave && !m_fileName.empty())
        saveToFile();
}
