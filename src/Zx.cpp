/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2025
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

#include "Globals.h"
#include "Memory.h"
#include "Zx.h"
#include "Emulation.h"
#include "EmuWindow.h"
#include "Platform.h"
#include "CpuZ80.h"
#include "Psg3910.h"
#include "Fdc1793.h"
#include "TapeRedirector.h"
#include "CpuHook.h"
#include "GenericModules.h"
#include "WavReader.h"
#include "GeneralSound.h"

using namespace std;


void ZxCore::draw()
{
    m_window->drawFrame(m_crtRenderer->getPixelData());
    m_window->endDraw();
}


void ZxCore::setInt(bool state)
{
    // todo: transfer to CPU implementation
    m_intReq = state;

    if (!state)
        return;

    CpuZ80* cpu = static_cast<CpuZ80*>(m_platform->getCpu());

    if (cpu->getInte()) {
        m_intReq = false;
        cpu->intRst(7);
    }
}


void ZxCore::initConnections()
{
    PlatformCore::initConnections();

    REG_INPUT("int", ZxCore::setInt);
}


void ZxCore::attachCrtRenderer(CrtRenderer* crtRenderer)
{
    m_crtRenderer = crtRenderer;
}


bool ZxCore::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (PlatformCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<ZxRenderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }
    return false;
}


void ZxPorts::reset()
{
    m_bdiActive = false;
}


void ZxPorts::initConnections()
{
    AddressableDevice::initConnections();

    REG_INPUT("kbdMatrixData", ZxPorts::setKbdMatrixData);
    REG_INPUT("bdiActive", ZxPorts::setBdiActive);

    m_kbdMaskOutput = registerOutput("kbdMatrixMask");
    m_portFEOutput = registerOutput("portFE");
    m_port7FFDOutput = registerOutput("port7FFD");
}


void ZxPorts::setKbdMatrixData(int data)
{
    m_kbdMatrixData = data;
}


uint8_t ZxPorts::readByte(int addr)
{
    if (!(addr & 1)) {
        // port FE
        int kbdMatrixMask = (addr >> 8) & 0xFF;
        m_kbdMaskOutput->setValue(kbdMatrixMask);
        // recieve kbdData from Keyboard here
        return (m_kbdMatrixData & 0x1F) + (g_emulation->getWavReader()->getCurValue() ? 0x40 : 0x00);
    } else if (!(addr & 2)) {
        // port FD
        if (!m_ay)
            return 0xFF;
        addr >>= 8;
        if (addr == 0xFF)
            return m_ay->readByte(0);
        else
            return 0xFF;
    } else if (m_gs && (addr & 0xf7) == 0xb3) {
        return m_gs->readByte((addr >> 3) & 1);
    } else {
        if (m_bdiActive && m_fdc && (addr & 3) == 3) {
            if (addr & 0x80)
                return m_fddRegister->readByte((addr >> 5) & 3);
            else
                return m_fdc->readByte((addr >> 5) & 3);
        }
    }

    return 0xff;
}


void ZxPorts::writeByte(int addr, uint8_t value)
{
    if (!(addr & 1)) {
        // port FE
        m_portFEOutput->setValue(value);
    } else if (!(addr & 2)) {
        // port FD

        addr >>= 8;

        if ((addr & 0xC0) == 0x40) {
            // 128K FD register
            if (m_128kMode)
                m_port7FFDOutput->setValue(value);
        } else if ((addr & 0xC0) == 0xC0) {
            // AY address reg
            if (m_ay)
                m_ay->writeByte(1, value);
        } else if ((addr & 0xC0) == 0x80) {
            // AY data register
            if (m_ay)
                m_ay->writeByte(0, value);
        }
        /*if (m_128kMode && (addr & 0x20)) {
            // 48K mode
            m_ramPageOutput->setValue(0); // ?
            m_romPageOutput->setValue(1);
            m_screenPageOutput->setValue(0);
            m_128kMode = false;
        }*/
    } else if (m_gs && (addr & 0xf7) == 0xb3) {
        m_gs->writeByte((addr >> 3) & 1, value);
    } else {
        if (m_bdiActive && m_fdc && (addr & 3) == 3) {
            if (addr & 0x80)
                m_fddRegister->writeByte((addr >> 5) & 3, value);
            else
                m_fdc->writeByte((addr >> 5) & 3, value);
        }
    }
}


