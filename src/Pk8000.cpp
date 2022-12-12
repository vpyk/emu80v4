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

#include <string.h>

#include "Pk8000.h"
#include "Emulation.h"
#include "Platform.h"
#include "Globals.h"
#include "EmuWindow.h"
#include "SoundMixer.h"
#include "Memory.h"
#include "AddrSpace.h"
#include "Cpu.h"
#include "Fdc1793.h"
#include "WavReader.h"
#include "TapeRedirector.h"
#include "PrnWriter.h"

using namespace std;


Pk8000Core::Pk8000Core()
{
    // ...
}



Pk8000Core::~Pk8000Core()
{
    // ...
}


void Pk8000Core::reset()
{
    m_intReq = false;
}


void Pk8000Core::draw()
{
    /*if (g_emulation->isDebuggerActive())
        m_crtRenderer->renderFrame();*/ // not possible with scanline cpu-sync
    m_window->drawFrame(m_crtRenderer->getPixelData());
    m_window->endDraw();
}


void Pk8000Core::inte(bool isActive)
{
    // actually this is not VTRC, but this proc is called onece per frame on interrupt
    Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_platform->getCpu());
    if (isActive && m_intReq && cpu->getInte()) {
        m_intReq = false;
        cpu->intRst(7);
        cpu->hrq(cpu->getKDiv() * 7); // syncronize interrupt with waits
    }
}


void Pk8000Core::vrtc(bool isActive)
{
    if (isActive) {
        Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_platform->getCpu());
        m_intReq = true;
        if (cpu->getInte()) {
            m_intReq = false;
            cpu->intRst(7);
            // add waits to RST and consider one character offset (wide border 21-3-11=7, narrow border 20-4-11=5)
            cpu->hrq(cpu->getKDiv() * (m_crtRenderer->isBorderWide() ? 7 : 5)); // syncronize interrupt with waits
        }
    }
}


void Pk8000Core::attachCrtRenderer(Pk8000Renderer* crtRenderer)
{
    m_crtRenderer = crtRenderer;
}


bool Pk8000Core::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (PlatformCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<Pk8000Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }
    return false;
}


Pk8000Renderer::Pk8000Renderer()
{
    for (int i = 0; i < 4; i++) {
        m_screenMemoryBanks[i] = nullptr;
        m_screenMemoryRamBanks[i] = nullptr;
    }

    memset(m_colorRegs, 0, 32);

    const int pixelFreq = 5; // MHz
    const int maxBufSize = 261 * 288; // 261 = 704 / 13.5 * pixelFreq

    m_sizeX = m_prevSizeX = 256;
    m_sizeY = m_prevSizeY = 192;
    m_aspectRatio = m_prevAspectRatio = 5184. / 704 / pixelFreq;
    m_bufSize = m_prevBufSize = m_sizeX * m_sizeY;
    m_pixelData = new uint32_t[maxBufSize];
    m_prevPixelData = new uint32_t[maxBufSize];
    memset(m_pixelData, 0, m_bufSize * sizeof(uint32_t));
    memset(m_prevPixelData, 0, m_prevBufSize * sizeof(uint32_t));
    m_ticksPerPixel = g_emulation->getFrequency() / 5000000;
    setMode(0);

    m_palette = c_pk8000ColorPalette;

    m_curScanlineClock = m_curClock;
    memset(m_bgScanlinePixels, 0, 320 * sizeof(uint32_t));
    memset(m_fgScanlinePixels, 0, 320 * sizeof(uint32_t));

    m_frameBuf = new uint32_t[maxBufSize];

    prepareFrame(); // prepare 1st frame dimensions
}


Pk8000Renderer::~Pk8000Renderer()
{
    delete[] m_frameBuf;
}


void Pk8000Renderer::operate()
{
    if (m_activeArea) {
        renderLine(m_curLine);
        if (++m_curLine == 308) {
            m_curLine = 0;
            renderFrame();
        }

        // New scanline
        m_curScanlineClock = m_curClock;
        m_curScanlinePixel = 0;
        m_curBlankingPixel = 0;
        m_bank = m_nextLineBank;
        m_sgBase = m_nextLineSgBase;

        m_curClock += m_ticksPerScanLineSideBorder;
    } else {
        if (m_curLine == 263)
            m_platform->getCore()->vrtc(true);
        m_curClock += m_ticksPerScanLineActiveArea;
    }

    m_activeArea = !m_activeArea;

    if (m_waits)
        m_waits->setState(m_activeArea && m_wideBorder);
}


void Pk8000Renderer::attachScreenMemoryBank(int bank, Ram* screenMemoryBank)
{
    if (bank >= 0 && bank < 4) {
        m_screenMemoryBanks[bank] = screenMemoryBank->getDataPtr();
        m_screenMemoryRamBanks[bank] = screenMemoryBank;
    }
}


