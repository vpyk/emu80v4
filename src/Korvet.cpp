/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2021-2024
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

#include <cstring>
#include <sstream>

#include "Pal.h"

#include "Korvet.h"
#include "Emulation.h"
#include "Platform.h"
#include "Globals.h"
#include "EmuWindow.h"
#include "Cpu.h"
#include "Fdc1793.h"
#include "Covox.h"
#include "Pit8253.h"
#include "Pic8259.h"
#include "WavReader.h"
#include "PrnWriter.h"
#include "Psg3910.h"

// Korvet implementation

using namespace std;


KorvetAddrSpace::KorvetAddrSpace(string fileName)
{
    m_memMap = new uint8_t[8192];
    if (palReadFromFile(fileName, 0, 8192, m_memMap) != 8192) {
        delete[] m_memMap;
        m_memMap = nullptr;
    }
}


KorvetAddrSpace::~KorvetAddrSpace()
{
    delete[] m_memMap;
}



uint8_t KorvetAddrSpace::readByte(int addr)
{
    m_lastTag = 0;
    int page = m_memMap[(m_memCfg << 6) | (addr >> 8) ];
    if (page == 5)
        m_lastTag = 1;
    return m_pages[page]->readByte(addr);
}



void KorvetAddrSpace::writeByte(int addr, uint8_t value)
{
    m_lastTag = 0;
    int page = m_memMap[(m_memCfg << 6) | (addr >> 8)];
    if (page == 5)
        m_lastTag = 1;
    m_pages[page]->writeByte(addr, value);
}



void KorvetAddrSpace::setPage(int pageNum, AddressableDevice* page)
{
    m_pages[pageNum] = page;
}


bool KorvetAddrSpace::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "page") {
        if (values[0].isInt()) {
            setPage(values[0].asInt(), static_cast<AddressableDevice*>(g_emulation->findObject(values[1].asString())));
            return true;
        }
    }
    return false;
}


void KorvetAddrSpaceSelector::writeByte(int, uint8_t value)
{
    m_korvetAddrSpace->setMemCfg(value & 0x7C);
}