bool ZxPorts::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "ay") {
        m_ay = static_cast<AddressableDevice*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "fdc") {
        m_fdc = static_cast<Fdc1793*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "fddRegister") {
        m_fddRegister = static_cast<Register*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "gsPorts") {
        m_gs = static_cast<GsPorts*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "mode") {
        if (values[0].asString() == "128k" || values[0].asString() == "48k" || values[0].asString() == "pentagon") {
            m_128kMode = values[0].asString() != "48k";
            return true;
        }
        return false;
    }
    return false;
}


ZxKeyboard::ZxKeyboard()
{
    ZxKeyboard::resetKeys();
}


void ZxKeyboard::initConnections()
{
    Keyboard::initConnections();

    REG_INPUT("matrixMask", ZxKeyboard::setMatrixMask);
    m_dataOutput = registerOutput("matrixData");
}


void ZxKeyboard::resetKeys()
{
    for (int i = 0; i < 8; i++)
        m_keys[i] = 0;

    m_mask = 0;
}


ZxRenderer::ZxRenderer()
{
    const int pixelFreq = 7; // MHz
    const int maxBufSize = 365 * 288; // 365 = 704 / 13.5 * 7 (pixelFreq)

    m_sizeX = m_prevSizeX = 256;
    m_sizeY = m_prevSizeY = 192;
    m_aspectRatio = m_prevAspectRatio = 5184. / 704 / pixelFreq;
    m_bufSize = m_prevBufSize = m_sizeX * m_sizeY;
    m_pixelData = new uint32_t[maxBufSize];
    m_prevPixelData = new uint32_t[maxBufSize];
    memset(m_pixelData, 0, m_bufSize * sizeof(uint32_t));
    memset(m_prevPixelData, 0, m_prevBufSize * sizeof(uint32_t));

    m_fullFrame = new uint32_t[320 * 456];
    memset(m_fullFrame, 0, 320 * 456 * sizeof(uint32_t));

    m_ticksPerTState = g_emulation->getFrequency() * 2 / 7000000;

    m_flashCnt = 0;
}


ZxRenderer::~ZxRenderer()
{
    if (m_fullFrame)
        delete[] m_fullFrame;
}


void ZxRenderer::renderFrame()
{
    swapBuffers();

    if (m_showBorder) {
        m_sizeX = 365;
        m_sizeY = 288;
        m_aspectRatio = double(m_sizeY) * 4 / 3 / m_sizeX;

        for (int i = 0; i < 288; i++)
            memcpy(m_pixelData + m_sizeX * i, m_fullFrame + (i + m_vOffset /*m_visibleScanLine - 40*/) * m_linePixels - 50/*73*/, m_sizeX * 4);

    } else {
        m_sizeX = 256;
        m_sizeY = 192;
        m_aspectRatio = 576.0 * 9 / 704 / 7;

        for (int i = 0; i < 192; i++)
            memcpy(m_pixelData + m_sizeX * i, m_fullFrame + ((i + m_visibleScanLine) * m_linePixels), m_sizeX * 4);
    }

    m_flashCnt = (m_flashCnt + 1) & 0x1F;
}


void ZxRenderer::operate()
{
    advanceTo(m_curClock + 8 * m_ticksPerTState);

    if (m_intActive) {
        m_intActive = false;
        m_intOutput->setValue(false);
        m_curClock += m_ticksPerTState * (m_lineTStates - 32);
        return;
    }

    m_curScanLine = (m_curScanLine + 1) % m_scanLines;

    if (m_curScanLine == 0) {
        m_intActive = true;
        m_intOutput->setValue(true);
        m_curFrameClock = m_curClock;
        m_curClock += m_ticksPerTState * 32;
        renderFrame();
        g_emulation->screenUpdateReq();
        return;
    }

    m_curClock += m_ticksPerTState * m_lineTStates;
}


