/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2019-2024
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
#include <algorithm>
#include <cstring>

#include "Globals.h"
#include "EmuCalls.h"
#include "Vector.h"
#include "EmuWindow.h"
#include "Cpu.h"
#include "Platform.h"
#include "Emulation.h"
#include "Memory.h"
#include "Fdc1793.h"
#include "SoundMixer.h"
#include "WavReader.h"
#include "Covox.h"
#include "PrnWriter.h"
#include "AtaDrive.h"

using namespace std;


void VectorAddrSpace::reset() {
    m_romEnabled = true;
    m_inRamPagesMask = 0;
    m_stackDiskEnabled = false;
    m_inRamDiskPage = 0;
    m_stackDiskPage = 0;
    m_inRamPagesMask2 = 0;
    m_stackDiskEnabled2 = false;
    m_inRamDiskPage2 = 0;
    m_stackDiskPage2 = 0;
    m_eramSegment = 0;
    m_eramPageStartAddr = 0xA000;
    m_eramPageEndAddr = 0xDFFF;
}


void VectorAddrSpace::writeByte(int addr, uint8_t value)
{
    if (m_eram) {
        // ERAM
        if (m_stackDiskEnabled && m_cpu->checkForStackOperation())
            m_ramDisk->writeByte(m_eramSegment * 0x40000 + m_stackDiskPage * 0x10000 + addr, value);
        else if (m_inRamPagesMask & 2 && addr >= m_eramPageStartAddr && addr <= m_eramPageEndAddr)
            m_ramDisk->writeByte(m_eramSegment * 0x40000 + m_inRamDiskPage * 0x10000 + addr, value);
        else {
            if (addr >= 0x8000 && m_crtRenderer)
                m_crtRenderer->vidMemWriteNotify();
            m_mainMemory->writeByte(addr, value);
        }
        return;
    }

    // Barkar
    if (m_stackDiskEnabled && m_cpu->checkForStackOperation())
        m_ramDisk->writeByte(m_stackDiskPage * 0x10000 + addr, value);
    else if (m_stackDiskEnabled2 && m_cpu->checkForStackOperation())
        m_ramDisk2->writeByte(m_stackDiskPage2 * 0x10000 + addr, value);
    else if (m_inRamPagesMask && (addr >= 0x8000) && m_inRamPagesMask & (1 << ((addr & 0x6000) >> 13)))
        m_ramDisk->writeByte(m_inRamDiskPage * 0x10000 + addr, value);
    else if (m_inRamPagesMask2 && (addr >= 0x8000) && m_inRamPagesMask2 & (1 << ((addr & 0x6000) >> 13)))
        m_ramDisk2->writeByte(m_inRamDiskPage2 * 0x10000 + addr, value);
    else {
        if (addr >= 0x8000 && m_crtRenderer)
            m_crtRenderer->vidMemWriteNotify();
        m_mainMemory->writeByte(addr, value);
    }
}


uint8_t VectorAddrSpace::readByte(int addr)
{
    if (m_eram) {
        // ERAM
        if (m_stackDiskEnabled && m_cpu->checkForStackOperation())
            return m_ramDisk->readByte(m_eramSegment * 0x40000 + m_stackDiskPage * 0x10000 + addr);
        if (m_inRamPagesMask & 2 && addr >= m_eramPageStartAddr && addr <= m_eramPageEndAddr)
            return m_ramDisk->readByte(m_eramSegment * 0x40000 + m_inRamDiskPage * 0x10000 + addr);
        if (m_romEnabled && addr < m_rom->getSize())
            return m_rom->readByte(addr); // add rom check
        else
            return m_mainMemory->readByte(addr);
    }

    // Barkar
    if (m_stackDiskEnabled && m_cpu->checkForStackOperation())
        return m_ramDisk->readByte(m_stackDiskPage * 0x10000 + addr);
    if (m_stackDiskEnabled2 && m_cpu->checkForStackOperation())
        return m_ramDisk2->readByte(m_stackDiskPage2 * 0x10000 + addr);
    if (m_inRamPagesMask && (addr >= 0x8000) && m_inRamPagesMask & (1 << ((addr & 0x6000) >> 13)))
        return m_ramDisk->readByte(m_inRamDiskPage * 0x10000 + addr);
    if (m_inRamPagesMask2 && (addr >= 0x8000) && m_inRamPagesMask2 & (1 << ((addr & 0x6000) >> 13)))
        return m_ramDisk2->readByte(m_inRamDiskPage2 * 0x10000 + addr);
    if (m_romEnabled && addr < m_rom->getSize())
        return m_rom->readByte(addr); // add rom check
    else
        return m_mainMemory->readByte(addr);
}