bool KorvetAddrSpaceSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "addrSpace") {
        attachKorvetAddrSpace(static_cast<KorvetAddrSpace*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


void KorvetCore::draw()
{
    m_window->drawFrame(m_crtRenderer->getPixelData());
    m_window->endDraw();
}


void KorvetCore::inte(bool isActive)
{
    m_pic->inte(isActive);
}


void KorvetCore::vrtc(bool isActive)
{
//    m_pic->irq(4, isActive);

    if (m_curVrtc != isActive) {
        m_curVrtc = isActive;
        m_ppiCircuit->setVbl(!isActive);
        //m_pit->getCounter(2)->setGate(!isActive);
    }
}


void KorvetCore::hrtc(bool isActive, int)
{
    if (isActive && !m_curHrtc) {
        Pit8253Counter* cnt = m_pit->getCounter(2);
        cnt->operateForTicks(1);
    }
    m_curHrtc = isActive;
}


void KorvetCore::timer(int /*id*/, bool isActive)
{
    m_pic->irq(5, isActive);
}


void KorvetCore::int4(bool isActive)
{
    m_pic->irq(4, isActive);
}

void KorvetCore::int7()
{
    m_pic->irq(7, true);
    m_pic->irq(7, false);
}


void KorvetCore::attachCrtRenderer(CrtRenderer* crtRenderer)
{
    m_crtRenderer = crtRenderer;
}


bool KorvetCore::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (PlatformCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<CrtRenderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "ppiCircuit") {
        m_ppiCircuit = static_cast<KorvetPpi8255Circuit*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "pit") {
        m_pit = static_cast<Pit8253*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "pic") {
        m_pic = static_cast<Pic8259*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


KorvetRenderer::KorvetRenderer()
{
    const int pixelFreq = 10; // MHz
    const int maxBufSize = 521 * 288; // 521 = 704 / 13.5 * pixelFreq

    m_sizeX = m_prevSizeX = 512;
    m_sizeY = m_prevSizeY = 256;

    m_aspectRatio = m_prevAspectRatio = 576.0 * 9 / 704 / pixelFreq;
    m_bufSize = m_prevBufSize = m_sizeX * m_sizeY;
    m_pixelData = new uint32_t[maxBufSize];
    m_prevPixelData = new uint32_t[maxBufSize];
    memset(m_pixelData, 0, m_bufSize * sizeof(uint32_t));
    memset(m_prevPixelData, 0, m_prevBufSize * sizeof(uint32_t));

    memset(m_lut, 0, sizeof(m_lut));

    m_frameBuf = new uint32_t[maxBufSize];
}


KorvetRenderer::~KorvetRenderer()
{
    if (m_font)
        delete[] m_font;

    delete[] m_frameBuf;
}


void KorvetRenderer::setFontFile(std::string fontFileName)
{
    m_font = new uint8_t[8192];
    if (palReadFromFile(fontFileName, 0, 8192, m_font) != 8192) {
        delete[] m_font;
        m_font = nullptr;
    }
}


void KorvetRenderer::setDisplayPage(int page)
{
    m_displayPage = page;
}


void KorvetRenderer::operate()
{
    renderLine(m_curLine);
    if (++m_curLine == 312) {
        m_curLine = 0;
        renderFrame();
        g_emulation->screenUpdateReq();
    }

    if (m_curLine == 297) // acrually SVBL in much shorter than 1 scanline
        static_cast<KorvetCore*>(m_platform->getCore())->int4(false);
    if (m_curLine == 296) {
        m_platform->getCore()->vrtc(true);
        static_cast<KorvetCore*>(m_platform->getCore())->int4(true);
    } else if (m_curLine == 39)
        m_platform->getCore()->vrtc(false);

    static_cast<KorvetCore*>(m_platform->getCore())->hrtc(true, 0);
    static_cast<KorvetCore*>(m_platform->getCore())->hrtc(false, 0);

    m_curClock += g_emulation->getFrequency() / 10000000 * 640;
}


void KorvetRenderer::prepareFrame()
{
    if (!m_showBorder) {
        m_sizeX = 512;
        m_sizeY = 256;
        m_aspectRatio = 576.0 * 9 / 704 / 10;
    } else {
        m_sizeX = 521;
        m_sizeY = 288;
        m_aspectRatio = double(m_sizeY) * 4 / 3 / m_sizeX;
    }
}


void KorvetRenderer::renderFrame()
{
    memcpy(m_pixelData, m_frameBuf, m_sizeX * m_sizeY * sizeof(uint32_t));
    swapBuffers();
    prepareFrame();
}


void KorvetRenderer::renderLine(int nLine)
{
    // Render scan line #nLine
    // Vertical: 0-22 - invisible, 23-39 - border, 40-295 - visible, 296-311 - border)

    if (nLine < 23 || nLine >= 311)
        return;

    uint32_t* linePtr;

    if (m_showBorder) {
        linePtr = m_frameBuf + m_sizeX * (nLine - 23);

        if (nLine < 40 || nLine >= 296) {
            for(int i = 0; i < m_sizeX; i++)
                *linePtr++ = m_palette[m_lut[0]];
            return;
        } else {
            for(int i = 0; i < 4; i++)
                linePtr[i] = m_palette[m_lut[0]];
            for(int i = 512 + 4; i < 521; i++)
                linePtr[i] = m_palette[m_lut[0]];
        }
        linePtr += 4;
    } else {
        if (nLine < 40 || nLine >= 296)
            return;
        linePtr = m_frameBuf + m_sizeX * (nLine - 40);
    }

    nLine -= 40;
    uint8_t* fontPtr = m_font + m_fontNo * 4096;

    for (int nbt = nLine * 64; nbt < (nLine + 1) * 64; nbt++) {
        uint8_t bt0 = m_graphicsAdapter->m_planes[0][nbt + m_displayPage * 0x4000];
        uint8_t bt1 = m_graphicsAdapter->m_planes[1][nbt + m_displayPage * 0x4000];
        uint8_t bt2 = m_graphicsAdapter->m_planes[2][nbt + m_displayPage * 0x4000];
        int symbol = (nbt >> 4 & 0x3C0) | (nbt & 0x003F);
        uint8_t bt3;
        if (!m_wideChr)
            bt3 = fontPtr[m_textAdapter->m_symbols[symbol] << 4 | (nbt >> 6 & 0x0F)] ^ m_textAdapter->m_attrs[symbol];
        else {
            uint8_t b = fontPtr[m_textAdapter->m_symbols[symbol & ~1] << 4 | (nbt >> 6 & 0x0F)] ^ m_textAdapter->m_attrs[symbol & ~1];
            if (symbol & 1) b <<= 4;
            bt3 = b & 0x80 ? 0xC0 : 0;
            if (b & 0x40) bt3 |= 0x30;
            if (b & 0x20) bt3 |= 0x0C;
            if (b & 0x10) bt3 |= 0x03;
        }
        for (int px = 0; px < 8; px++) {
            int colorIdx = (bt0 >> 7) | (bt1 >> 6 & 2) | (bt2 >> 5 & 4) | (bt3 >> 4 & 8);
            *linePtr++ = m_palette[m_lut[colorIdx]];
            bt0 <<= 1;
            bt1 <<= 1;
            bt2 <<= 1;
            bt3 <<= 1;
        }
    }
}


void KorvetRenderer::setColorMode(bool colorMode)
{
    m_colorMode = colorMode;
    m_palette = m_colorMode ? c_korvetColorPalette : c_korvetBwPalette;
}


void KorvetRenderer::toggleColorMode()
{
    setColorMode(!m_colorMode);
}


void KorvetRenderer::toggleCropping()
{
    m_showBorder = !m_showBorder;
}


const char* KorvetRenderer::getTextScreen()
{
    int step = m_wideChr ? 2 : 1;
    int w = m_wideChr ? 32 : 64;

    wchar_t* wTextArray = new wchar_t[16 * w];

    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 64; x += step) {
            uint8_t chr = m_textAdapter->m_symbols[y * 64 + x];
            wchar_t wchr = c_korvetSymbols[m_fontNo * 256 + chr];
            wTextArray[y * w + x / step] = wchr;
        }
    }

    return generateTextScreen(wTextArray, w, 16);
}


bool KorvetRenderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (CrtRenderer::setProperty(propertyName, values))
        return true;

    if (propertyName == "font") {
        setFontFile(values[0].asString());
        return true;
    } else if (propertyName == "textAdapter") {
        attachTextAdapter(static_cast<KorvetTextAdapter*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "graphicsAdapter") {
        attachGraphicsAdapter(static_cast<KorvetGraphicsAdapter*>(g_emulation->findObject(values[0].asString())));
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


string KorvetRenderer::getPropertyStringValue(const string& propertyName)
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
        return u8"512\u00D7256@50.08Hz";
    }

    return "";
}


string KorvetRenderer::getDebugInfo()
{
    stringstream ss;
    ss << "CRT:" << "\n";
    ss << "L:" << m_curLine << "\n";
    return ss.str();
}


uint8_t KorvetPpi8255Circuit::getPortA()
{
    return m_addr | (g_emulation->getWavReader()->getCurValue() ? 0x01 : 0x00) |
           (m_vbl ? 0x02 : 0x00) | (m_textAdapter->getAttr() ? 0x08 : 0x00) |
           0x04; //(g_emulation->getPrnWriter()->getReady() ? 0x00 : 0x01);
}


void KorvetPpi8255Circuit::setPortB(uint8_t value)
{
    if (value & 1)
        m_fdc->setDrive(0);
    else if (value & 2)
        m_fdc->setDrive(1);
    else if (value & 4)
        m_fdc->setDrive(2);
    else if (value & 8)
        m_fdc->setDrive(3);
    m_fdc->setHead((value & 0x10) >> 4);
    //m_fdc->setHead(((value & 0x10) >> 4) ^ 1);
    if (value & 0x20 && !m_motorBit)
        m_motor->on();
    m_motorBit = value & 0x20;
}


void KorvetPpi8255Circuit::setPortC(uint8_t value)
{
    m_graphicsAdapter->setRwPage(value >> 6);
    m_textAdapter->setAttrMask(value >> 4 & 3);
    m_crtRenderer->setDisplayPage(value & 3);
    m_crtRenderer->setFont(value >> 2 & 1);
    m_crtRenderer->setWideChr(value >> 3 & 1);
}


bool KorvetPpi8255Circuit::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (Ppi8255Circuit::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        m_crtRenderer = static_cast<KorvetRenderer*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "graphicsAdapter") {
        m_graphicsAdapter = static_cast<KorvetGraphicsAdapter*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "textAdapter") {
        m_textAdapter = static_cast<KorvetTextAdapter*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "fdc") {
        m_fdc = static_cast<Fdc1793*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "fddMotor") {
        m_motor = static_cast<KorvetFddMotor*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "addr" && values[0].isInt()) {
        m_addr = (~values[0].asInt() & 0x0F) << 4;
        return true;
    }

    return false;
}