void ZxRenderer::advanceTo(uint64_t clocks)
{
    int toFrameTState = (int64_t(clocks) - int64_t(m_curFrameClock)) / m_ticksPerTState + m_bias;

    if (toFrameTState <= m_curFrameTState)
        return;

    int scanLine = m_curFrameTState / m_lineTStates;
    int toTState = (toFrameTState + m_lineTStates - 1) % m_lineTStates;
    int fromTState = m_curFrameTState % m_lineTStates;

    if (fromTState <= toTState)
        drawLine(scanLine, fromTState, toTState);
    else {
        drawLine(scanLine, fromTState, m_lineTStates - 1);
        if (scanLine != m_scanLines - 1)
            drawLine(scanLine + 1, 0, toTState);
    }

    m_curFrameTState = toFrameTState % (m_lineTStates * m_scanLines);
}


void ZxRenderer::drawLine(int scanLine, int fromTState, int toTState)
{
    int fromByte = fromTState / 4;
    int toByte = toTState / 4;

    uint8_t bt = m_savedBt;
    uint8_t attr = m_savedAttr;

    uint32_t fgColor = 0;
    uint32_t bgColor = 0;

    bool firstPixel = fromTState % 4 == 0;

    for (int col = fromByte; col <= toByte; col++) {
        if (scanLine < m_visibleScanLine || scanLine >= m_visibleScanLine + 192 || col >= 32) {
            // border
            bt = 0;
            bgColor = zxPalette[m_borderColor];
        } else {
            if (col != fromByte || firstPixel) {
                int row = scanLine - m_visibleScanLine;
                int addr = (((row & 0xC0) << 5) | ((row & 0x07) << 8) | ((row & 0x38) << 2)) + col;
                bt = m_screenMemory[m_screenPage][addr];
                attr = m_screenMemory[m_screenPage][0x1800 + (row / 8 * 32) + col];
                m_savedBt = bt;
                m_savedAttr = attr;
            }

            bool inv = (attr & 0x80) && (m_flashCnt & 0x10);
            if (!inv) {
                fgColor = zxPalette[(attr & 7) + ((attr & 0x40) >> 3)];
                bgColor = zxPalette[((attr & 0x38) >> 3) + ((attr & 0x40) >> 3)];
            } else {
                bgColor = zxPalette[(attr & 7) + ((attr & 0x40) >> 3)];
                fgColor = zxPalette[((attr & 0x38) >> 3) + ((attr & 0x40) >> 3)];
            }
        }

        int p1 = 0;
        int p2 = 7;

        if (m_model == ZM_PENTAGON) {
            if (col == fromByte)
                p1 = fromTState % 4 * 2;
            if (col == toByte)
                p2 = toTState % 4 * 2 + 1;
        }

        bt <<= p1;

        for (int p = p1; p <= p2; p++) {
            uint32_t color = (bt & 0x80) ? fgColor : bgColor;
            m_fullFrame[scanLine * m_linePixels + col * 8 + p] = color;
            bt <<= 1;
        }
    }
}


void ZxRenderer::initConnections()
{
    CrtRenderer::initConnections();

    m_intOutput = registerOutput("int");
    REG_INPUT("borderColor", ZxRenderer::setBorderColor);
    REG_INPUT("screenPage", ZxRenderer::setScreenPage);
}


void ZxRenderer::setBorderColor(uint8_t color)
{
    int shift = m_cpu->getCurIoInstructionDuration() - 1;
    advanceTo(g_emulation->getCurClock() + shift * m_ticksPerTState);
    m_borderColor = color;
}


void ZxRenderer::setScreenPage(int screenPage)
{
    if (screenPage >=0 && screenPage <= 1) {
        int shift = m_cpu->getCurIoInstructionDuration() - 1;
        advanceTo(g_emulation->getCurClock() + shift * m_ticksPerTState);
        m_screenPage = screenPage;
    }
}


void ZxRenderer::vidMemWriteNotify(int screenPage)
{
    if (screenPage == m_screenPage) {
        int shift = m_cpu->getCurIoInstructionDuration() - 1;
        advanceTo(g_emulation->getCurClock() + shift * m_ticksPerTState);
    }
}


void ZxRenderer::toggleColorMode()
{
    m_colorMode = !m_colorMode;
}


void ZxRenderer::toggleCropping()
{
    m_showBorder = !m_showBorder;
}


