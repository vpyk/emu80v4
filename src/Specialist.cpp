/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2018
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

#include <string.h>

#include "Pal.h"

#include "Specialist.h"
#include "Emulation.h"
#include "Globals.h"
#include "EmuWindow.h"
#include "Platform.h"
#include "AddrSpace.h"
#include "Fdc1793.h"
#include "SoundMixer.h"
#include "WavReader.h"
#include "Cpu.h"

using namespace std;


void SpecCore::draw()
{
    m_crtRenderer->renderFrame();
    m_window->drawFrame(m_crtRenderer->getPixelData());
    m_window->endDraw();
}


void SpecCore::attachCrtRenderer(CrtRenderer* crtRenderer)
{
    m_crtRenderer = crtRenderer;
}


bool SpecCore::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (PlatformCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<CrtRenderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }
    return false;
}


SpecRenderer::SpecRenderer()
{
    m_sizeX = m_prevSizeX = 384;
    m_sizeY = m_prevSizeY = 256;
    m_aspectRatio = m_prevAspectRatio = 576.0 * 9 / 704 / 8;
    m_bufSize = m_prevBufSize = m_sizeX * m_sizeY;
    int maxBufSize = 417 * 288;
    m_pixelData = new uint32_t[maxBufSize];
    m_prevPixelData = new uint32_t[maxBufSize];
    memset(m_pixelData, 0, m_bufSize * sizeof(uint32_t));
    memset(m_prevPixelData, 0, m_prevBufSize * sizeof(uint32_t));
}


void SpecRenderer::toggleColorMode()
{
    if (m_colorMode == SCM_MX)
        return; // в режиме MS другие режимы недоступны

    if (m_colorMode == SCM_MONO)
        m_colorMode = SCM_4COLOR;
    else if (m_colorMode == SCM_4COLOR)
        m_colorMode = SCM_8COLOR;
    else if (m_colorMode == SCM_8COLOR)
        m_colorMode = SCM_MONO;
}


void SpecRenderer::renderFrame()
{
    swapBuffers();

    int offsetX = 0;
    int offsetY = 0;

    if (m_showBorder) {
        m_sizeX = 417;
        m_sizeY = 288;
        memset(m_pixelData, 0, m_sizeX * m_sizeY * sizeof(uint32_t));
        offsetX = 21;
        offsetY = 10;
        m_aspectRatio = double(m_sizeY) * 4 / 3 / m_sizeX;
    } else {
        m_sizeX = 384;
        m_sizeY = 256;
        offsetX = offsetY = 0;
        m_aspectRatio = 576.0 * 9 / 704 / 8;
    }

    for (int row = 0; row < 256; row++)
        for (int col = 0; col < 48; col++) {
            int addr = col * 256 + row;
            uint8_t bt = m_screenMemory[addr];
            uint8_t colorByte = m_colorMemory[addr];
            uint32_t fgColor;
            uint32_t bgColor = 0;
            switch (m_colorMode) {
                case SCM_MONO:
                    fgColor = 0xC0C0C0;
                    break;
                case SCM_4COLOR:
                    fgColor = spec4ColorPalette[(colorByte & 0xC0) >> 6];
                    break;
                case SCM_8COLOR:
                    fgColor = spec8ColorPalette[((colorByte & 0xC0) >> 5) | ((colorByte & 0x10) >> 4)];
                    break;
                case SCM_MX:
                default:
                    fgColor = spec16ColorPalette[(colorByte & 0xF0) >> 4];
                    bgColor = spec16ColorPalette[colorByte & 0xF];
            }
            for (int pt = 0; pt < 8; pt++, bt <<= 1)
                m_pixelData[(row + offsetY) * m_sizeX + col * 8 + pt + offsetX] = (bt & 0x80) ? fgColor : bgColor;
        }
}


void SpecRenderer::toggleCropping()
{
    m_showBorder = !m_showBorder;
}


bool SpecRenderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "screenMemory") {
        attachScreenMemory(static_cast<SpecVideoRam*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "colorMode") {
        if (values[0].asString() == "mono")
            m_colorMode = SCM_MONO;
        else if (values[0].asString() == "4color")
            m_colorMode = SCM_4COLOR;
        else if (values[0].asString() == "8color")
            m_colorMode = SCM_8COLOR;
        else if (values[0].asString() == "mx")
            m_colorMode = SCM_MX;
        else
            return false;
        return true;
    } else if (propertyName == "visibleArea") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_showBorder = values[0].asString() == "yes";
            return true;
        }
    }
    return false;
}