void VectorAddrSpace::attachRamDisk(int diskNum, AddressableDevice* ramDisk)
{
    if (diskNum == 0)
        m_ramDisk = ramDisk;
    else // if (diskNum == 1)
        m_ramDisk2 = ramDisk;
}

void VectorAddrSpace::ramDiskControl(int diskNum, int inRamPagesMask, bool stackEnabled, int inRamPage, int stackPage)
{
    if (diskNum == 0) {
        m_inRamPagesMask = inRamPagesMask;
        m_stackDiskEnabled = stackEnabled;
        m_inRamDiskPage = inRamPage;
        m_stackDiskPage = stackPage;
    } else { // if (diskNum == 1)
        m_inRamPagesMask2 = inRamPagesMask;
        m_stackDiskEnabled2 = stackEnabled;
        m_inRamDiskPage2 = inRamPage;
        m_stackDiskPage2 = stackPage;
    }
}


void VectorAddrSpace::eramControl(int eramSegment, int eramPageStartAddr, int eramPageEndAddr)
{
    m_eramSegment = eramSegment;
    m_eramPageStartAddr = eramPageStartAddr;
    m_eramPageEndAddr = eramPageEndAddr;
}


bool VectorAddrSpace::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "ram") {
        attachRam(static_cast<AddressableDevice*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "rom") {
        attachRom(static_cast<Rom*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "ramDisk") {
        int diskNum = values[1].asInt();
        if (diskNum == 0 || diskNum == 1) {
            attachRamDisk(diskNum, static_cast<AddressableDevice*>(g_emulation->findObject(values[0].asString())));
            return true;
        }
        return false;
    } else  if (propertyName == "cpu") {
        m_cpu = static_cast<Cpu8080Compatible*>(g_emulation->findObject(values[0].asString()));
        return m_cpu;
    } else if (propertyName == "crtRenderer") {
            attachCrtRenderer(static_cast<VectorRenderer*>(g_emulation->findObject(values[0].asString())));
            return true;
    } else if (propertyName == "eram") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_eram = values[0].asString() == "yes";
            return true;
        }
    }
    return false;

}


string VectorAddrSpace::getDebugInfo()
{
    stringstream ss;

    if (!m_eram) {
        if (m_stackDiskEnabled || m_inRamPagesMask != 0) {
            ss << "ED1:";
            for (int i = 0; i < 4; i++)
                ss << ((m_inRamPagesMask & (1 << i)) ? "+" : "-");
            ss << "/" << m_inRamDiskPage;
            ss << " S";
            if (m_stackDiskEnabled)
                ss << m_stackDiskPage;
            else
                ss << "-";
            //ss << "\n";
        }

        if (m_stackDiskEnabled2 || m_inRamPagesMask2 != 0) {
            ss << "ED2:";
            for (int i = 0; i < 4; i++)
                ss << ((m_inRamPagesMask2 & (1 << i)) ? "+" : "-");
            ss << "/" << m_inRamDiskPage2;
            ss << " S";
            if (m_stackDiskEnabled2)
                ss << m_stackDiskPage2;
            else
                ss << "-";
            //ss << "\n";
        }
    }

    return ss.str();
}


VectorCore::VectorCore()
{
    // ...
}


void VectorCore::reset()
{
    m_intReq = false;
    m_intsEnabled = false;
}


void VectorCore::draw()
{
    m_window->drawFrame(m_crtRenderer->getPixelData());
    m_window->endDraw();
}


void VectorCore::inte(bool isActive)
{
    m_intsEnabled = isActive;
    if (!isActive)
        m_intReq = false;
    else if (m_intReq) {
        Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_platform->getCpu());
        if (cpu->getInte()) {
            cpu->intRst(7);
            cpu->hrq(cpu->getKDiv() * 5); // add waits to RST
        }
    }
}