void ZxRenderer::setModel(ZxModel model)
{
    m_model = model;

    switch (model) {
    case ZM_48K:
        m_lineTStates = 224;
        m_linePixels = 448;
        m_scanLines = 312;
        m_visibleScanLine = 64;
        m_bias = 0;
        m_vOffset = 24;
        break;
    case ZM_128K:
        m_lineTStates = 228;
        m_linePixels = 456;
        m_scanLines = 311;
        m_visibleScanLine = 63;
        m_bias = 0;
        m_vOffset = 23;
        break;
    case ZM_PENTAGON:
        m_lineTStates = 224;
        m_linePixels = 448;
        m_scanLines = 320;
        m_visibleScanLine = 81;
        m_bias = 156;
        m_vOffset = 32;
        break;
    default:
        break;
    }
}


bool ZxRenderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (CrtRenderer::setProperty(propertyName, values))
        return true;

    if (propertyName == "screenMemory") {
        int n = values[1].asInt();
        if (n >= 0 && n <= 1) {
            m_screenMemory[n] = static_cast<Ram*>(g_emulation->findObject(values[0].asString()))->getDataPtr();
            return true;
        }
        return false;
    } else if (propertyName == "visibleArea") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_showBorder = values[0].asString() == "yes";
            return true;
        }
    } else if (propertyName == "mode") {
        if (values[0].asString() == "48k")
            setModel(ZM_48K);
        else if (values[0].asString() == "128k")
            setModel(ZM_128K);
        else if (values[0].asString() == "pentagon")
            setModel(ZM_PENTAGON);
        else
            return false;
        return true;
    } else if (propertyName == "colorMode") {
        if (values[0].asString() == "mono")
            m_colorMode = false;
        else if (values[0].asString() == "color")
            m_colorMode = true;
        else
            return false;
        return true;
    } else if (propertyName == "cpu") {
        m_cpu = static_cast<CpuZ80*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


string ZxRenderer::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "visibleArea") {
        return m_showBorder ? "yes" : "no";
    } else if (propertyName == "crtMode") {
        return u8"256\u00D7192@50Hz" ;
    } else if (propertyName == "colorMode") {
        return m_colorMode ? "color" : "mono";
    }

    return "";
}


string ZxRenderer::getDebugInfo()
{
    uint64_t clocks = m_cpu->getClock();
    //uint64_t clocks = m_platform->getCpu()->getClock();
    advanceTo(clocks  + 8 * m_ticksPerTState);

    int ticksPerTState = g_emulation->getFrequency() / 3500000;

    int tState = (int64_t(clocks) - int64_t(m_curFrameClock)) / ticksPerTState;
    int scanLine = tState / 224;
    int pos = (tState + 223) % 224;


    stringstream ss;
    ss << "CRT:" << "\n";
    ss << "T-State:" << tState << "\n";
    ss << "L:" << scanLine;
    ss << " P:" << pos << "\n";
    return ss.str();
}


void ZxBdiAddrSpace::initConnections()
{
    AddrSpace::initConnections();
    m_bdiActiveOutput = registerOutput("bdiActive");
}


void ZxBdiAddrSpace::reset()
{
    m_bdiActive = false;
    m_bdiActiveOutput->setValue(0);
}


uint8_t ZxBdiAddrSpace::readByte(int addr)
{
    if (m_cpu->getM1Status()) {
        if (!m_bdiActive && addr >> 8 == 0x3D) {
            m_bdiActive = true;
            m_bdiActiveOutput->setValue(1);
        } else if (m_bdiActive && addr >= 0x4000) {
            m_bdiActive = false;
            m_bdiActiveOutput->setValue(0);
        }
    }

    return AddrSpace::readByte(addr);
}


bool ZxBdiAddrSpace::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddrSpace::setProperty(propertyName, values))
        return true;

    if (propertyName == "cpu") {
        m_cpu = static_cast<CpuZ80*>(g_emulation->findObject(values[0].asString()));
        return true;
    }
    return false;
}


void ZxKeyboard::processKey(EmuKey key, bool isPressed)
{
    if (key == EK_NONE)
        return;

    int i, j;

    // Main matrix
    for (i = 0; i < 8; i++)
        for (j = 0; j < 5; j++)
            if (key == m_keyMatrix[i][j]) {
                goto found;
            }
    return;

    found:
    if (isPressed)
        m_keys[i] |= (1 << j);
    else
        m_keys[i] &= ~(1 << j);
}



