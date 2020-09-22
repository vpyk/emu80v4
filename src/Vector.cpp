﻿/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2019-2020
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

using namespace std;


void VectorAddrSpace::reset() {
    m_romEnabled = true;
    m_inRamPagesMask = 0;
    m_stackDiskEnabled = false;
    m_inRamDiskPage = 0;
    m_stackDiskPage = 0;
}


void VectorAddrSpace::writeByte(int addr, uint8_t value)
{
    if (m_stackDiskEnabled && m_cpu->checkForStackOperation())
        m_ramDisk->writeByte(m_stackDiskPage * 0x10000 + addr, value);
    else if (m_inRamPagesMask && (addr >= 0x8000) && m_inRamPagesMask & (1 << ((addr & 0x6000) >> 13)))
        m_ramDisk->writeByte(m_inRamDiskPage * 0x10000 + addr, value);
    else {
        if (addr >= 0x8000 && m_crtRenderer)
            m_crtRenderer->vidMemWriteNotify();
        m_mainMemory->writeByte(addr, value);
    }
}


uint8_t VectorAddrSpace::readByte(int addr)
{
    if (m_stackDiskEnabled && m_cpu->checkForStackOperation())
        return m_ramDisk->readByte(m_stackDiskPage * 0x10000 + addr);
    if (m_inRamPagesMask && (addr >= 0x8000) && m_inRamPagesMask & (1 << ((addr & 0x6000) >> 13)))
        return m_ramDisk->readByte(m_inRamDiskPage * 0x10000 + addr);
    if (m_romEnabled && addr < 0x8000)
        return m_rom->readByte(addr); // add rom check
    else
        return m_mainMemory->readByte(addr);
}


void VectorAddrSpace::ramDiskControl(int inRamPagesMask, bool stackEnabled, int inRamPage, int stackPage)
{
    m_inRamPagesMask = inRamPagesMask;
    m_stackDiskEnabled = stackEnabled;
    m_inRamDiskPage = inRamPage;
    m_stackDiskPage = stackPage;
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
        attachRamDisk(static_cast<AddressableDevice*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else  if (propertyName == "cpu") {
        m_cpu = static_cast<Cpu8080Compatible*>(g_emulation->findObject(values[0].asString()));
        return m_cpu;
    } else if (propertyName == "crtRenderer") {
            attachCrtRenderer(static_cast<VectorRenderer*>(g_emulation->findObject(values[0].asString())));
            return true;
    }
    return false;

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
    if (g_emulation->isDebuggerActive())
        m_crtRenderer->prepareDebugScreen();

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
            cpu->hrq(cpu->getKDiv() * 16); // add waits to RST
        }
    }
}


void VectorCore::vrtc(bool isActive)
{
    if (isActive && m_intsEnabled) {
        m_intReq = true;
        Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_platform->getCpu());
        if (cpu->getInte()) {
            cpu->intRst(7);
            cpu->hrq(cpu->getKDiv() * 16); // add waits to RST
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
    m_curScanlineClock = m_curClock;

    m_curFramePixel = 0;
    m_curFrameClock = m_curClock;

    m_frameBuf = new uint32_t[maxBufSize];

    memset(m_palette, 0, sizeof(uint32_t) * 16);

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
    const int bias = 145;

    if (clock <= m_curFrameClock)
        return;

    int toPixel = (int64_t(clock) - int64_t(m_curFrameClock)) / m_ticksPerPixel + bias;

    if (toPixel <= m_curFramePixel)
        return;

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
        renderLine(lastLine, firstLine == lastLine ? firstPixel : 0, lastPixel);
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
    //advance();
    //m_palette[m_borderColor] = ((color & 0x7) << 21) | ((color & 0x38) << 10) | (color & 0xC0);
    m_palette[m_lastColor] = ((color & 0x7) << 21) | ((color & 0x7) << 18) | ((color & 0x6) << 15) |
                               ((color & 0x38) << 10) | ((color & 0x38) << 7) | ((color & 0x30) << 4) |
                               (color & 0xC0) | ((color & 0xC8) >> 2) | ((color & 0xC8) >> 4) | ((color & 0xC8) >> 6);
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
        for (int px = firstPx; px < lastPx && px >= 124 && px < 750; px++) {
            *ptr++ = m_palette[m_borderColor];
        }
    } else {
        // left border
        if (firstPx < 124) firstPx = 124;
        ptr = linePtr + firstPx - 124;
        for (int px = firstPx; px < lastPx && px >= 124 && px < 181; px++)
            *ptr++ = m_palette[m_borderColor];

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
            m_lastColor = px & 1 ? logYRcolor : logBGcolor;
            if (m_mode512px) {
                *ptr++ = m_palette[m_lastColor];
            } else {
                *ptr++ = m_palette[logBGcolor | logYRcolor];
            }
        }

        // right border
        if (firstPx < 693) firstPx = 693;
        ptr = linePtr + firstPx - 124;
        for (int px = firstPx; px < lastPx && px >= 693 && px < 750; px++)
            *ptr++ = m_palette[m_borderColor];

        if (lastPx < 182 || lastPx >= 694)
            m_lastColor = m_borderColor;
    }
}