void Pk8000Renderer::setScreenBank(unsigned bank)
{
    if (bank < 4) {
        int curPixel = (g_emulation->getCurClock() - m_curScanlineClock) / m_ticksPerPixel;
        m_nextLineBank = bank;
        if (curPixel + m_pixelsPerOutInstruction < 80) // for ease, actually exact value must be used, w/o correction
            m_bank = bank;
    }
}


void Pk8000Renderer::setSymGenBufferBase(uint16_t base)
{
    int curPixel = (g_emulation->getCurClock() - m_curScanlineClock) / m_ticksPerPixel;
    m_nextLineSgBase = base;
    if (curPixel < 80 - m_pixelsPerOutInstruction) // for ease, actually exact value must be used, w/o correction
        m_sgBase = base;
}


void Pk8000Renderer::setMode(unsigned mode)
{
    if (mode < 4)
        m_mode = mode;
    m_wideBorder = (mode == 0) || (mode == 3);

    if (m_wideBorder) {
        m_ticksPerScanLineActiveArea = g_emulation->getFrequency() * 240 / 5000000;
        m_ticksPerScanLineSideBorder = g_emulation->getFrequency() * 80 / 5000000;
        m_pixelsPerOutInstruction = 30 - 3; // 3 (3.5) - correction from real
    } else {
        m_ticksPerScanLineActiveArea = g_emulation->getFrequency() * 256 / 5000000;
        m_ticksPerScanLineSideBorder = g_emulation->getFrequency() * 64 / 5000000;
        m_pixelsPerOutInstruction = 40 - 7; // 7 (7.5) - correction from real
    }
}


void Pk8000Renderer::setFgBgColors(unsigned fgColor, unsigned bgColor)
{
    int curPixel = (g_emulation->getCurClock() - m_curScanlineClock) / m_ticksPerPixel;
    for (int i = m_curScanlinePixel; i < curPixel; i++) {
        m_bgScanlinePixels[(i + m_pixelsPerOutInstruction) % 320] = m_bgColor;
        m_fgScanlinePixels[(i + m_pixelsPerOutInstruction) % 320] = m_fgColor;
    }
    m_curScanlinePixel = curPixel;

    m_fgColor = m_palette[fgColor];
    m_bgColor = m_palette[bgColor];
}


void Pk8000Renderer::setBlanking(bool blanking)
{
    int curPixel = (g_emulation->getCurClock() - m_curScanlineClock) / m_ticksPerPixel;
    for (int i = m_curBlankingPixel; i < curPixel; i++)
        m_blankingPixels[(i + m_pixelsPerOutInstruction) % 320] = m_blanking ? BS_BLANK : BS_NORMAL;
    m_curBlankingPixel = curPixel;

    m_blanking = blanking;
}

void Pk8000Renderer::setColorReg(unsigned addr, uint8_t value)
{
    if (m_mode == 1 && m_blanking) {
        m_colorRegs[addr & 0x1F] = value;

        int curPixel = (g_emulation->getCurClock() - m_curScanlineClock) / m_ticksPerPixel;

        for (int i = m_curBlankingPixel; i < curPixel - 8; i++)
            m_blankingPixels[(i + m_pixelsPerOutInstruction) % 320] = BS_BLANK;

        for (int i = curPixel - 8; i < curPixel; i++)
            m_blankingPixels[(i + m_pixelsPerOutInstruction) % 320] = BS_WRITE;

        m_curBlankingPixel = curPixel;
    }
}


uint8_t Pk8000Renderer::getColorReg(unsigned addr)
{
    return m_colorRegs[addr & 0x1F];
}