void ZxKeyboard::setMatrixMask(uint8_t mask)
{
    mask = ~mask;

    uint8_t val = 0;
    for (int i = 0; i < 8; i++) {
        if (mask & 1)
            val |= m_keys[i];
        mask >>= 1;
    }

    m_dataOutput->setValue(~val);
}


EmuKey ZxKbdLayout::translateKey(PalKeyCode keyCode)
{
    if (keyCode == PK_LCTRL || keyCode == PK_RCTRL)
        return EK_CTRL;

    switch (keyCode) {
    case PK_F1:
        keyCode = PK_1;
        break;
    case PK_F2:
        keyCode = PK_2;
        break;
    case PK_F3:
        keyCode = PK_3;
        break;
    case PK_F4:
        keyCode = PK_5;
        break;
    case PK_F5:
        keyCode = PK_5;
        break;
    case PK_F6:
        keyCode = PK_6;
        break;
    case PK_F7:
        keyCode = PK_7;
        break;
    case PK_F8:
        keyCode = PK_8;
        break;
    case PK_F9:
        keyCode = PK_9;
        break;
    case PK_F10:
        keyCode = PK_0;
        break;
    default:
        break;
    }

    EmuKey key = translateCommonKeys(keyCode);
    if (key != EK_NONE)
        return key;

    return EK_NONE;
}


EmuKey ZxKbdLayout::translateUnicodeKey(unsigned unicodeKey, PalKeyCode keyCode, bool& shift, bool& lang, bool& ctrl)
{
    switch(keyCode) {
    case PK_UP:
        shift = true;
        return EK_7;
    case PK_DOWN:
        shift = true;
        return EK_6;
    case PK_LEFT:
        shift = true;
        return EK_5;
    case PK_RIGHT:
        shift = true;
        return EK_8;
    case PK_TAB:
        ctrl = true;
        return EK_SPACE;
    case PK_BSP:
        shift = true;
        return EK_0;
    default:
        break;
    }

    switch(unicodeKey) {
    case L'!':
        ctrl = true;
        return EK_1;
    case L'@':
        ctrl = true;
        return EK_2;
    case L'#':
        ctrl = true;
        return EK_3;
    case L'$':
        ctrl = true;
        return EK_4;
    case L'%':
        ctrl = true;
        return EK_5;
    case L'&':
        ctrl = true;
        return EK_6;
    case L'\'':
        ctrl = true;
        return EK_7;
    case L'(':
        ctrl = true;
        return EK_8;
    case L')':
        ctrl = true;
        return EK_9;
    case L'_':
        ctrl = true;
        return EK_0;
    case L'<':
        ctrl = true;
        return EK_R;
    case L'>':
        ctrl = true;
        return EK_T;
    case L';':
        ctrl = true;
        return EK_O;
    case L'\"':
        ctrl = true;
        return EK_P;
    case L'^':
        ctrl = true;
        return EK_H;
    case L'-':
        ctrl = true;
        return EK_J;
    case L'+':
        ctrl = true;
        return EK_K;
    case L'=':
        ctrl = true;
        return EK_L;
    case L':':
        ctrl = true;
        return EK_Z;
    case L'?':
        ctrl = true;
        return EK_C;
    case L'/':
        ctrl = true;
        return EK_V;
    case L'*':
        ctrl = true;
        return EK_B;
    case L',':
        ctrl = true;
        return EK_N;
    case L'.':
        ctrl = true;
        return EK_M;
    }

    return translateCommonUnicodeKeys(unicodeKey, shift, lang);
}