void VectorCore::vrtc(bool isActive)
{
    if (isActive)
        g_emulation->screenUpdateReq();

    if (isActive && m_intsEnabled) {
        m_intReq = true;
        Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_platform->getCpu());
        if (cpu->getInte()) {
            cpu->intRst(7);
            cpu->hrq(cpu->getKDiv() * 5); // add waits to RST
        }
    }
}


void VectorCore::attachCrtRenderer(VectorRenderer* crtRenderer)
{
    m_crtRenderer = crtRenderer;
}


bool VectorCore::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (PlatformCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<VectorRenderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }
    return false;
}


VectorRenderer::VectorRenderer()
{
    const int pixelFreq = 12; // MHz
    const int maxBufSize = 626 * 288; // 626 = 704 / 13.5 * pixelFreq

    m_sizeX = m_prevSizeX = 512;
    m_sizeY = m_prevSizeY = 256;
    m_aspectRatio = m_prevAspectRatio = 5184. / 704 / pixelFreq;
    m_bufSize = m_prevBufSize = m_sizeX * m_sizeY;
    m_pixelData = new uint32_t[maxBufSize];
    m_prevPixelData = new uint32_t[maxBufSize];
    memset(m_pixelData, 0, m_bufSize * sizeof(uint32_t));
    memset(m_prevPixelData, 0, m_prevBufSize * sizeof(uint32_t));

    m_ticksPerPixel = g_emulation->getFrequency() / 12000000;
    m_curScanlineClock = m_curClock;

    m_curFramePixel = 0;
    m_curFrameClock = m_curClock;

    m_frameBuf = new uint32_t[maxBufSize];

    memset(m_colorPalette, 0, sizeof(uint32_t) * 16);
    memset(m_bwPalette, 0, sizeof(uint32_t) * 16);
    m_palette = m_colorPalette;

    prepareFrame(); // prepare 1st frame dimensions
}


VectorRenderer::~VectorRenderer()
{
    delete[] m_frameBuf;
}


void VectorRenderer::operate()
{
    advanceTo(m_curClock);
    m_curFrameClock = m_curClock;
    m_curFramePixel = 0;
    m_curClock += m_ticksPerPixel * 768 * 312;
    m_lineOffsetIsLatched = false;
    renderFrame();
    m_platform->getCore()->vrtc(true);
    m_mode512pxLatched = m_mode512px;
    m_lastColor = 0;
}


void VectorRenderer::advanceTo(uint64_t clock)
{
    const int bias = 189;

    if (clock <= m_curFrameClock)
        return;

    int toPixel = (int64_t(clock) - int64_t(m_curFrameClock)) / m_ticksPerPixel + bias;

    if (toPixel <= m_curFramePixel) {
        if (toPixel < 40 * 768 || toPixel >= 296 * 768)
            m_lastColor = m_borderColor;
        return;
    }

    if (toPixel >= 312 * 768)
        toPixel = 312 * 768 - 1;

    if (!m_lineOffsetIsLatched && toPixel > 768 * 40 + 180) {
        m_lineOffsetIsLatched = true;
        m_latchedLineOffset = m_lineOffset;
    }

    int firstLine = m_curFramePixel / 768;
    int firstPixel = m_curFramePixel % 768;
    int lastLine = toPixel / 768;
    int lastPixel = toPixel % 768;
    m_curFramePixel = toPixel;
    renderLine(firstLine, firstPixel, firstLine == lastLine ? lastPixel : 768);
    for (int line = firstLine + 1; line < lastLine; line++)
        renderLine(line, 0, 768);
    if (firstLine != lastLine)
        renderLine(lastLine, 0, lastPixel);
}


void VectorRenderer::setBorderColor(uint8_t color)
{
    advanceTo(g_emulation->getCurClock() + m_ticksPerPixel * 48);
    m_borderColor = color;
}


void VectorRenderer::set512pxMode(bool mode512)
{
    advanceTo(g_emulation->getCurClock() + m_ticksPerPixel * 34);
    m_mode512px = mode512;
}