void VectorRenderer::renderFrame()
{
    if (m_showBorder)
        memcpy(m_pixelData, m_frameBuf, m_sizeX * m_sizeY * sizeof(uint32_t));
    else {
        uint32_t* ptr = m_frameBuf + 626 * 24 + 57;
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
    renderFrame();
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
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "memory") {
        attachMemory(static_cast<Ram*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "visibleArea") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_showBorder = values[0].asString() == "yes";
            return true;
        }
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

    Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(m_platform->getCpu());
    VectorAddrSpace* as = static_cast<VectorAddrSpace*>(cpu->getAddrSpace());
    m_platform->reset();
    as->enableRom();
    cpu->disableHooks();
    g_emulation->exec(int64_t(cpu->getKDiv()) * m_skipTicks);
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
        g_emulation->exec(int64_t(cpu->getKDiv()) * 4000000);
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


VectorPpi8255Circuit::VectorPpi8255Circuit()
{
    m_tapeSoundSource = new GeneralSoundSource;
}


VectorPpi8255Circuit::~VectorPpi8255Circuit()
{
    delete m_tapeSoundSource;
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
    resetKeys();
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
    static const int waits[24] = {0, 0, 0, 0, 0, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 1, 4, 3, 2, 5, 4, 7, 0, 5};
    // 8, 11, 12, 13, 15 should be revised
    switch (normalClocks) {
    case 8:
        if ((opcode & 0xFF) == 0x10) // DJNZ, if B==0
            return 4;
        break;
    case 11:
        if ((opcode & 0xCF) == 0xC5 /* PUSH qq */ ||
            (opcode & 0xC7) == 0xC0 /* RET cc if cc = true */ ||
            (opcode & 0xC7) == 0xC7 /* RST p */)
            return 5;
        break;
    case 12:
        if ((opcode & 0xFF) == 0xCB || (opcode & 0xFF) == 0xEB || (opcode & 0xFF) == 0xED) // BIT b, (HL) or IN r,(C) or OUT (C),r
            return 0;
        break;
    case 13:
        if ((opcode & 0xFF) == 0x10) // DJNZ, if B<>0
            return 7;
        break;
    case 15:
        if (opcode == 0xE5DD || opcode == 0xE5FD) // PUSH ix/iy
            return 5;
        break;
    default:
        break;
    }
    return waits[normalClocks];
}


bool VectorKbdLayout::processSpecialKeys(PalKeyCode keyCode)
{
    if (keyCode == PK_F11) {
        m_platform->getKeyboard()->disableKeysReset();
        m_platform->reset();
        m_platform->getKeyboard()->enableKeysReset();
        return true;
    } else if (keyCode == PK_F12) {
        m_platform->getKeyboard()->disableKeysReset();
        m_platform->reset();
        m_platform->getKeyboard()->enableKeysReset();
        static_cast<VectorAddrSpace*>(m_platform->getCpu()->getAddrSpace())->disableRom();
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
    }

    return false;
}


void VectorRamDiskSelector::writeByte(int, uint8_t value)
{
    if (m_vectorAddrSpace)
        m_vectorAddrSpace->ramDiskControl(((value & 0x40) >> 6) | ((value & 0x20) >> 4) | ((value & 0x20) >> 3) | ((value & 0x80) >> 4), value & 0x10, value & 0x3, (value >> 2) & 0x3);
}


void VectorFddControlRegister::writeByte(int, uint8_t value)
{
    m_fdc->setDrive(value & 1);
    m_fdc->setHead(((value & 0x4) >> 2) ^ 1);
}


bool VectorFddControlRegister::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
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
}