bool ZxKbdLayout::translateKeyEx(PalKeyCode keyCode, EmuKey &key1, EmuKey &key2)
{
    switch(keyCode) {
    case PK_UP:
        key1 = EK_SHIFT;
        key2 = EK_7;
        return true;
    case PK_DOWN:
        key1 = EK_SHIFT;
        key2 = EK_6;
        return true;
    case PK_LEFT:
        key1 = EK_SHIFT;
        key2 = EK_5;
        return true;
    case PK_RIGHT:
        key1 = EK_SHIFT;
        key2 = EK_8;
        return true;
    case PK_F12:
        key1 = EK_CTRL;
        key2 = EK_SPACE;
        return true;
    case PK_BSP:
        key1 = EK_SHIFT;
        key2 = EK_0;
        return true;
    case PK_COMMA:
        key1 = EK_CTRL;
        key2 = EK_N;
        return true;
    case PK_PERIOD:
        key1 = EK_CTRL;
        key2 = EK_M;
        return true;
    case PK_APOSTROPHE:
        key1 = EK_CTRL;
        key2 = EK_P;
        return true;
    case PK_SEMICOLON:
        key1 = EK_CTRL;
        key2 = EK_O;
        return true;
    case PK_SLASH:
        key1 = EK_CTRL;
        key2 = EK_V;
        return true;
    case PK_TILDE:
        key1 = EK_CTRL;
        key2 = EK_7;
        return true;
    case PK_MINUS:
        key1 = EK_CTRL;
        key2 = EK_J;
        return true;
    case PK_EQU:
        key1 = EK_CTRL;
        key2 = EK_L;
        return true;
    default:
        break;
    }

    return false;
}


bool ZxKbdLayout::processSpecialKeys(PalKeyCode keyCode, bool pressed)
{
    if (!pressed)
        return false;

    if (keyCode == PK_F11) {
        m_platform->reset();
        static_cast<Cpu8080Compatible*>(m_platform->getCpu())->setPC(0);
        return true;
    } else if (keyCode == PK_F12) {
        m_platform->reset();
        //static_cast<Cpu8080Compatible*>(m_platform->getCpu())->getIoAddrSpace()->writeByte(0xFFFD, );
        Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_platform->getCpu());
        g_emulation->exec((int64_t)cpu->getKDiv() * 6000000);

        Keyboard* kbd = m_platform->getKeyboard();

        for (int i = 0; i < 3; i++) {
            kbd->processKey(EK_SHIFT, true);
            kbd->processKey(EK_6, true);
            g_emulation->exec((int64_t)cpu->getKDiv() * 100000, true);
            kbd->processKey(EK_SHIFT, false);
            kbd->processKey(EK_6, false);
            g_emulation->exec((int64_t)cpu->getKDiv() * 350000, false);
        }

        // Press CR
        kbd->processKey(EK_CR, true);
        g_emulation->exec((int64_t)cpu->getKDiv() * 100000, true);
        kbd->processKey(EK_CR, false);

        g_emulation->exec((int64_t)cpu->getKDiv() * 6000000);

        AddressableDevice* as = cpu->getAddrSpace();
        as->writeByte(0xfff6, 0);
        as->writeByte(0xfff7, 0);
        cpu->setSP(0xfff6);

        cpu->setPC(0x3d00);
        return true;
    }
    return false;
}


bool ZxTapeInHook::hookProc()
{
    if (!m_isEnabled)
        return false;

    if (g_emulation->getWavReader()->isPlaying())
        return false;

    if (!m_file->isOpen())
        m_file->openFile();

    if (m_file->isCancelled())
        return false;

    CpuZ80* cpu = static_cast<CpuZ80*>(m_cpu);

    uint16_t addr = cpu->getIX();
    uint16_t len = cpu->getDE();
    bool verify = !(cpu->getAF2() & 1);

    uint16_t af = cpu->getAF2() & ~1; // clear C (error)

    if (m_file->isOpen() && !m_file->isEof() && (m_file->isTap() || m_file->isTzx()))
    {
        int blockSize = m_file->advanceToNextBlock();
        if (len > blockSize - 2)
            len = blockSize - 2;
        m_file->readByte(); // flag
        for (int i = addr; i < addr + len; i++) {
            uint8_t inByte = m_file->readByte();
            if (!verify)
                cpu->getAddrSpace()->writeByte(i, inByte);
        }
        m_file->readByte(); // CS
        af |= 1; // set C (success)
        cpu->setIX((addr + len));
    }

    cpu->setAF((af));

    static_cast<Cpu8080Compatible*>(m_cpu)->ret();

    if (m_file->isEof())
        m_file->closeFile();

    return true;
}