void VectorRenderer::setLineOffset(uint8_t lineOffset)
{
    advanceTo(g_emulation->getCurClock() + m_ticksPerPixel * 48);
    m_lineOffset = lineOffset;
}


void VectorRenderer::setPaletteColor(uint8_t color)
{
    advanceTo(g_emulation->getCurClock() + m_ticksPerPixel * 27);

    m_colorPalette[m_lastColor] = ((color & 0x7) << 21) | ((color & 0x7) << 18) | ((color & 0x6) << 15) |
                                  ((color & 0x38) << 10) | ((color & 0x38) << 7) | ((color & 0x30) << 4) |
                                  (color & 0xC0) | ((color & 0xC0) >> 2) | ((color & 0xC0) >> 4) | ((color & 0xC0) >> 6);
    uint8_t bw = c_bwMap[color];
    m_bwPalette[m_lastColor] = (bw << 16) | (bw << 8) | bw;
}


void VectorRenderer::vidMemWriteNotify()
{
    advanceTo(g_emulation->getCurClock() + m_ticksPerPixel * 40);
}


void VectorRenderer::renderLine(int nLine, int firstPx, int lastPx)
{
    // Render scan line #nLine
    // Vertical: 0-22 - invisible, 23-39 - border, 40-295 - visible, 296-311 - border) from firstPx to lastPx
    // Horizonlal: 0-123 - invisible, 124-180 - border, 181-692 - active area, 693-749 - border, 750-767 - invisible
    // (lastPx is non-inclusive)

    if (nLine < 24) {
        m_lastColor = m_borderColor;
        return;
    }

    uint32_t* linePtr = m_frameBuf + (nLine - 24) * 626;
    uint32_t* ptr;

    if (nLine < 40 || nLine >= 296) {
        // upper and lower borders
        if (firstPx < 124) firstPx = 124;
        ptr = linePtr + firstPx - 124;
        m_lastColor = m_borderColor;
        for (int px = firstPx; px < lastPx && px < 750; px++) {
            *ptr++ = m_palette[m_mode512px ? (px & 1 ? m_borderColor & 0x0c : m_borderColor & 0x03) : m_borderColor];
        }
    } else {
        // left border
        if (firstPx < 124) firstPx = 124;
        ptr = linePtr + firstPx - 124;
        for (int px = firstPx; px < lastPx && px < 181; px++)
            *ptr++ = m_palette[m_mode512px ? (px & 1 ? m_borderColor & 0x0c : m_borderColor & 0x03) : m_borderColor];

        // active area
        if (firstPx < 181) firstPx = 181;
        ptr = linePtr + firstPx - 124;
        uint8_t rollOff = uint8_t(m_latchedLineOffset - nLine + 40);
        for (int px = firstPx - 181; px < lastPx - 181 && px < 693 - 181; px++) {
            int dot = (px & 0x0E) >> 1;
            int offset = ((px & 0x1F0) << 4) | rollOff;
            uint8_t btY = m_screenMemory[0x8000 + offset] << dot;
            uint8_t btR = m_screenMemory[0xA000 + offset] << dot;
            uint8_t btG = m_screenMemory[0xC000 + offset] << dot;
            uint8_t btB = m_screenMemory[0xE000 + offset] << dot;
            int logBGcolor = ((btG & 0x80) >> 6) | ((btB & 0x80) >> 7);
            int logYRcolor = ((btY & 0x80) >> 4) | ((btR & 0x80) >> 5);
            m_lastColor = logYRcolor | logBGcolor;
            if (m_mode512px) {
                *ptr++ = m_palette[px & 1 ? m_lastColor & 0x0c : m_lastColor & 0x03];
            } else {
                *ptr++ = m_palette[m_lastColor];
            }
        }

        // right border
        if (firstPx < 693) firstPx = 693;
        ptr = linePtr + firstPx - 124;
        for (int px = firstPx; px < lastPx && px < 750; px++)
            *ptr++ = m_palette[m_mode512px ? (px & 1 ? m_borderColor & 0x0c : m_borderColor & 0x03) : m_borderColor];

        if (lastPx < 182 || lastPx >= 694)
            m_lastColor = m_borderColor;
    }
}