void Pk8000Renderer::renderLine(int nLine)
{
    // Render scan line #nLine (0-22 - invisible, 23-70 - border, 71-262 - visible, 263-302 - border, 303-311 - not used)

    if (nLine < 23 || nLine >= 303)
        return;

    for (int i = m_curScanlinePixel; i < 320; i++) {
        m_bgScanlinePixels[(i + m_pixelsPerOutInstruction) % 320] = m_bgColor;
        m_fgScanlinePixels[(i + m_pixelsPerOutInstruction) % 320] = m_fgColor;
    }

    for (int i = m_curBlankingPixel; i < 320; i++)
        m_blankingPixels[(i + m_pixelsPerOutInstruction) % 320] = m_blanking ? BS_BLANK : BS_NORMAL;

    uint32_t* linePtr;

    if (m_showBorder) {

        linePtr = m_frameBuf + m_sizeX * (nLine - 23);
        for(int i = 0; i < m_offsetX; i++)
            *linePtr++ = 0;

        if (nLine < 71 || nLine > 262 || /*m_blanking ||*/ m_mode > 2) {
            uint32_t* borderPixels = m_bgScanlinePixels + 59 + m_offsetX;
            for(int i = 0; i < m_sizeX - m_offsetX; i++)
                *linePtr++ = *borderPixels++;
            return;
        }
    } else {
        if (nLine < 71 || nLine > 262)
            return;

        linePtr = m_frameBuf + m_sizeX * (nLine - 71);

        if (/*m_blanking ||*/ m_mode > 2) {
            for(int i = 0; i < m_sizeX; i++)
                *linePtr++ = m_bgColor;
            return;
        }
    }

    nLine -= 71;

    int row = nLine / 8;
    int line = nLine % 8;

    switch (m_mode) {
    case 0:
        for (int pos = 0; pos < 40; pos++) {
            uint8_t chr = m_screenMemoryBanks[m_bank][(m_txtBase & ~0x0400) + row * 64 + pos];
            uint8_t bt = m_screenMemoryBanks[m_bank][m_sgBase + chr * 8 + line];
            for (int i = 0; i < 6; i++) {
                uint32_t color = (bt & 0x80 ? m_fgScanlinePixels : m_bgScanlinePixels)[59 + m_offsetX + pos * 6 + i];
                *linePtr++ = color;
                bt <<= 1;
            }
        }
        break;
    case 1:
        for (int pos = 0; pos < 32; pos++) {
            uint8_t chr = m_screenMemoryBanks[m_bank][m_txtBase + row * 32 + pos];
            unsigned colorCode = m_colorRegs[chr >> 3];
            uint32_t fgColor = m_palette[colorCode & 0x0F];
            uint32_t bgColor = m_palette[colorCode >> 4];
            uint8_t bt = m_screenMemoryBanks[m_bank][m_sgBase + chr * 8 + line];
            for (int i = 0; i < 8; i++) {
                BlankingState bs = m_blankingPixels[54 + m_offsetX + pos * 8 + i];
                uint32_t color;
                switch (bs) {
                case BS_NORMAL:
                    color = bt & 0x80 ? fgColor : bgColor;
                    break;
                case BS_BLANK:
                    color = m_palette[15];
                    break;
                case BS_WRITE:
                default:
                    color = bgColor;
                    break;
                }

                *linePtr++ = color;
                bt <<= 1;
            }
        }
        break;
    case 2: {
        int part = nLine / 64;
        row %= 8;
        for (int pos = 0; pos < 32; pos++) {
            uint8_t chr = m_screenMemoryBanks[m_bank][m_sgBase + part * 256 + row * 32 + pos];
            unsigned colorCode = m_screenMemoryBanks[m_bank][m_colBase + part * 0x800 + chr * 8 + line];
            uint32_t fgColor = m_palette[colorCode & 0x0F];
            uint32_t bgColor = m_palette[colorCode >> 4];
            uint8_t bt = m_screenMemoryBanks[m_bank][m_grBase + part * 0x800 + chr * 8 + line];
            for (int i = 0; i < 8; i++) {
                uint32_t color = bt & 0x80 ? fgColor : bgColor;
                *linePtr++ = color;
                bt <<= 1;
            }
        }
        break;
    }
    default:
        break;
    }
}


void Pk8000Renderer::renderFrame()
{
    memcpy(m_pixelData, m_frameBuf, m_sizeX * m_sizeY * sizeof(uint32_t));
    swapBuffers();
    prepareFrame();
}


void Pk8000Renderer::prepareFrame()
{
    if (!m_showBorder) {
        m_offsetX = m_offsetY = 0;
        m_sizeX = m_mode ? 256 : 240;
        m_sizeY = 192;
        m_aspectRatio = 576.0 * 9 / 704 / 5;
    } else {
        m_offsetX = (m_mode == 0) ? 21 : 5;
        m_sizeX = 261;
        m_sizeY = 288;
        m_aspectRatio = double(m_sizeY) * 4 / 3 / m_sizeX;
    }

    if (m_showBorder) {
        // Last 8 lines are black
        uint32_t* ptr = m_frameBuf + 280 * m_sizeX;
        for(int i = 0; i < m_sizeX * 8; i++)
            *ptr++ = 0;
    }

}


void Pk8000Renderer::setColorMode(bool colorMode)
{
    m_colorMode = colorMode;
    m_palette = m_colorMode ? c_pk8000ColorPalette : c_pk8000BwPalette;
}


void Pk8000Renderer::toggleColorMode()
{
    setColorMode(!m_colorMode);
}


void Pk8000Renderer::toggleCropping()
{
    m_showBorder = !m_showBorder;
}


bool Pk8000Renderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (CrtRenderer::setProperty(propertyName, values))
        return true;

    if (propertyName == "screenMemoryBank") {
        attachScreenMemoryBank(values[0].asInt(), static_cast<Ram*>(g_emulation->findObject(values[1].asString())));
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
    } else if (propertyName == "cpuWaits") {
        m_waits = (static_cast<Pk8000CpuWaits*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


string Pk8000Renderer::getPropertyStringValue(const string& propertyName)
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
        switch (m_mode) {
        case 0:
            return u8"Mode 0: 40\u00D76\u00D724\u00D78@50.73Hz";
            break;
        case 1:
            return u8"Mode 1: 32\u00D78\u00D724\u00D78@50.73Hz";
            break;
        case 2:
            return "Mode 2: 256\u00D7192@50.73Hz";
            break;
        default:
            return "Mode 3";
        }
    }

    return "";
}


