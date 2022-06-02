/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2022
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
#include "Emulation.h"
#include "Platform.h"
#include "Cpu.h"
#include "EmuWindow.h"
#include "TapeRedirector.h"
#include "FileLoader.h"


using namespace std;


void FileLoader::attachAddrSpace(AddressableDevice* as)
{
    m_as = as;
}


void FileLoader::attachTapeRedirector(TapeRedirector* tapeRedirector)
{
    m_tapeRedirector = tapeRedirector;
}


void FileLoader::setFilter(const std::string& filter)
{
    m_filter = filter;
}


bool FileLoader::chooseAndLoadFile(bool run)
{
    string fileName = palOpenFileDialog("Open file", m_filter, false, m_platform->getWindow());
    g_emulation->restoreFocus();
    if (fileName == "")
        return true;
    if (!loadFile(fileName, run)) {
        emuLog << "Error loading file: " << fileName << "\n";
        return false;
    }
    return true;
}


bool FileLoader::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "filter") {
        setFilter(values[0].asString());
        return true;
    } else if (propertyName == "addrSpace") {
        attachAddrSpace(static_cast<AddressableDevice*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "skipTicks" && values[0].isInt()) {
        m_skipTicks = values[0].asInt();
        return true;
    } else if (propertyName == "tapeRedirector") {
        attachTapeRedirector(static_cast<TapeRedirector*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "allowMultiblock") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_allowMultiblock = values[0].asString() == "yes";
            return true;
        }
    }
    return false;
}


string FileLoader::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    else if (propertyName == "allowMultiblock") {
        return m_multiblockAvailable ? m_allowMultiblock ? "yes" : "no" : "";
    }

    return "";
}


bool RkFileLoader::loadFile(const std::string& fileName, bool run)
{
    int fileSize;
    uint8_t* buf = palReadFile(fileName, fileSize, false);
    if (!buf)
        return false;

    int fullSize = fileSize;

    if (fileSize < 9) {
        delete[] buf;
        return false;
    }

    uint8_t* ptr = buf;

    if ((*ptr) == 0xE6) {
        ptr++;
        fileSize--;
    }

    uint16_t begAddr = (ptr[0] << 8) | ptr[1];
    uint16_t endAddr = (ptr[2] << 8) | ptr[3];
    ptr += 4;
    fileSize -= 4;

    uint16_t progLen = endAddr - begAddr + 1;

    if (begAddr == 0xE6E6 || begAddr == 0xD3D3 || fileSize < progLen + 2) {
        // Basic or EDM File
        delete[] buf;
        return false;
    }

    Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(m_platform->getCpu());

    if (run && cpu) {
        m_platform->reset();
        Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(m_platform->getCpu());
        cpu->disableHooks();
        g_emulation->exec((int64_t)cpu->getKDiv() * m_skipTicks);
    }

    for (unsigned addr = begAddr; addr <= endAddr; addr++)
        m_as->writeByte(addr, *ptr++);

    fileSize -= (endAddr - begAddr + 1);

    // skip CS
    while (fileSize >0 && (*ptr) != 0xE6) {
        ++ptr;
        --fileSize;
    }
    if (fileSize > 3) {
        fileSize -= 3;
        ptr += 3;
    } else
        fileSize = 0;

    // Find next block
    if (m_allowMultiblock && m_tapeRedirector && fileSize > 0)
        while (fileSize > 0 && (*ptr) != 0xE6) {
            ++ptr;
            --fileSize;
        }
    //if (fileSize > 0)
    //    --fileSize;

    delete[] buf;

    if (run && cpu) {
        afterReset();

        cpu->enableHooks();
        cpu->setPC(begAddr);
        if (m_allowMultiblock && m_tapeRedirector && fileSize > 0) {
            m_tapeRedirector->assignFile(fileName, "r");
            m_tapeRedirector->openFile();
            m_tapeRedirector->assignFile("", "r");
            m_tapeRedirector->setFilePos(fullSize - fileSize);
        }
    }

    return true;
}