void VectorRenderer::renderFrame()
{
    if (m_showBorder)
        memcpy(m_pixelData, m_frameBuf, m_sizeX * m_sizeY * sizeof(uint32_t));
    else {
        uint32_t* ptr = m_frameBuf + 626 * 16 + 57;
        for (int i = 0; i < 256 * 512; i += 512) {
            memcpy(m_pixelData + i, ptr, 512 * sizeof(uint32_t));
            ptr += 626;
        }
    }

    swapBuffers();
    prepareFrame();
}


void VectorRenderer::prepareFrame()
{
    if (!m_showBorder) {
        m_sizeX = 512;
        m_sizeY = 256;
        m_aspectRatio = 576.0 * 9 / 704 / 12;
    } else {
        m_sizeX = 626;
        m_sizeY = 288;
        m_aspectRatio = double(m_sizeY) * 4 / 3 / m_sizeX;
    }
}


void VectorRenderer::prepareDebugScreen()
{
    advanceTo(g_emulation->getCurClock());
    enableSwapBuffersOnce();
    renderFrame();
}


void VectorRenderer::setColorMode(bool colorMode)
{
    m_colorMode = colorMode;
    m_palette = m_colorMode ? m_colorPalette : m_bwPalette;
}


void VectorRenderer::toggleColorMode()
{
    setColorMode(!m_colorMode);
}


void VectorRenderer::toggleCropping()
{
    m_showBorder = !m_showBorder;
}


void VectorRenderer::attachMemory(Ram* memory)
{
    m_screenMemory = memory->getDataPtr();
}


bool VectorRenderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (CrtRenderer::setProperty(propertyName, values))
        return true;

    if (propertyName == "memory") {
        attachMemory(static_cast<Ram*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "visibleArea") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_showBorder = values[0].asString() == "yes";
            return true;
        }
    } else if (propertyName == "colorMode") {
        if (values[0].asString() == "mono")
            setColorMode(false);
        else if (values[0].asString() == "color")
            setColorMode(true);
        else
            return false;
        return true;
    }

    return false;
}


string VectorRenderer::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "visibleArea") {
        return m_showBorder ? "yes" : "no";
    } else if (propertyName == "colorMode") {
        return m_colorMode ? "color" : "mono";
    } else if (propertyName == "crtMode") {
        if (m_mode512pxLatched)
            return u8"512\u00D7256@50.08Hz";
        else
            return u8"256\u00D7256@50.08Hz";
        }

    return "";
}


string VectorRenderer::getDebugInfo()
{
    advanceTo(g_emulation->getCurClock());

    // some magic
    int pixel = (m_curFramePixel + 768 * 312 - 301) % (768 * 312);

    int line = pixel / 768;
    int pos = pixel % 768;

    stringstream ss;
    ss << "CRT:" << "\n";
    ss << "L:" << line;
    ss << " P:" << pos << "\n";
    return ss.str();
}