Pk8000Ppi8255Circuit1::Pk8000Ppi8255Circuit1()
{
    for (int i = 0; i < 4; i++)
        m_addrSpaceMappers[i] = nullptr;
}


void Pk8000Ppi8255Circuit1::attachAddrSpaceMapper(int bank, AddrSpaceMapper* addrSpaceMapper)
{
    if (bank >= 0 && bank < 4)
        m_addrSpaceMappers[bank] = addrSpaceMapper;
}


// port 80h
void Pk8000Ppi8255Circuit1::setPortA(uint8_t value)
{
    m_addrSpaceMappers[0]->setCurPage(value & 0x03);
    m_addrSpaceMappers[1]->setCurPage((value & 0x0C) >> 2);
    m_addrSpaceMappers[2]->setCurPage((value & 0x30) >> 4);
    m_addrSpaceMappers[3]->setCurPage((value & 0xC0) >> 6);
};


// port 81h
uint8_t Pk8000Ppi8255Circuit1::getPortB()
{
    if (m_kbd)
        return m_kbd->getMatrixRowState();
    else
        return 0xFF;

}


// port 82h
void Pk8000Ppi8255Circuit1::setPortC(uint8_t value)
{
    if (m_kbd)
        m_kbd->setMatrixRowNo(value & 0x0F);

    m_beepSoundSource->setValue(value & 0x80 ? 1 : 0);
    m_tapeSoundSource->setValue(value & 0x40 ? 1 : 0);
    m_platform->getCore()->tapeOut(value & 0x40);
}