string SpecRenderer::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "colorMode") {
        switch (m_colorMode) {
            case SCM_MONO:
                return "mono";
            case SCM_4COLOR:
                return "4color";
            case SCM_8COLOR:
                return "8color";
            case SCM_MX:
                return "mx";
        }
    } else if (propertyName == "visibleArea") {
        return m_showBorder ? "yes" : "no";
    } else if (propertyName == "crtMode") {
            return u8"384\u00D7256@50.08Hz" ;
    }

    return "";
}


SpecVideoRam::SpecVideoRam(int memSize)  : Ram(memSize)
{
    m_memSize = memSize;
    m_colorBuf = new uint8_t[memSize];
}


SpecVideoRam::~SpecVideoRam()
{
    delete[] m_colorBuf;
}


void SpecVideoRam::writeByte(int addr, uint8_t value)
{
    Ram::writeByte(addr, value);
    m_colorBuf[addr] = m_color;
}


void SpecVideoRam::setCurColor(uint8_t color)
{
    m_color = color;
}


void SpecVideoRam::reset()
{
    memset(m_colorBuf, m_memSize, 0x70); // нужно ли?
    m_color = 0x70;
};


void SpecMxMemPageSelector::reset()
{
    m_addrSpaceMapper->setCurPage(0);
}


void SpecMxMemPageSelector::writeByte(int addr, uint8_t value)
{
    if (addr == 0) // RAM
        m_addrSpaceMapper->setCurPage(1);
    else if (addr == 1) // RAM Disk
        m_addrSpaceMapper->setCurPage(m_onePageMode ? 2 : (value & 0x7) + 2);
    else // 2,3 - ROM
        m_addrSpaceMapper->setCurPage(0);
};


bool SpecMxMemPageSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "mapper") {
        attachAddrSpaceMapper(static_cast<AddrSpaceMapper*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "mode") {
        if (values[0].asString() == "1bank")
            m_onePageMode = true;
        else if (values[0].asString() == "8banks")
            m_onePageMode = false;
        else
            return false;
        return true;
    }

    return false;
}


void SpecMxColorRegister::writeByte(int, uint8_t value)
{
    if (m_videoRam)
        m_videoRam->setCurColor(value);
};