bool VectorFileLoader::loadFile(const std::string& fileName, bool run)
{
    auto periodPos = fileName.find_last_of(".");
    string ext = periodPos != string::npos ? fileName.substr(periodPos) : fileName;
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext == ".fdd") {
        if (!emuSetPropertyValue(m_platform->getName() + ".diskA", "fileName", fileName))
            return false;

        Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_platform->getCpu());
        static_cast<VectorAddrSpace*>(m_platform->getCpu()->getAddrSpace())->enableRom();
        static_cast<Cpu8080Compatible*>(m_platform->getCpu())->setPC(0);
        g_emulation->exec((int64_t)cpu->getKDiv() * 25000000, true);

        if (run) {
            m_platform->reset();
            static_cast<VectorAddrSpace*>(m_platform->getCpu()->getAddrSpace())->disableRom();
        }
        return true;
    }

    int fileSize;
    uint8_t* buf = palReadFile(fileName, fileSize, false);
    if (!buf)
        return false;

    /*if (fileSize > 32768 - 256) {  // review
        delete[] buf;
        return false;
    }*/

    bool basFile = false;

    uint8_t* ptr = buf;

    uint16_t begAddr = 0x100;

    // check for "r0m"
    if (fileName.size() >= 4) {
        string ext = fileName.substr(fileName.size() - 4, 4);
        if (ext == ".r0m" || ext == ".R0M")
            begAddr = 0;
        else if (ext == ".bas" || ext == ".BAS" || ext == ".cas" || ext == ".CAS")
            basFile = true;
    }

    Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_platform->getCpu());
    VectorAddrSpace* as = static_cast<VectorAddrSpace*>(cpu->getAddrSpace());
    m_platform->reset();
    as->enableRom();
    cpu->disableHooks();
    g_emulation->exec(int64_t(cpu->getKDiv()) * m_skipTicks, true);
    cpu->enableHooks();

    for (unsigned i = 0; i < 0x100; i++)
        m_as->writeByte(i, 0x00);

    if (!basFile)
        for (int i = 0; i < fileSize; i++) {
            uint16_t addr = begAddr + i;
            m_as->writeByte(addr, *ptr++);
            if (!run && (addr & 0xFF) == 0) {
                // paint block
                int block = addr >> 8;
                uint16_t blockAddr = 0xC018 + (block % 32) * 0x100 + (block / 32) * 0x18;
                for (int i = 0; i < 8; i++)
                    m_as->writeByte(blockAddr + i, 0x7E);
            }
        }
    else {
        // check for CAS
        if (fileSize >= 14 && ptr[0] == 0xD3 && ptr[1] == 0xD3 && ptr[2] == 0xD3 && ptr[3] == 0xD3) {
            // Cas file
            while (fileSize && *ptr != 0xE6) {
                ptr++;
                fileSize--;
            }

            if (fileSize < 7) {
                delete[] buf;
                return false;
            }

            ptr += 5;
            fileSize -= 5;
        }

        for (int i = 0; i < 0x39c6; i++)
            m_as->writeByte(0x0100 + i, as->readByte(0x08C5 + i));
        as->disableRom();
        cpu->setPC(begAddr);
        cpu->setIFF(false);
        cpu->disableHooks();
        g_emulation->exec(int64_t(cpu->getKDiv()) * 4000000, true);
        cpu->enableHooks();
        m_as->writeByte(0x4300, 0);

        uint16_t addr, nextAddr;
        addr = nextAddr = 0x4301;
        for(;;) {
            if (addr == nextAddr + 1)
                nextAddr = (ptr[0] << 8) | ptr[-1];
            m_as->writeByte(addr++, *ptr++);
            fileSize--;
            if (nextAddr == 0 || fileSize == 0 || addr >= 0x7EFF)
                break;
        }
        m_as->writeByte(0x4045, addr & 0xFF);
        m_as->writeByte(0x4046, addr >> 8);
        m_as->writeByte(0x4047, addr & 0xFF);
        m_as->writeByte(0x4048, addr >> 8);
        m_as->writeByte(0x4049, addr & 0xFF);
        m_as->writeByte(0x404A, addr >> 8);
        delete[] buf;

        if (run) {
            m_as->writeByte(0x3DBF, 'R');
            m_as->writeByte(0x3DC0, 'U');
            m_as->writeByte(0x3DC1, 'N');
            m_as->writeByte(0x3DC2, '\r');
            m_as->writeByte(0x3DB8, 4);
            m_as->writeByte(0x3DB9, 4);
            m_as->writeByte(0x3DBA, 0);
        }

        return true;
    }


    delete[] buf;

    if (run) {
        as->disableRom();
        cpu->setPC(begAddr);
        cpu->setIFF(false);
    } else {
        as->enableRom();
        cpu->setPC(0xDF);
        //cpu->setSP(0xDCF0);
    }

    return true;
}


// Port 01
void VectorPpi8255Circuit::setPortC(uint8_t value)
{
    m_tapeSoundSource->setValue(value & 1);
    m_platform->getCore()->tapeOut(value & 1);
}


// Port 02
void VectorPpi8255Circuit::setPortB(uint8_t value)
{
    // order is important!
    m_renderer->set512pxMode(value & 0x10);
    m_renderer->setBorderColor(value & 0x0f);
}


// Port 03
void VectorPpi8255Circuit::setPortA(uint8_t value)
{
    m_renderer->setLineOffset(value);
    m_kbd->setMatrixMask(value);
}