void KorvetPpi8255Circuit2::setPortA(uint8_t value)
{
    m_printerData = value;
}


void KorvetPpi8255Circuit2::setPortC(uint8_t value)
{
    const int c_covoxValues[4] = {-7, 0, 0, 7};
    m_covox->setValue(c_covoxValues[value & 3]);
    m_pitSoundSource->setGate(value & 8);

    bool newStrobe = value & 0x20;
    if (!m_printerStrobe && newStrobe) {
        g_emulation->getPrnWriter()->printByte(~m_printerData);
    }
    m_printerStrobe = newStrobe;
}


bool KorvetPpi8255Circuit2::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (Ppi8255Circuit::setProperty(propertyName, values))
        return true;

    if (propertyName == "covox") {
        m_covox = static_cast<Covox*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else  if (propertyName == "pitSoundSource") {
        m_pitSoundSource = static_cast<KorvetPit8253SoundSource*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


bool KorvetColorRegister::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "graphicsAdapter") {
        m_graphicsAdapter = static_cast<KorvetGraphicsAdapter*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


void KorvetColorRegister::writeByte(int, uint8_t value)
{
    m_graphicsAdapter->setColorRegisterValue(value);
}


bool KorvetLutRegister::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        m_crtRenderer = static_cast<KorvetRenderer*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


void KorvetLutRegister::writeByte(int, uint8_t value)
{
    m_crtRenderer->setLutValue(value & 0xF, value >> 4);
}


KorvetGraphicsAdapter::KorvetGraphicsAdapter()
{
    for (int i = 0; i < 3; i++) {
        m_planes[i] = new uint8_t[0x10000];
        memset(m_planes[i], 0, 0x10000);
    }
}


KorvetGraphicsAdapter::~KorvetGraphicsAdapter()
{
    for (int i = 0; i < 3; i++)
        delete[] m_planes[i];
}


void KorvetGraphicsAdapter::writeByte(int addr, uint8_t value)
{
    addr &= 0x3FFF;
    addr += m_rwPage * 0x4000;

    if (m_colorRegisterValue & 0x80) {
        // color mode
        if (m_colorRegisterValue & 0x02)
            m_planes[0][addr] |= value;
        else
            m_planes[0][addr] &= ~value;
        if (m_colorRegisterValue & 0x04)
            m_planes[1][addr] |= value;
        else
            m_planes[1][addr] &= ~value;
        if (m_colorRegisterValue & 0x08)
            m_planes[2][addr] |= value;
        else
            m_planes[2][addr] &= ~value;
    } else {
        // plane mode
        if (m_colorRegisterValue & 1) {
            if (!(m_colorRegisterValue & 0x02))
                m_planes[0][addr] |= value;
            if (!(m_colorRegisterValue & 0x04))
                m_planes[1][addr] |= value;
            if (!(m_colorRegisterValue & 0x08))
                m_planes[2][addr] |= value;
        } else {
            if (!(m_colorRegisterValue & 0x02))
                m_planes[0][addr] &= ~value;
            if (!(m_colorRegisterValue & 0x04))
                m_planes[1][addr] &= ~value;
            if (!(m_colorRegisterValue & 0x08))
                m_planes[2][addr] &= ~value;
        }
    }
}


uint8_t KorvetGraphicsAdapter::readByte(int addr)
{
    addr &= 0x3FFF;
    addr += m_rwPage * 0x4000;

    uint8_t res;
    if (m_colorRegisterValue & 0x80) {
        // color mode
        res = m_colorRegisterValue & 0x10 ? ~m_planes[0][addr] : m_planes[0][addr];
        res |= m_colorRegisterValue & 0x20 ? ~m_planes[1][addr] : m_planes[1][addr];
        res |= m_colorRegisterValue & 0x40 ? ~m_planes[2][addr] : m_planes[2][addr];
    } else {
        // plane mode
        res = 0;
        if (m_colorRegisterValue & 0x10)
            res |= m_planes[0][addr];
        if (m_colorRegisterValue & 0x20)
            res |= m_planes[1][addr];
        if (m_colorRegisterValue & 0x40)
            res |= m_planes[2][addr];
    }

    return res;
}


KorvetTextAdapter::KorvetTextAdapter()
{
    m_symbols = new uint8_t[1024];
    memset(m_symbols, 0, 1024);
    m_attrs = new uint8_t[1024];
    memset(m_attrs, 0, 1024);
}


KorvetTextAdapter::~KorvetTextAdapter()
{
    delete[] m_symbols;
    delete[] m_attrs;
}


void KorvetTextAdapter::writeByte(int addr, uint8_t value)
{
    addr &= 0x03FF;

    m_symbols[addr] = value;
    switch (m_attrMask) {
    case 0:
        break;
    case 1:
        m_attrs[addr] = 0xFF;
        break;
    case 2:
        m_attrs[addr] = 0x00;
        break;
    case 3:
        m_attrs[addr] = m_curAttr; // review!
        break;
    }
}


uint8_t KorvetTextAdapter::readByte(int addr)
{
    addr &= 0x03FF;

    if (m_attrMask == 3)
        m_curAttr = m_attrs[addr];

    return m_symbols[addr];
}


KorvetKeyboard::KorvetKeyboard()
{
    KorvetKeyboard::resetKeys();
}


void KorvetKeyboard::resetKeys()
{
    for (int i = 0; i < 8; i++)
        m_keys1[i] = 0;

    for (int i = 0; i < 3; i++)
        m_keys2[i] = 0;

    m_mask1 = 0;
    m_mask2 = 0;
}

void KorvetKeyboard::processKey(EmuKey key, bool isPressed)
{
    if (key == EK_NONE)
        return;

    int i, j;
    bool found1 = false;
    bool found2 = false;

    // Main matrix
    for (i = 0; i < 8; i++)
        for (j = 0; j < 8; j++)
            if (key == m_keyMatrix1[i][j]) {
                found1 = true;
                goto found;
            }

    // Additional matrix
    for (i = 0; i < 3; i++)
        for (j = 0; j < 8; j++)
            if (key == m_keyMatrix2[i][j]) {
                found2 = true;
                goto found;
            }

    return;

    found:

    if (found1) {
        if (isPressed)
            m_keys1[i] |= (1 << j);
        else
            m_keys1[i] &= ~(1 << j);
    } else if (found2) {
        if (isPressed)
            m_keys2[i] |= (1 << j);
        else
            m_keys2[i] &= ~(1 << j);
    }
}


uint8_t KorvetKeyboard::getMatrix1Data()
{
    uint8_t val = 0;
    uint8_t mask = m_mask1;
    for (int i = 0; i < 8; i++) {
        if (mask & 1)
            val |= m_keys1[i];
        mask >>= 1;
    }

    return val;
}


uint8_t KorvetKeyboard::getMatrix2Data()
{
    uint8_t val = 0;
    uint8_t mask = m_mask2;
    for (int i = 0; i < 3; i++) {
        if (mask & 1)
            val |= m_keys2[i];
        mask >>= 1;
    }

    return val;
}


EmuKey KorvetKbdLayout::translateKey(PalKeyCode keyCode)
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
    case PK_DOWN:
        return m_downAsNumpad5 ? EK_MENU : EK_DOWN;
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
    case PK_F9:
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


EmuKey KorvetKbdLayout::translateUnicodeKey(unsigned unicodeKey, PalKeyCode keyCode, bool& shift, bool& lang)
{
    if (keyCode == PK_KP_MUL || keyCode == PK_KP_DIV || keyCode == PK_KP_MINUS)
        return EK_NONE;

    if (unicodeKey >= L'A' && unicodeKey <= L'Z')
        unicodeKey += 0x20; // uppercase latin to lowercase
    else if (unicodeKey >= L'a' && unicodeKey <= L'z')
        unicodeKey -= 0x20; // lowercase latin to uppercase
    else if (unicodeKey >= L'А' && unicodeKey <= L'Я')
        unicodeKey += 0x20; // uppercase cyrillic to lowercase
    else if (unicodeKey >= L'а' && unicodeKey <= L'я')
        unicodeKey -= 0x20; // lowercase cyrillic to uppercase

    EmuKey key = translateCommonUnicodeKeys(unicodeKey, shift, lang);
    if (unicodeKey == L'@')
        shift = false;
    else if (unicodeKey == L'`') {
        key = EK_AT;
        shift = true;
        lang = false;
    } else if (unicodeKey == L'_') {
        key = EK_UNDSCR;
        shift = true;
        lang = false;
    }
    return key;
}


bool KorvetKbdLayout::processSpecialKeys(PalKeyCode keyCode)
{
    if (keyCode == PK_F11) {
        m_platform->getKeyboard()->disableKeysReset();
        m_platform->reset();
        m_platform->getKeyboard()->enableKeysReset();
        return true;
    }
    return false;
}


bool KorvetKeyboardRegisters::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "keyboard") {
        m_keyboard = static_cast<KorvetKeyboard*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


uint8_t KorvetKeyboardRegisters::readByte(int addr)
{
    addr &= 0x1FF;
    if (addr < 0x100) {
        m_keyboard->setMatrix1Mask(addr);
        return m_keyboard->getMatrix1Data();
    } else {
        m_keyboard->setMatrix2Mask(addr & 7);
        return m_keyboard->getMatrix2Data();
    }
}


KorvetFddMotor::KorvetFddMotor()
{
    pause();
    EmuObject::setFrequency(1); // 1 s
}


void KorvetFddMotor::on()
{
    resume();
    syncronize();
    m_curClock += m_kDiv * 3; // 3 s
}


void KorvetFddMotor::operate()
{
    pause();
    m_core->int7();
}


bool KorvetFddMotor::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "core") {
        m_core = static_cast<KorvetCore*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


void KorvetPit8253SoundSource::tuneupPit()
{
    m_pit->getCounter(2)->setExtClockMode(true);
}


void KorvetPit8253SoundSource::updateStats()
{
    if (m_pit) {
        m_pit->getCounter(0)->updateState();
        if (m_gate)
            m_sumValue += m_pit->getCounter(0)->getAvgOut();
        }
}


int KorvetPit8253SoundSource::calcValue()
{
    updateStats();
    int res = m_sumValue;
    m_sumValue = 0;

    for (int i = 0; i < 3; i++)
        m_pit->getCounter(i)->resetStats();

    return res * m_ampFactor;
}


void KorvetPit8253SoundSource::setGate(bool gate)
{
    updateStats();
    m_gate = gate;
}


int KorvetCpuCycleWaits::getCpuCycleWaitStates(int memTag, bool /*write*/)
{
    return memTag ? 2 : 0;
}


bool KorvetKbdLayout::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (KbdLayout::setProperty(propertyName, values))
        return true;

    if (propertyName == "downAsNumpad5") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_downAsNumpad5 = values[0].asString() == "yes";
            return true;
        }
    }

    return false;
}


string KorvetKbdLayout::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = KbdLayout::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "downAsNumpad5")
        return m_downAsNumpad5 ? "yes" : "no";

    return "";
}


bool KorvetPpiPsgAdapter::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (Ppi8255Circuit::setProperty(propertyName, values))
        return true;

    if (propertyName == "psg") {
        m_psg = static_cast<Psg3910*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


uint8_t KorvetPpiPsgAdapter::getPortA()
{
    return m_read;
}


void KorvetPpiPsgAdapter::setPortA(uint8_t value)
{
    m_write = value;
}


void KorvetPpiPsgAdapter::setPortB(uint8_t value)
{
    bool bdir = value & 0x80;
    bool bc1 = value & 0x40;

    if (!m_strobe && (bdir || bc1)) {
        m_strobe = true;
        if (!bdir && bc1)
            m_read = m_psg->readByte(0);
        else if (bdir && !bc1)
            m_psg->writeByte(0, m_write);
        else // if (bdir && bc1)
            m_psg->writeByte(1, m_write & 15);
    } else
        m_strobe = false;
}