bool Pk8000Ppi8255Circuit1::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "bank") {
        attachAddrSpaceMapper(values[0].asInt(), static_cast<AddrSpaceMapper*>(g_emulation->findObject(values[1].asString())));
        return true;
    } else if (propertyName == "keyboard") {
        attachKeyboard(static_cast<Pk8000Keyboard*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "tapeSoundSource") {
        m_tapeSoundSource = static_cast<GeneralSoundSource*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "beepSoundSource") {
        m_beepSoundSource = static_cast<GeneralSoundSource*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


// port 84h
void Pk8000Ppi8255Circuit2::setPortA(uint8_t value)
{
    if (m_renderer) {
        if (value & 0x10)
            m_renderer->setMode(value & 0x20 ? 3 : 2);
        else
            m_renderer->setMode(value & 0x20 ? 0 : 1);
        m_renderer->setScreenBank((value & 0xc0) >> 6);
    }
}


// port 85h - printer data
void Pk8000Ppi8255Circuit2::setPortB(uint8_t value)
{
    m_printerData = value;
}


// port 86h
void Pk8000Ppi8255Circuit2::setPortC(uint8_t value)
{
    if (m_renderer) {
        m_renderer->setBlanking(!(value & 0x10));
    }

    bool newStrobe = value & 0x80;
    if (m_printerStrobe && !newStrobe) {
        g_emulation->getPrnWriter()->printByte(m_printerData);
    }
    m_printerStrobe = newStrobe;
}


uint8_t Pk8000Ppi8255Circuit2::getPortC()
{
    return 0xFB; // printer not busy
}


bool Pk8000Ppi8255Circuit2::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<Pk8000Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


// ports A0-BFh
void Pk8000Mode1ColorMem::writeByte(int addr, uint8_t value)
{
    if (m_renderer)
        m_renderer->setColorReg(addr & 0x1F, value);
}


// ports A0-BFh
uint8_t Pk8000Mode1ColorMem::readByte(int addr)
{
    if (m_renderer)
        return m_renderer->getColorReg(addr & 0x1F);
    return 0;
}


bool Pk8000Mode1ColorMem::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<Pk8000Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


// port 88h
void Pk8000ColorSelector::writeByte(int, uint8_t value)
{
    m_value = value;
    if (m_renderer) {
        m_renderer->setFgBgColors(value & 0x0F, value >> 4);
    }
}


bool Pk8000ColorSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<Pk8000Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


// port 90h
void Pk8000TxtBufSelector::writeByte(int, uint8_t value)
{
    m_value = value;
    if (m_renderer) {
        m_renderer->setTextBufferBase((value & 0x0F) << 10);
    }
}


bool Pk8000TxtBufSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<Pk8000Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


// port 91h
void Pk8000SymGenBufSelector::writeByte(int, uint8_t value)
{
    m_value = value;
    if (m_renderer) {
        m_renderer->setSymGenBufferBase((value & 0x0E) << 10);
    }
}


bool Pk8000SymGenBufSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<Pk8000Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


// port 93h
void Pk8000GrBufSelector::writeByte(int, uint8_t value)
{
    m_value = value;
    if (m_renderer) {
        m_renderer->setGraphicsBufferBase((~value & 0x08) << 10);
    }
}


bool Pk8000GrBufSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<Pk8000Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


// port 92h
void Pk8000ColBufSelector::writeByte(int, uint8_t value)
{
    m_value = value;
    if (m_renderer) {
        m_renderer->setColorBufferBase((~value & 0x08) << 10);
    }
}


bool Pk8000ColBufSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<Pk8000Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


bool Pk8000FileLoader::loadFile(const std::string& fileName, bool run)
{
    static const uint8_t headerSeq[8] = {0x1F, 0xA6, 0xDE, 0xBA, 0xCC, 0x13, 0x7D, 0x74};

    int fileSize;
    uint8_t* buf = palReadFile(fileName, fileSize, false);
    if (!buf)
        return false;

    int fullSize = fileSize;

    if (fileSize < 39) {
        delete[] buf;
        return false;
    }

    uint8_t* ptr = buf;

    if (memcmp(ptr, headerSeq, 8) != 0) {
        delete[] buf;
        return false;
    }

    ptr += 8;
    fileSize -= 8;

    if (*ptr == 0xD0) {
        // Binary file
        for (int i = 0; i < 10; i++)
            if (ptr[i] != 0xD0) {
                delete[] buf;
                return false;
            }

        ptr += 16;
        fileSize -= 16;

        if (*ptr != headerSeq[0]) {
            ptr += 8;
            fileSize -= 8;
        }

        if (fileSize < 15 || memcmp(ptr, headerSeq, 8) != 0) {
            delete[] buf;
            return false;
        }

        ptr += 8;
        fileSize -= 8;

        uint16_t begAddr = (ptr[1] << 8) | ptr[0];
        uint16_t endAddr = (ptr[3] << 8) | ptr[2];
        uint16_t startAddr = (ptr[5] << 8) | ptr[4];

        endAddr--; // PK8000 feature?

        ptr += 6;
        fileSize -= 6;

        if (begAddr == 0xF6D0 /*&& endAddr == 0xF88E*/) {
            // программы с автозапуском
            if (fileSize < 0x60) {
                delete[] buf;
                return false;
            }

            ptr += 0x5B;

            begAddr = (ptr[1] << 8) | ptr[0];
            uint16_t len = (ptr[3] << 8) | ptr[2];
            endAddr = begAddr + len - 1;
            startAddr = (ptr[5] << 8) | ptr[4];

            ptr += 6;
            fileSize -= 6;
        }

        uint16_t progLen = endAddr - begAddr + 1;

        if (progLen > fileSize) {
            delete[] buf;
            return false;
        }

        if (run) {
            m_as->writeByte(0x4000, 0);
            m_as->writeByte(0x4001, 0);
            m_platform->reset();
            Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(m_platform->getCpu());
            if (cpu) {
                cpu->disableHooks();
                g_emulation->exec((int64_t)cpu->getKDiv() * m_skipTicks);
                cpu->enableHooks();
                cpu->setPC(startAddr);
            }
        }

        for (unsigned addr = begAddr; addr <= endAddr; addr++)
            m_as->writeByte(addr, *ptr++);
        if (begAddr != 0xF6D0)
            fileSize -= (endAddr - begAddr + 1);
        else
            fileSize -= 0x61;

        if (run && m_allowMultiblock && m_tapeRedirector && fileSize > 0) {
            m_tapeRedirector->assignFile(fileName, "r");
            m_tapeRedirector->openFile();
            m_tapeRedirector->assignFile("", "r");
            m_tapeRedirector->setFilePos(fullSize - fileSize);
        }

    } else if (*ptr == 0xD3) {
        // Basic file
        for (int i = 0; i < 10; i++)
            if (ptr[i] != 0xD3) {
                delete[] buf;
                return false;
            }

        ptr += 16;
        fileSize -= 16;

        if (*ptr != headerSeq[0]) {
            ptr += 8;
            fileSize -= 8;
        }

        if (fileSize < 10 || memcmp(ptr, headerSeq, 8) != 0) {
            delete[] buf;
            return false;
        }

        ptr += 8;
        fileSize -= 8;

        if (fileSize >= 0xAF00) {
            delete[] buf;
            return false;
        }

        Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(m_platform->getCpu());

        m_as->writeByte(0x4000, 0);
        m_as->writeByte(0x4001, 0);
        m_platform->reset();
        if (cpu) {
            cpu->disableHooks();
            g_emulation->exec((int64_t)cpu->getKDiv() * m_skipTicks);
            cpu->enableHooks();
        }

        m_as->writeByte(0x4000, 0);
        /*for (unsigned addr = 0x4001; fileSize; addr++, fileSize--)
            m_as->writeByte(addr, *ptr++);*/

        uint16_t addr, nextAddr;
        addr = nextAddr = 0x4001;
        for(;;) {
            if (addr == nextAddr + 1)
                nextAddr = (ptr[0] << 8) | ptr[-1];
            m_as->writeByte(addr++, *ptr++);
            fileSize--;
            if (nextAddr == 0 || fileSize == 0 || addr >= 0xEEFF)
                break;
        }

        uint16_t memLimit = addr + 0x100;

        if (run && m_allowMultiblock && m_tapeRedirector && fileSize > 0) {
            m_tapeRedirector->assignFile(fileName, "r");
            m_tapeRedirector->openFile();
            m_tapeRedirector->assignFile("", "r");
            m_tapeRedirector->setFilePos(fullSize - fileSize);
        }

        if (cpu) {
            m_as->writeByte(0xF930, memLimit & 0xFF);
            m_as->writeByte(0xF931, memLimit >> 8);
            m_as->writeByte(0xF932, memLimit & 0xFF);
            m_as->writeByte(0xF933, memLimit >> 8);
            m_as->writeByte(0xF934, memLimit & 0xFF);
            m_as->writeByte(0xF935, memLimit >> 8);
            if (run) {
                uint16_t kbdBuf = m_as->readByte(0xFA2C) | (m_as->readByte(0xFA2D) << 8);
                m_as->writeByte(kbdBuf++, 0x52);
                m_as->writeByte(kbdBuf++, 0x55);
                m_as->writeByte(kbdBuf++, 0x4E);
                m_as->writeByte(kbdBuf++, 0x0D);
                m_as->writeByte(0xFA2A, kbdBuf & 0xFF);
                m_as->writeByte(0xFA2B, kbdBuf >> 8);
            }
            //cpu->setSP(0xF7FD);
            cpu->setPC(0x030D);

            // Workaround to suppress closing file by CloseFileHook
            cpu->disableHooks();
            g_emulation->exec((int64_t)cpu->getKDiv() * 100000);
            cpu->enableHooks();
        }
    }

    return true;
}


Pk8000Keyboard::Pk8000Keyboard()
{
    Pk8000Keyboard::resetKeys();
}


void Pk8000Keyboard::resetKeys()
{
    for (int i = 0; i < 10; i++)
        m_keys[i] = 0;
    m_rowNo = 0;
    m_joystickKeys = 0;
}


void Pk8000Keyboard::processKey(EmuKey key, bool isPressed)
{
    if (key == EK_NONE)
        return;

    int i, j;

    // Joystick
    if (m_platform->getKbdLayout()->getNumpadJoystickMode())
        switch (key) {
        case EK_JS_UP:
            m_joystickKeys = isPressed ? m_joystickKeys | 0x01 : m_joystickKeys & ~0x01;
            break;
        case EK_JS_DOWN:
            m_joystickKeys = isPressed ? m_joystickKeys | 0x02 : m_joystickKeys & ~0x02;
            break;
        case EK_JS_LEFT:
            m_joystickKeys = isPressed ? m_joystickKeys | 0x04 : m_joystickKeys & ~0x04;
            break;
        case EK_JS_RIGHT:
            m_joystickKeys = isPressed ? m_joystickKeys | 0x08 : m_joystickKeys & ~0x08;
            break;
        case EK_JS_BTN1:
            m_joystickKeys = isPressed ? m_joystickKeys | 0x10 : m_joystickKeys & ~0x10;
            break;
        case EK_JS_BTN2:
            m_joystickKeys = isPressed ? m_joystickKeys | 0x20 : m_joystickKeys & ~0x20;
            break;
        default:
            break;
        }
    else
        switch (key) {
        case EK_UP:
            m_joystickKeys = isPressed ? m_joystickKeys | 0x01 : m_joystickKeys & ~0x01;
            break;
        case EK_DOWN:
            m_joystickKeys = isPressed ? m_joystickKeys | 0x02 : m_joystickKeys & ~0x02;
            break;
        case EK_LEFT:
            m_joystickKeys = isPressed ? m_joystickKeys | 0x04 : m_joystickKeys & ~0x04;
            break;
        case EK_RIGHT:
            m_joystickKeys = isPressed ? m_joystickKeys | 0x08 : m_joystickKeys & ~0x08;
            break;
        case EK_SPACE:
            m_joystickKeys = isPressed ? m_joystickKeys | 0x10 : m_joystickKeys & ~0x10;
            break;
        case EK_CR:
            m_joystickKeys = isPressed ? m_joystickKeys | 0x20 : m_joystickKeys & ~0x20;
            break;
        default:
            break;
        }

    // Основная матрица
    for (i = 0; i < 10; i++)
        for (j = 0; j < 8; j++)
            if (key == m_keyMatrix[i][j])
                goto found;
    return;

    found:
    if (isPressed)
        m_keys[i] |= (1 << j);
    else
        m_keys[i] &= ~(1 << j);
}


void Pk8000Keyboard::setMatrixRowNo(uint8_t row)
{
    m_rowNo = row < 10 ? row : 0;
}


uint8_t Pk8000Keyboard::getMatrixRowState()
{
    return ~m_keys[m_rowNo];
}


uint8_t Pk8000InputRegister::readByte(int)
{
    return (m_kbd->getJoystickState() & 0x3F) | (g_emulation->getWavReader()->getCurValue() ? 0x80 : 0x00);
}


bool Pk8000InputRegister::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "keyboard") {
        attachKeyboard(static_cast<Pk8000Keyboard*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


EmuKey Pk8000KbdLayout::translateKey(PalKeyCode keyCode)
{
    switch (keyCode) {
    case PK_INS:
        return EK_INS;
    case PK_DEL:
        return EK_DEL;
    case PK_PGUP:
        return EK_LANG;
    case PK_KP_0:
        return EK_PHOME;
    default:
        break;
    }

    EmuKey key = translateCommonKeys(keyCode);
    if (key != EK_NONE)
        return key;

    switch (keyCode) {
    case PK_KP_1:
        return EK_HOME;
    case PK_LCTRL:
    case PK_RCTRL:
        return EK_CTRL;

    case PK_F6:
        return EK_UNDSCR;
    case PK_F10:
        return EK_GRAPH;
    case PK_F8:
    case PK_MENU:
        return EK_FIX;
    case PK_F12:
        return EK_STOP;
    case PK_F11:
        return EK_SEL;
    case PK_KP_MUL:
        return EK_INS;
    case PK_KP_DIV:
        return EK_DEL;
    case PK_KP_7:
        return EK_SHOME; //7
    case PK_KP_9:
        return EK_SEND;  // 9
    case PK_KP_3:
        return EK_END;   // 3
    case PK_KP_PERIOD:
        return EK_PEND;  // .
    case PK_KP_0:
        return EK_PEND;  // 0
    case PK_KP_5:
        return EK_MENU;  // 5

    case PK_KP_MINUS:
        return EK_CLEAR;  // - СТРН

    default:
        return translateCommonKeys(keyCode);
    }
}


EmuKey Pk8000KbdLayout::translateUnicodeKey(unsigned unicodeKey, PalKeyCode keyCode, bool& shift, bool& lang)
{
    if (keyCode == PK_KP_MUL || keyCode == PK_KP_DIV || keyCode == PK_KP_MINUS)
        return EK_NONE;
    EmuKey key = translateCommonUnicodeKeys(unicodeKey, shift, lang);
    if (key >= EK_0 && key <= EK_9)
        shift = !shift;
    if (unicodeKey == L'_') {
        key = EK_UNDSCR;
        shift = true;
        lang = false;
    }
    return key;
}


bool Pk8000KbdLayout::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (KbdLayout::setProperty(propertyName, values))
        return true;

    if (propertyName == "numpadJoystick") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_numpadJoystick = values[0].asString() == "yes";
            return true;
        }
    }

    return false;
}


