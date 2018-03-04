/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017
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
#include "FileLoader.h"
#include "Emulation.h"
#include "Platform.h"
#include "Cpu.h"

using namespace std;

void FileLoader::attachAddrSpace(AddressableDevice* as)
{
    m_as = as;
}


void FileLoader::setFilter(const std::string& filter)
{
    m_filter = filter;
}


bool FileLoader::chooseAndLoadFile(bool run)
{
    string fileName = palOpenFileDialog("Open file", m_filter, false);
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
    }
    return false;
}


bool RkFileLoader::loadFile(const std::string& fileName, bool run)
{
    int fileSize;
    uint8_t* buf = palReadFile(fileName, fileSize, false);
    if (!buf)
        return false;

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

    for (uint16_t addr = begAddr; addr <= endAddr; addr++)
        m_as->writeByte(addr, *ptr++);

    if (run) {
        m_platform->reset();
        Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(m_platform->getCpu());
        if (cpu) {
            cpu->disableHooks();
            g_emulation->exec((int64_t)cpu->getKDiv() * m_skipTicks);
            cpu->enableHooks();
            cpu->setPC(begAddr);
        }
    }

    return true;
}
