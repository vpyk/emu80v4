/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2025
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

#include <sstream>

#include "Pal.h"
#include "Globals.h"
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
    m_lastFile = fileName;
    m_platform->updateDebugger();
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
    } else if (propertyName == "loadFile") {
        string fileName = values[0].asString();
        if (!fileName.empty()) {
            bool res = loadFile(fileName, false); // todo: consider to replace with m_platform->loadFile()
            if (res) {
                m_lastFile = fileName;
                m_platform->updateDebugger();
            }
            return res;
        }
    } else if (propertyName == "loadRunFile") {
        string fileName = values[0].asString();
        if (!fileName.empty()) {
            bool res = loadFile(fileName, true); // todo: consider to replace with m_platform->loadFile()
            if (res) {
                m_lastFile = fileName;
                m_platform->updateDebugger();
            }
            return res;
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

    else if (propertyName == "allowMultiblock")
        return m_multiblockAvailable ? m_allowMultiblock ? "yes" : "no" : "";
    else if (propertyName == "lastFile")
        return m_lastFile;

    return "";
}


bool FileLoader::loadHex(const uint8_t* hexFile, size_t fileLen, uint16_t& minAddr)
{
    stringstream stream(std::string(hexFile, hexFile + fileLen));

    minAddr = 0xffff;
    string line;
    while (getline(stream, line)) {
        vector<uint8_t> data;
        hexStr(line, data);
        if( data.size() < 5)
            return false;

        int bytes = data[0];
        uint16_t addr = (data[1] << 8 | data[2]);
        int type = data[3];

        if (type == 1)
            return true;
        if (type == 0) {
            if (addr < minAddr)
                minAddr = addr;

            for (int i = 0; i < bytes; i++) {
                if (i >= data.size())
                    continue;

                m_as->writeByte(addr + i, data[i + 4]);
            }
        }
    }

    return true;
}


void FileLoader::hexStr(const std::string &str, std::vector<uint8_t> &data)
{
    data.clear();

    size_t pos = 0;

    if (pos >= str.size())
        return;

    if (str[pos++] != ':')
        return;

    for(;;) {
        if (pos > str.size() - 2)
            return;

        int bt = 0;
        try {
            stringstream ss;
            ss << str[pos];
            ss << str[pos + 1];
            ss >> hex >> bt;
            data.push_back(bt);
        } catch(...) {}

        pos += 2;
    }

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
        g_emulation->exec((int64_t)cpu->getKDiv() * m_skipTicks, true);
        afterReset();
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


static const uint8_t casHeader[8] = {0x1F, 0xA6, 0xDE, 0xBA, 0xCC, 0x13, 0x7D, 0x74};
static const uint8_t tzxHeader[8] = {0x5A, 0x58, 0x54, 0x61, 0x70, 0x65, 0x21, 0x1A};
static const uint8_t tapHeader[2] = {0x13, 0x00};


TapeFileParser::TapeFileParser(uint8_t* data, int size) :
    m_data(data), m_size(size)
{
    if (m_size < 8)
        m_format = Format::MF_UNKNOWN;
    else if (!memcmp(data, casHeader, sizeof(casHeader)))
        m_format = Format::MF_CAS;
    else if (!memcmp(data, tzxHeader, sizeof(tzxHeader)))
        m_format = Format::MF_TZX;
    else if (!memcmp(data, tapHeader, sizeof(tapHeader)))
        m_format = Format::MF_TAP;
    else
        m_format = Format::MF_UNKNOWN;
}


bool TapeFileParser::getNextBlock(int& pos, int& size)
{
    // returns false if no block found
    // pos - block offset
    // size - maximum block size, for cas may be greater than real block size

    // default return values if no next block
    pos = m_size;
    size = 0;

    unsigned curPos;

    switch (m_format) {

    case Format::MF_UNKNOWN:
        pos = m_curPos;
        size = m_size - m_curPos;
        return size != 0;

    case Format::MF_CAS: {
        m_curPos = (m_curPos + 7) & ~7;
        if (m_size - m_curPos < 8)
            return false;
        while (memcmp(m_data + m_curPos, casHeader, 8)) {
            m_curPos += 8;
            if (m_size - m_curPos < 8)
                return false;
        }
        m_curPos += 8;
        pos = m_curPos;

        int nextPos = pos;
        while (memcmp(m_data + nextPos, casHeader, 8)) {
            nextPos += 8;
            if (m_size - nextPos < 8) {
                size = m_size - m_curPos;
                m_curPos = m_size;
                return true;
            }
        }

        size = nextPos - pos;
        m_curPos = nextPos;
        return true; }

    case Format::MF_TZX:
        curPos = m_nextBlockPos;
        while(true) {
            if (int(curPos) >= m_size)
                return false;
            uint8_t blockId = m_data[curPos];
            switch (blockId) {
            case 0x5A: {
                // TZX Header or Glue block
                if (m_size - curPos < 10)
                    return false;
                if (memcmp(m_data + curPos, tzxHeader, 8))
                    return false;
                curPos += 10;
                break;
            }
            case 0x32: {
                // Archive info block
                if (m_size - curPos < 3)
                    return false;
                uint16_t blockSize = m_data[curPos + 1] + (m_data[curPos + 2] << 8);
                curPos += 3;
                if (m_size - curPos < blockSize)
                    return false;
                curPos += blockSize;
                break;
            }
            case 0x35: {
                // Custom info block
                if (m_size - curPos < 21)
                    return false;
                uint32_t blockSize = m_data[curPos + 17] + (m_data[curPos + 18] << 8) + (m_data[curPos + 19] << 16) + (m_data[curPos + 20] << 24);
                curPos += 21;
                if (m_size - curPos < blockSize)
                    return false;
                curPos += blockSize;
                break;
            }
            case 0x30: {
                // Text description
                if (m_size - curPos < 2)
                    return false;
                uint32_t blockSize = m_data[curPos + 1];
                curPos += 2;
                if (m_size - curPos < blockSize)
                    return false;
                curPos += blockSize;
                break;
            }
            case 0x4B: {
                // Kansas City (MSX) block
                if (m_size - curPos < 5)
                    return false;
                uint32_t blockSize = m_data[curPos + 1] + (m_data[curPos + 2] << 8) + (m_data[curPos + 3] << 16) + (m_data[curPos + 4] << 24);
                curPos += 5;
                if (m_size - curPos < blockSize)
                    return false;
                pos = curPos + 12;
                size = blockSize - 12;
                m_nextBlockPos = curPos + blockSize;
                return true;
            }
            }
        }
        return false; // this should never occur
    case Format::MF_TAP:
        curPos = m_nextBlockPos;
        while(true) {
            if (int(curPos) >= m_size - 2)
                return false;
            uint16_t blockSize = m_data[curPos] + (m_data[curPos + 1] << 8);
            if (m_size - curPos < blockSize)
                return false;
            curPos += 2;
            m_nextBlockPos = curPos + blockSize;
                return true;
            }
        return false; // this should never occur
    }
    return false; // this should never occur
}