uint8_t VectorPpi8255Circuit::getPortB()
{
    return m_kbd->getMatrixData();
}


uint8_t VectorPpi8255Circuit::getPortC()
{
    return (m_kbd->getCtrlKeys() & 0xEF) | (g_emulation->getWavReader()->getCurValue() ? 0x10 : 0x00);
}


bool VectorPpi8255Circuit::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (Ppi8255Circuit::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachRenderer(static_cast<VectorRenderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "keyboard") {
        attachKeyboard(static_cast<VectorKeyboard*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "tapeSoundSource") {
        m_tapeSoundSource = static_cast<GeneralSoundSource*>(g_emulation->findObject(values[0].asString()));
        return true;
    }
    return false;
}


void VectorColorRegister::writeByte(int, uint8_t value)
{
    m_renderer->setPaletteColor(value);
}


bool VectorColorRegister::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachRenderer(static_cast<VectorRenderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }
    return false;
}


VectorKeyboard::VectorKeyboard()
{
    VectorKeyboard::resetKeys();
}


void VectorKeyboard::resetKeys()
{
    for (int i = 0; i < 8; i++)
        m_keys[i] = 0;
    m_mask = 0;
    m_ctrlKeys = 0;
}


void VectorKeyboard::processKey(EmuKey key, bool isPressed)
{
    if (key == EK_NONE)
        return;

    int i, j;

    // Основная матрица
    for (i = 0; i < 8; i++)
        for (j = 0; j < 8; j++)
            if (key == m_keyMatrix[i][j])
                goto found;

    // Управляющие клавиши
    for (i = 0; i < 8; i++)
        if (m_ctrlKeyMatrix[i] == key) {
            if (isPressed)
                m_ctrlKeys |= (1 << i);
            else
                m_ctrlKeys &= ~(1 << i);
        }
    return;

found:
    if (isPressed)
        m_keys[i] |= (1 << j);
    else
        m_keys[i] &= ~(1 << j);
}


uint8_t VectorKeyboard::getMatrixData()
{
    uint8_t val = 0;
    uint8_t mask = m_mask;
    for (int i=0; i<8; i++) {
        if (mask & 1)
            val |= m_keys[i];
        mask >>= 1;
    }
    return ~val;
}


int VectorCpuWaits::getCpuWaitStates(int, int, int normalClocks)
{
    static const int waits[19] = {0, 0, 0, 0, 0, 3, 0, 1, 0, 0, 2, 5, 0, 3, 0, 0, 4, 7, 6};
    return waits[normalClocks];
}


int VectorZ80CpuWaits::getCpuWaitStates(int, int opcode, int normalClocks)
{
    static const int waits[24] = {0, 0, 0, 0, 0, 3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 4, 3, 2, 1, 4, 3, 0, 5};
    // 8, 11, 12, 13, 15 should be revised
    switch (normalClocks) {
    case 8:
        if ((opcode & 0xFF) == 0x10) // DJNZ, if B==0
            return 4;
        break;
    case 11:
        if ((opcode & 0xCF) == 0xC5 ||   // PUSH qq
            (opcode & 0xC7) == 0xC0 ||   // RET cc if cc = true
            (opcode & 0xC7) == 0xC7)     // RST p
            return 5;
        break;
    case 14:
        if ((opcode & 0xFFDF) == 0xE1DD) // POP ix/iy
            return 6;
        break;
    case 15:
        if ((opcode & 0xFFDF) == 0xE5DD) // PUSH ix/iy
            return 5;
        break;
    case 19:
        if ((opcode & 0xFF) == 0xE3 ||   // EX (SP),HL
            (opcode & 0xFFDF) == 0x36DD) // LD (ix+d),n
            return 5;
        break;
    case 23:
        if ((opcode & 0xFEDF) == 0x34DD) // INC/DEC (ix/iy+d)
            return 1;
        break;
    default:
        break;
    }
    return waits[normalClocks];
}