bool SpecMxColorRegister::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "videoRam") {
        attachVideoRam(static_cast<SpecVideoRam*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


bool SpecMxFddControlRegisters::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "fdc") {
        attachFdc1793(static_cast<Fdc1793*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


void SpecMxFddControlRegisters::writeByte(int addr, uint8_t value)
{
    addr &= 3;

    switch(addr) {
/*        case 0: // порт синхронизации FFF0
        case 1: // порт плотности дискет FFF1
            break;*/
        case 2:
            // порт стороны FFF2
            if (m_fdc)
                m_fdc->setHead(value & 1);
            break;
        case 3:
            // порт переключения дисководов FFF3
            if (m_fdc)
                m_fdc->setDrive(value & 1);
            break;
    }
}


SpecPpi8255Circuit::SpecPpi8255Circuit()
{
    m_tapeSoundSource = new GeneralSoundSource;
    g_emulation->getSoundMixer()->addSoundSource(m_tapeSoundSource);
}

SpecPpi8255Circuit::~SpecPpi8255Circuit()
{
    delete m_tapeSoundSource;
}


uint8_t SpecPpi8255Circuit::getPortA()
{
    return m_kbd->getHMatrixData() & 0xFF;
}



uint8_t SpecPpi8255Circuit::getPortB()
{
    return (m_kbd->getVMatrixData() << 2) | (m_kbd->getShift() ? 0 : 2) | (g_emulation->getWavReader()->getCurValue() ? 0x00 : 0x01);
}



uint8_t SpecPpi8255Circuit::getPortC()
{
    return (m_kbd->getHMatrixData() & 0xf00) >> 8;
}



void SpecPpi8255Circuit::setPortA(uint8_t value)
{
    if (m_portAInputMode)
        value |= 0xff;
    m_kbdMask &= ~0xFF;
    m_kbdMask |= value;
    m_kbd->setVMatrixMask(m_kbdMask);
    return;
}



void SpecPpi8255Circuit::setPortB(uint8_t value)
{
    if (m_portBInputMode)
        value = 0xff;
    m_kbd->setHMatrixMask((value & 0xFC) >> 2);
}



void SpecPpi8255Circuit::setPortC(uint8_t value)
{
    if (m_portCloInputMode)
        value |= 0xf; // в режиме ввода на выходах 1
    m_kbdMask &= ~0xF00;
    m_kbdMask |= (value & 0xf) << 8;;
    m_kbd->setVMatrixMask(m_kbdMask);
    m_tapeSoundSource->setValue(((value & 0x20) >> 5) + ((value & 0x80) >> 7));
    m_platform->getCore()->tapeOut(value & 0x80);
    if (m_videoRam)
        m_videoRam->setCurColor(value & 0xD0);
}


void SpecPpi8255Circuit::setPortCLoMode(bool isInput)
{
    m_portCloInputMode = isInput;
    if (isInput)
        m_kbdMask |= 0xF00; // в режиме ввода на выходах 1
}


void SpecPpi8255Circuit::setPortAMode(bool isInput)
{
    m_portAInputMode = isInput;
    if (isInput)
        m_kbdMask |= 0xFF; // в режиме ввода на выходах 1
}


void SpecPpi8255Circuit::setPortBMode(bool isInput)
{
    m_portBInputMode = isInput;
    if (isInput)
        m_kbd->setHMatrixMask(0x3f); // в режиме ввода на выходах 1
}


void SpecPpi8255Circuit::attachSpecKeyboard(SpecKeyboard* kbd)
{
    m_kbd = kbd;
}


void SpecPpi8255Circuit::attachVideoRam(SpecVideoRam* videoRam)
{
    m_videoRam = videoRam;
}


bool SpecPpi8255Circuit::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "specKeyboard") {
        attachSpecKeyboard(static_cast<SpecKeyboard*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "videoRam") {
        attachVideoRam(static_cast<SpecVideoRam*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


SpecKeyboard::SpecKeyboard()
{
    resetKeys();
}


void SpecKeyboard::resetKeys()
{
    for (int i = 0; i < 12; i++)
        m_vKeys[i] = 0;
    for (int i = 0; i < 6; i++)
        m_hKeys[i] = 0;
    m_shift = false;
}


void SpecKeyboard::processKey(EmuKey key, bool isPressed)
{
    if (key == EK_NONE)
        return;

    int i, j;
    bool isFound = false;

    // Основная матрица
    for (i = 0; i < 6; i++) {
        for (j = 0; j < 12; j++) {
            if ((m_mxMatrix ? m_keyMatrixMx[j][i] : m_keyMatrix[j][i]) == key) {
                isFound = true;
                break;
            }
        }
        if (isFound)
            break;
    }
    if (isFound) {
        if (isPressed) {
            m_vKeys[j] |= (1 << i);
            m_hKeys[i] |= (1 << j);
        } else {
            m_vKeys[j] &= ~(1 << i);
            m_hKeys[i] &= ~(1 << j);
        }
    return;
    }

    // Управляющие клавиши
    if (key == EK_SHIFT)
        m_shift = isPressed;
}



void SpecKeyboard::setVMatrixMask(uint16_t mask)
{
    m_vMask = ~mask;
}



uint8_t SpecKeyboard::getVMatrixData()
{
    uint8_t val = 0;
    uint16_t mask = m_vMask;
    for (int i = 0; i < 12; i++) {
        if (mask & 1)
            val |= m_vKeys[i];
        mask >>= 1;
    }
    return ~val;
}


void SpecKeyboard::setHMatrixMask(uint8_t mask)
{
    m_hMask = ~mask;
}


uint16_t SpecKeyboard::getHMatrixData()
{
    uint16_t val = 0;
    uint8_t mask = m_hMask;
    for (int i = 0; i < 6; i++) {
        if (mask & 1)
            val |= m_hKeys[i];
        mask >>= 1;
    }
    return ~val;
}


bool SpecKeyboard::getShift()
{
    return m_shift;
}


bool SpecKeyboard::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "matrix") {
        if (values[0].asString() == "original") {
            m_mxMatrix = false;
            return true;
        } else if (values[0].asString() == "ramfos") {
            m_mxMatrix = true;
            return true;
        } else
            return false;
    }
    return false;
}


string SpecKeyboard::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "matrix")
        return m_mxMatrix ? "ramfos" : "original";

    return "";
}


bool SpecFileLoader::loadFile(const std::string& fileName, bool run)
{
    int fileSize;
    uint8_t* buf = palReadFile(fileName, fileSize, false);
    if (!buf)
        return false;

    if (fileSize < 7) {
        delete[] buf;
        return false;
    }

    uint8_t* ptr = buf;

    if ((*ptr) == 0xE6) {
        ptr++;
        fileSize--;
    }

    if (ptr[0] == 0xD9 && ptr[1] == 0xD9 && ptr[2] == 0xD9) {
        // Named file
        ptr += 3;
        fileSize -= 3;
        while ((*ptr) != 0xE6 && fileSize > 0) {
            ++ptr;
            --fileSize;
        }
        ++ptr;
        --fileSize;
        if (fileSize < 7) {
            delete[] buf;
            return false;
        }
    }

    uint16_t begAddr = (ptr[1] << 8) | ptr[0];
    uint16_t endAddr = (ptr[3] << 8) | ptr[2];
    ptr += 4;
    fileSize -= 4;

    uint16_t progLen = endAddr - begAddr + 1;

    if (begAddr == 0xE6E6 || begAddr == 0xD3D3 || fileSize < progLen/* + 2*/) {
        // Basic or EDM File
        delete[] buf;
        return false;
    }

    for (uint16_t addr = begAddr; addr <= endAddr; addr++)
        m_as->writeByte(addr, *ptr++);

    if (fileSize < progLen + 2)
        emuLog << "Warning: no checksum in file " << fileName << "\n";

    if (run) {
        m_platform->reset();
        Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(m_platform->getCpu());
        if (cpu) {
            g_emulation->exec(cpu->getKDiv() * 2000000);
            cpu->setPC(begAddr);
        }
    }

    return true;
}


bool SpecMxFileLoader::loadFile(const std::string& fileName, bool run)
{
    const char* romDiskFileName = "PROGRAM  EXE";

    int fileSize;
    uint8_t* buf = palReadFile(fileName, fileSize, false);
    if (!buf)
        return false;

    string s((const char*)buf, fileSize);
    delete[] buf;
    istringstream* inputStream = new istringstream(s);

    uint16_t begAddr, runAddr;
    uint16_t monBegAddr = 0;
    string monFileName;
    *inputStream >> hex >> begAddr;
    *inputStream >> hex >> runAddr;
    *inputStream >> monFileName;

    delete inputStream;

    string::size_type dotPos = fileName.find_last_of(".");
    if (dotPos == string::npos)
        return false;

    string binFileName = fileName.substr(0, dotPos);
    binFileName += string(".i80");

    buf = palReadFile(binFileName, fileSize, false);
    if (!buf)
        return false;

    uint16_t endAddr = begAddr + fileSize - 1;

    if (!run) {
        uint16_t addr = 0;
        uint8_t* ptr = buf;

        // Сигнатура (D3D3D3)
        for (int i = 0; i < 3; i++)
            m_ramDisk->writeByte(addr++, 0xd3);

        // Имя файла (PROGRAM.EXE)
        for (int i = 0; i < 12; i++)
            m_ramDisk->writeByte(addr++, romDiskFileName[i]);

        // Признак сохраненности
        m_ramDisk->writeByte(addr++, 0x8c);

        // Дата файла, пусть будет 01.01.00
        m_ramDisk->writeByte(addr++, 0x01);
        m_ramDisk->writeByte(addr++, 0x01);
        m_ramDisk->writeByte(addr++, 0x00);

        // Резерв (5 x 0x00)
        for (int i = 0; i < 5; i++)
            m_ramDisk->writeByte(addr++, 0x00);

        // Начальный адрес
        m_ramDisk->writeByte(addr++, begAddr & 0xFF);
        m_ramDisk->writeByte(addr++, (begAddr & 0xFF00) >> 8);

        // Конечный адрес
        m_ramDisk->writeByte(addr++, endAddr & 0xFF);
        m_ramDisk->writeByte(addr++, (endAddr & 0xFF00) >> 8);

        uint16_t cs = 0;
        for (uint16_t i = 0; i < fileSize - 1; i++) {
            cs += buf[i];
            cs += (buf[i] << 8);
        }
        cs = (cs & 0xff00) | ((cs + buf[fileSize - 1]) & 0xff);

        // Контрольная сумма (пока 0x0000)
        m_ramDisk->writeByte(addr++, cs & 0xFF);
        m_ramDisk->writeByte(addr++, (cs & 0xFF00) >> 8);

        for (uint16_t i = begAddr; i <= endAddr; i++)
            m_ramDisk->writeByte(addr++, *ptr++);

        // Указатель на начало файла - ???
        m_ramDisk->writeByte(addr++, begAddr & 0xFF);
        m_ramDisk->writeByte(addr++, (begAddr & 0xFF00) >> 8);

        // Еще один доп. байт, чтобы не попасть случайно на сигнатуру
        if (addr != 0)
            m_ramDisk->writeByte(addr++, 0x0);
    }
    //delete[] buf;

    if (monFileName != "") {
        monFileName = m_platform->getBaseDir() + monFileName;

        uint8_t* monBuf = palReadFile(monFileName, fileSize, true);
        if (!monBuf) {
            emuLog << "Warning: file " + monFileName + " has not been loaded\n";
            return true;
        }

        // выделяем имя файла без пути
        string::size_type slashPos = monFileName.find_last_of("\\/");
        if (slashPos != string::npos)
            monFileName = monFileName.substr(slashPos + 1);
        if (monFileName.size() < 7) {
            emuLog << "Warning: file " + monFileName + " is invalid Monitor name\n";
            delete[] buf;
            return true;
        }

        string sAddr = monFileName.substr(3, 4);
        if (sAddr.find_first_not_of("0123456789ABCDabcd") != string::npos) {
            //emuLog << "Warning: file " + monFileName + " is invalid Monitor name, ignoring\n";
            //delete[] buf;
            //return true;
            monBegAddr = 0;
        } else {
            istringstream iss(sAddr);
            iss >> hex >> monBegAddr;
        }

        uint8_t* ptr = monBuf;
        if (monBegAddr != 0) {
            //Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(m_platform->getCpu());
            m_pageMapper->setCurPage(1); // switch to RAM page
            //m_as->writeByte(0xFFFC, 0); // switch to RAM page
            for (uint16_t addr = monBegAddr; addr < monBegAddr + fileSize; addr++)
                m_as->writeByte(addr, *ptr++);
        }

        delete[] monBuf;
    }

    if (run) {
        m_platform->reset();
        Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(m_platform->getCpu());
        if (!cpu) return false; // на всякий случай
        if (monBegAddr != 0) {
            m_pageMapper->setCurPage(1); // switch to RAM page
            //m_as->writeByte(0xFFFC, 0); // switch to RAM page
            cpu->setPC(monBegAddr);
        }
        g_emulation->exec(cpu->getKDiv() * 2000000);
        m_pageMapper->setCurPage(1); // switch to RAM page
        //m_as->writeByte(0xFFFC, 0); // switch to RAM page

        uint8_t* ptr = buf;
        for (uint16_t i = begAddr; i <= endAddr; i++)
            m_as->writeByte(i, *ptr++);

        cpu->setPC(runAddr);
    }

    if (buf)
        delete[] buf;

    return true;
}


bool SpecMxFileLoader::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (FileLoader::setProperty(propertyName, values))
        return true;

    if (propertyName == "ramDiskAddrSpace") {
        m_ramDisk = static_cast<AddressableDevice*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "pageMapper") {
        m_pageMapper = static_cast<AddrSpaceMapper*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


SpecRomDisk::SpecRomDisk(string romDiskName)
{
    m_romDisk = new uint8_t[65536];
    memset(m_romDisk, 0xFF, 65536);
    palReadFromFile(romDiskName, 0, 65536, m_romDisk);
}


SpecRomDisk::~SpecRomDisk()
{
    delete[] m_romDisk;
}


uint8_t SpecRomDisk::getPortB()
{
    return m_romDisk[m_curAddr];
}


void SpecRomDisk::setPortA(uint8_t value)
{
    m_curAddr = (m_curAddr & ~0xff) | value;
}


void SpecRomDisk::setPortC(uint8_t value)
{
    m_curAddr = (m_curAddr & ~0xff00) | (value << 8);
}