string Pk8000KbdLayout::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = KbdLayout::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "numpadJoystick")
        return m_numpadJoystick ? "yes" : "no";

    return "";
}


void Pk8000FddControlRegister::writeByte(int, uint8_t value)
{
    if (value & 0x80)
        m_fdc->reset();
    m_fdc->setDrive(value & 0x40 ? 1 : 0); // пока так
    m_fdc->setHead((value & 0x10) >> 4);
}


bool Pk8000FddControlRegister::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "fdc") {
        attachFdc1793(static_cast<Fdc1793*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


uint8_t Pk8000FdcStatusRegisters::readByte(int)
{
    //return (m_bytes[addr & 0x3] & 0x7E) | (m_fdc->getDrq() ? 0 : 1) | (m_fdc->getIrq() ? 0 : 0x80);
    return m_bytes[(m_fdc->getIrq() ? 0x01 : 0) | (m_fdc->getDrq() ? 0x02 : 0)];
}


bool Pk8000FdcStatusRegisters::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "fdc") {
        attachFdc1793(static_cast<Fdc1793*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


int Pk8000CpuWaits::getCpuWaitStates(int memTag, int opcode, int normalClocks)
{
    static const int waits12Ram[256] = {
        4,6,5,3,3,3,5,4,4,2,5,3,3,3,5,4,
        4,6,5,3,3,3,5,4,4,2,5,3,3,3,5,4,
        4,6,16,3,3,3,5,4,4,2,8,3,3,3,5,4,
        4,6,11,3,10,10,10,4,4,2,7,3,3,3,5,4,
        3,3,3,3,3,3,5,3,3,3,3,3,3,3,5,3,
        3,3,3,3,3,3,5,3,3,3,3,3,3,3,5,3,
        3,3,3,3,3,3,5,3,3,3,3,3,3,3,5,3,
        5,5,5,5,5,5,4,5,3,3,3,3,3,3,5,3,
        4,4,4,4,4,4,5,4,4,4,4,4,4,4,5,4,
        4,4,4,4,4,4,5,4,4,4,4,4,4,4,5,4,
        4,4,4,4,4,4,5,4,4,4,4,4,4,4,5,4,
        4,4,4,4,4,4,5,4,4,4,4,4,4,4,5,4,
        3,6,6,6,5,9,5,9,3,6,6,6,5,15,5,9,
        3,6,6,10,5,9,5,9,3,6,6,6,5,15,5,9,
        3,6,6,18,5,9,5,9,3,3,6,4,5,15,5,9,
        3,6,6,4,5,9,5,9,3,3,6,4,5,15,5,9
    };

    static const int waits03Ram[256] = {
        2,8,5,1,1,1,5,2,2,2,5,1,1,1,5,2,
        2,8,5,1,1,1,5,2,2,2,5,1,1,1,5,2,
        2,8,14,1,1,1,5,2,2,2,14,1,1,1,5,2,
        2,8,11,1,8,8,8,2,2,2,11,1,1,1,5,2,
        1,1,1,1,1,1,5,1,1,1,1,1,1,1,5,1,
        1,1,1,1,1,1,5,1,1,1,1,1,1,1,5,1,
        1,1,1,1,1,1,5,1,1,1,1,1,1,1,5,1,
        5,5,5,5,5,5,2,5,1,1,1,1,1,1,5,1,
        2,2,2,2,2,2,5,2,2,2,2,2,2,2,5,2,
        2,2,2,2,2,2,5,2,2,2,2,2,2,2,5,2,
        2,2,2,2,2,2,5,2,2,2,2,2,2,2,5,2,
        2,2,2,2,2,2,5,2,2,2,2,2,2,2,5,2,
        1,8,8,8,7,10,5,10,1,8,8,8,7,13,5,10,
        1,8,8,5,7,10,5,10,1,8,8,5,7,13,5,10,
        1,8,8,12,7,10,5,10,1,1,8,2,7,13,5,10,
        1,8,8,2,7,10,5,10,1,1,8,2,7,13,5,10
    };

    static const int waits12Rom[256] = {
        1,3,5,1,1,1,2,1,1,1,2,1,1,1,2,1,
        1,3,5,1,1,1,2,1,1,1,2,1,1,1,2,1,
        1,3,12,1,1,1,2,1,1,1,5,1,1,1,2,1,
        1,3,7,1,6,6,6,1,1,1,4,1,1,1,2,1,
        1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
        1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
        1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
        5,5,5,5,5,5,1,5,1,1,1,1,1,1,2,1,
        1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
        1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
        1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
        1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
        1,3,3,3,3,9,2,8,1,3,3,3,3,11,2,8,
        1,3,3,4,3,9,2,8,1,3,3,3,3,11,2,8,
        1,3,3,10,3,9,2,8,1,1,3,1,3,11,2,8,
        1,3,3,1,3,9,2,8,1,1,3,1,3,11,2,8
    };

    static const int waits03Rom[256] = {
        1,3,5,1,1,1,2,1,1,1,2,1,1,1,2,1,
        1,3,5,1,1,1,2,1,1,1,2,1,1,1,2,1,
        1,3,8,1,1,1,2,1,1,1,5,1,1,1,2,1,
        1,3,5,1,5,5,5,1,1,1,4,1,1,1,2,1,
        1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
        1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
        1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
        5,5,5,5,5,5,1,5,1,1,1,1,1,1,2,1,
        1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
        1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
        1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
        1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
        1,3,3,3,3,7,2,6,1,3,3,3,3,9,2,6,
        1,3,3,4,3,7,2,6,1,3,3,3,3,9,2,6,
        1,3,3,9,3,7,2,6,1,1,3,1,3,9,2,6,
        1,3,3,1,3,7,2,6,1,1,3,1,3,9,2,6
    };

    int addClocks;

    if (memTag) {
        addClocks = m_scr03activeArea ? waits03Ram[opcode] : waits12Ram[opcode];
        if ((opcode & 0xC7) == 0xC0) //Rxx
            addClocks = normalClocks == 5 ? (m_scr03activeArea ? 1 : 3) : (m_scr03activeArea ? 7 : 5);
        else if ((opcode & 0xC7) == 0xC4) //Cxx
            addClocks = normalClocks == 11 ? (m_scr03activeArea ? 7 : 5) : (m_scr03activeArea ? 13 : 15);
    } else {
        addClocks = m_scr03activeArea ? waits03Rom[opcode] : waits12Rom[opcode];
        if ((opcode & 0xC7) == 0xC0) //Rxx
            addClocks = normalClocks == 5 ? 1 : 3;
        else if ((opcode & 0xC7) == 0xC4) //Cxx
            addClocks = normalClocks == 11 ? 14 : (m_scr03activeArea ? 9 : 11);
    }

    return addClocks;
}