bool ZxTapeOutHook::hookProc()
{
    if (!m_isEnabled || (m_hasSignature && !checkSignature()))
        return false;

    if (m_file->isCancelled())
        return false;

    if (!m_file->isOpen())
        m_file->openFile();

    if (m_file->isCancelled())
        return false;

    CpuZ80* cpu = static_cast<CpuZ80*>(m_cpu);

    uint16_t addr = cpu->getIX();
    uint16_t len = cpu->getDE();

    m_file->writeByte((len + 2) & 0xFF);
    m_file->writeByte((len + 2) >> 8);

    m_file->writeByte(cpu->getAF() >> 8);

    uint8_t cs = 0;

    for (unsigned i = 0; i < len; i++) {
        uint8_t bt = cpu->getAddrSpace()->readByte(addr + i);
        m_file->writeByte(bt);
        cs ^= bt;
    }

    m_file->writeByte(cs);

    cpu->ret();

    return true;
}


bool ZxFileLoader::loadFile(const std::string& fileName, bool /*run*/)
{
    int fileSize;
    uint8_t* buf = palReadFile(fileName, fileSize, false);
    if (!buf)
        return false;

    TapeFileParser parser(buf, fileSize);

    auto fileFormat = parser.getFormat();

    if (fileFormat != TapeFileParser::Format::MF_TZX && fileFormat != TapeFileParser::Format::MF_TAP)
        return false;

    m_platform->reset();
    CpuZ80* cpu = dynamic_cast<CpuZ80*>(m_platform->getCpu());

    g_emulation->exec(int64_t(cpu->getKDiv()) * 6000000, true);

    Keyboard* kbd = m_platform->getKeyboard();

    // Press J
    kbd->processKey(EK_J, true);
    g_emulation->exec((int64_t)cpu->getKDiv() * 100000, true);
    kbd->processKey(EK_J, false);
    g_emulation->exec((int64_t)cpu->getKDiv() * 100000, true);

    // Press SS
    kbd->processKey(EK_CTRL, true);

    // Press P twice
    for (int i = 0; i < 2; i++) {
        kbd->processKey(EK_P, true);
        g_emulation->exec((int64_t)cpu->getKDiv() * 300000, true);
        kbd->processKey(EK_P, false);
        g_emulation->exec((int64_t)cpu->getKDiv() * 300000, true);
    }

    // Release SS
    kbd->processKey(EK_CTRL, false);

    if (!m_tapeRedirector)
        return false;

    m_tapeRedirector->assignFile(fileName, "r");
    m_tapeRedirector->openFile();
    m_tapeRedirector->assignFile("", "r");

    // Press CR
    kbd->processKey(EK_CR, true);
    g_emulation->exec((int64_t)cpu->getKDiv() * 100000, true);
    kbd->processKey(EK_CR, false);

    return true;
}


bool ZxFileLoader::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (FileLoader::setProperty(propertyName, values))
        return true;

    return false;
}


bool ZxVidMemAdapter::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "renderer") {
        m_renderer = static_cast<ZxRenderer*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "mem") {
        m_mem = static_cast<AddressableDevice*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "screenPage" /*&& m_supportsTags*/ && values[0].isInt()) {
        m_screenPage = values[0].asInt();
        return true;
    }

    return false;
}


void ZxVidMemAdapter::writeByte(int addr, uint8_t value)
{
    m_renderer->vidMemWriteNotify(m_screenPage);
    m_mem->writeByte(addr, value);
}


/*void ZxCpuWaits::initConnections()
{
    CpuWaits::initConnections();

    REG_INPUT("int", ZxCpuWaits::setInt);
}


int ZxCpuWaits::getCpuWaitStates(int memTag, int opcode, int normalClocks)
{
    static const int waits[8] = {6, 5, 4, 3, 2, 1, 0, 0};

    if (memTag || (opcode & 0xC7FF) == 0x41ED) {
        // OUT (c), r
        int kDiv = g_emulation->getFrequency() / 3500000; // !!! 2do: replace with cpu->freq
        //int outTime = (g_emulation->getCurClock() - m_lastIntTime) / kDiv + 12 - 2;
        int outTime = (g_emulation->getCurClock() - m_lastIntTime) / kDiv + 1;

        int scanLine = outTime / 224;
        if (scanLine < 64 || scanLine >= 256)
            return 0;

        int linePos = outTime % 224;

        if (linePos > 127)
            return 0;

        return waits[linePos % 8];
    }

    return 0;
}


void ZxCpuWaits::setInt(int)
{
    m_lastIntTime = g_emulation->getCurClock();
}*/