bool VectorKbdLayout::processSpecialKeys(PalKeyCode keyCode)
{
    if (keyCode == PK_F11) {
        //m_platform->getKeyboard()->disableKeysReset();
        //m_platform->reset();
        //m_platform->getKeyboard()->enableKeysReset();
        static_cast<VectorAddrSpace*>(m_platform->getCpu()->getAddrSpace())->enableRom();
        static_cast<Cpu8080Compatible*>(m_platform->getCpu())->setPC(0);
        return true;
    } else if (keyCode == PK_F12) {
        m_platform->getKeyboard()->disableKeysReset();
        m_platform->reset();
        m_platform->getKeyboard()->enableKeysReset();
        static_cast<VectorAddrSpace*>(m_platform->getCpu()->getAddrSpace())->disableRom();
        //static_cast<Cpu8080Compatible*>(m_platform->getCpu())->setPC(0);
        return true;
    }
    return false;
}


bool VectorRamDiskSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "addrSpace") {
        attachVectorAddrSpace(static_cast<VectorAddrSpace*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "diskNum") {
        int diskNum = values[0].asInt();
        if (diskNum == 0 || diskNum == 1) {
            m_diskNum = diskNum;
            return true;
        }
    }

    return false;
}


void VectorRamDiskSelector::writeByte(int, uint8_t value)
{
    if (m_vectorAddrSpace)
        m_vectorAddrSpace->ramDiskControl(m_diskNum, ((value & 0x40) >> 6) | ((value & 0x20) >> 4) | ((value & 0x20) >> 3) | ((value & 0x80) >> 4), value & 0x10, value & 0x3, (value >> 2) & 0x3);
}


bool VectorEramSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "addrSpace") {
        attachVectorAddrSpace(static_cast<VectorAddrSpace*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


void VectorEramSelector::writeByte(int, uint8_t value)
{
    int start, end, segment;
    if (value & 4) {
        start = 0x0000;
        end = 0xFFFF;
    } else switch (value & 3) {
    case 0:
        start = 0xA000;
        end = 0xDFFF;
        break;
    case 1:
        start = 0x8000;
        end = 0xDFFF;
        break;
    case 2:
        start = 0x8000;
        end = 0xFFFF;
        break;
    default: // case 3
        start = 0x0100;
        end = 0x7FFF;
    }
    segment = (value >> 3) & 7;

    if (m_vectorAddrSpace)
        m_vectorAddrSpace->eramControl(segment, start, end);
}


void VectorFddControlRegister::writeByte(int, uint8_t value)
{
    m_fdc->setDrive(value & 1);
    m_fdc->setHead(((value & 0x4) >> 2) ^ 1);
}


bool VectorFddControlRegister::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "fdc") {
        attachFdc1793(static_cast<Fdc1793*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


bool VectorPpi8255Circuit2::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (Ppi8255Circuit::setProperty(propertyName, values))
        return true;

    if (propertyName == "covox") {
        attachCovox(static_cast<Covox*>(g_emulation->findObject(values[0].asString())));
        return true;
    }
    return false;
}


void VectorPpi8255Circuit2::setPortA(uint8_t value)
{
    if (m_covox) {
        m_covox->setValue(value >> 1);
    }

    m_printerData = value;
}


void VectorPpi8255Circuit2::setPortC(uint8_t value)
{
    bool newStrobe = value & 0x10;
    if (m_printerStrobe && !newStrobe) {
        g_emulation->getPrnWriter()->printByte(m_printerData);
    }
    m_printerStrobe = newStrobe;
}


bool VectorHddRegisters::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "ataDrive") {
        attachAtaDrive(static_cast<AtaDrive*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


void VectorHddRegisters::writeByte(int addr, uint8_t value)
{
    if (!m_ataDrive)
        return;

    if (addr == 8) {
        m_highW = value;
        return;
    }

    addr &= 7;

    if (addr == 0)
        m_ataDrive->writeReg(addr, value | m_highW << 8);
    else
        m_ataDrive->writeReg(addr, value);
}


uint8_t VectorHddRegisters::readByte(int addr)
{
    if (!m_ataDrive)
        return 0xFF;

    if (addr == 8)
        return m_highR;

    addr &= 7;

    uint16_t read = m_ataDrive->readReg(addr);
    if (addr == 0)
        m_highR = read >> 8;
    return read & 0x00FF;
}
