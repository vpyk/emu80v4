/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2025
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
#include <algorithm>

//#include "Pal.h"

#include "Okean.h"
#include "Emulation.h"
#include "Globals.h"
#include "EmuWindow.h"
#include "Platform.h"
#include "Memory.h"
//#include "AddrSpace.h"
#include "Fdc1793.h"
//#include "SoundMixer.h"
//#include "WavReader.h"
#include "Cpu.h"
#include "Pic8259.h"
//#include "Pit8253.h"

using namespace std;


void OkeanCore::draw()
{
    //m_crtRenderer->renderFrame();
    m_window->drawFrame(m_crtRenderer->getPixelData());
    m_window->endDraw();
}


void OkeanCore::inte(bool isActive)
{
    m_pic->inte(isActive);
}


bool OkeanCore::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (PlatformCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        m_crtRenderer = static_cast<CrtRenderer*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "pic") {
        m_pic = static_cast<Pic8259*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


OkeanRenderer::OkeanRenderer()
{
    m_defaultDebugRendering = false;

    m_frameBuf = new uint32_t[512 * 256];
    memset(m_frameBuf, 0, 512 * 256 * sizeof(uint32_t));

    m_sizeX = m_prevSizeX = 512;
    m_sizeY = m_prevSizeY = 256;
    m_aspectRatio = m_prevAspectRatio = 576.0 * 9 / 704 / 12;
    m_bufSize = m_prevBufSize = m_sizeX * m_sizeY;
    int maxBufSize = 626 * 288;
    m_pixelData = new uint32_t[maxBufSize];
    m_prevPixelData = new uint32_t[maxBufSize];
    memset(m_pixelData, 0, m_bufSize * sizeof(uint32_t));
    memset(m_prevPixelData, 0, m_prevBufSize * sizeof(uint32_t));
}


OkeanRenderer::~OkeanRenderer()
{
    delete[] m_frameBuf;
}


void OkeanRenderer::initConnections()
{
    CrtRenderer::initConnections();

    REG_INPUT("mode", OkeanRenderer::setMode);
    REG_INPUT("page", OkeanRenderer::setPage);
    REG_INPUT("palette", OkeanRenderer::setPalette);
    REG_INPUT("vscroll", OkeanRenderer::setVscroll);
    REG_INPUT("hscroll", OkeanRenderer::setHscroll);
    REG_INPUT("bgColor", OkeanRenderer::setBgColor);

    REG_OUTPUT("vblank", m_vblankOutput);
    REG_OUTPUT("hblank", m_hblankOutput);
}


void OkeanRenderer::toggleColorMode()
{
    switch (m_colorMode) {
    case ColorMode::Color1:
        setColorMode(ColorMode::Color2);
        break;
    case ColorMode::Color2:
        setColorMode(ColorMode::GrayScale);
        break;
    case ColorMode::GrayScale:
        setColorMode(ColorMode::Color1);
    }
}


void OkeanRenderer::renderLine(int line)
{
    if (m_lowResMode) {
        for (int col = 0; col < 32; col++) {
            int addr = col * 512 + (line + m_vscroll) % 256;
            uint8_t bt1 = m_screenMemory[m_page][addr];
            uint8_t bt2 = m_screenMemory[m_page][addr + 256];
            for (int pt = 0; pt < 8; pt++, bt1 >>= 1, bt2 >>= 1) {
                int color = 0;
                color <<= 1;
                color |= (bt2 & 1);
                color <<= 1;
                color |= (bt1 & 1);
                uint32_t* framebufPtr = m_frameBuf + line * 512 + (col * 16 + (pt + m_hscroll) * 2) % 512;
                uint32_t color32 = m_colorPalette[m_palette][color];
                *framebufPtr++ = color32;
                *framebufPtr = color32;
            }
        }
    } else {
        for (int col = 0; col < 64; col++) {
            int addr = col * 256 + line;
            uint8_t bt = m_screenMemory[m_page][addr];
            for (int pt = 0; pt < 8; pt++, bt >>= 1) {
                m_frameBuf[line * 512 + col * 8 + pt] = m_monoPalette[bt & 1][m_palette];
            }
        }
    }
}


void OkeanRenderer::setColorMode(ColorMode colorMode)
{
    m_colorMode = colorMode;
    switch (colorMode) {
    case ColorMode::Color1:
        m_colorPalette = m_colorPalette1;
        m_monoPalette = m_monoPalette1;
        break;
    case ColorMode::Color2:
        m_colorPalette = m_colorPalette2;
        m_monoPalette = m_monoPalette2;
        break;
    case ColorMode::GrayScale:
        m_colorPalette = m_colorPaletteGray;
        m_monoPalette = m_monoPaletteGray;
        break;
    }

}


void OkeanRenderer::renderFrame()
{
    swapBuffers();

    if (m_showBorder) {
        m_sizeX = 626;
        m_sizeY = 288;
        m_offsetX = 60;
        m_offsetY = 26;
        m_aspectRatio = double(m_sizeY) * 4 / 3 / m_sizeX;

        for (int i = 0; i < m_bufSize; i++)
            m_pixelData[i] = m_bgColor;

        memset(m_pixelData, 0, m_sizeX * m_sizeY * sizeof(uint32_t));
        for (int i = 0; i < 256; i++)
            memcpy(m_pixelData + m_sizeX * (i + m_offsetY) + m_offsetX, m_frameBuf + i * 512, 512 * sizeof(uint32_t));
    } else {
        m_sizeX = 512;
        m_sizeY = 256;
        m_offsetX = m_offsetY = 0;
        m_aspectRatio = 576.0 * 9 / 704 / 12;

        memcpy(m_pixelData, m_frameBuf, m_sizeX * m_sizeY * sizeof(uint32_t));
    }
}


void OkeanRenderer::operate()
{
    if (m_hblank) {
        m_curClock += g_emulation->getFrequency() * 256 / 12000000;
        m_hblank = false;
        m_hblankOutput->setValue(0);
        return;
    }

    if (m_curLine >= 64)
        renderLine(m_curLine - 64);

    if (m_curLine == 0) {
        m_vblankOutput->setValue(1);
        renderFrame();
        g_emulation->screenUpdateReq();
    } else if (m_curLine == 64)
        m_vblankOutput->setValue(0);

    if (++m_curLine == 320)
        m_curLine = 0;

    m_curClock += g_emulation->getFrequency() * 512 / 12000000;
    m_hblank = true;
    m_hblankOutput->setValue(1);
}


void OkeanRenderer::setHscroll(uint8_t hscroll)
{
    m_hscroll = ((hscroll & 0xf8) | ((hscroll + 1) & 7)) << 1;
}


void OkeanRenderer::setBgColor(uint8_t bgColor)
{
    m_bgColor = (((bgColor & 4) << 11) | ((bgColor & 2) << 4) | ((bgColor & 1) << 20)) * 3;
    //m_bgColor = (((bgColor & 4) << 21) | ((bgColor & 2) << 14) | ((bgColor & 1) << 7)) * 3;

    m_colorPalette1[0][0] = m_bgColor;
    m_colorPalette1[3][0] = m_bgColor;
    m_colorPalette1[4][0] = m_bgColor;
    m_colorPalette1[5][0] = m_bgColor;

    m_colorPalette2[0][0] = m_bgColor;
    m_colorPalette2[3][0] = m_bgColor;
    m_colorPalette2[4][0] = m_bgColor;
    m_colorPalette2[5][0] = m_bgColor;

    for (int i = 0; i < 8; i++) {
        m_monoPalette1[i == 6 ? 1 : 0][i] = m_bgColor;
        m_monoPalette2[i == 6 ? 1 : 0][i] = m_bgColor;
    }
}


void OkeanRenderer::toggleCropping()
{
    m_showBorder = !m_showBorder;
}


void OkeanRenderer::prepareDebugScreen()
{
    enableSwapBuffersOnce();

    for (int i = 0; i < 256; i++)
        renderLine(i);

    renderFrame();
    g_emulation->screenUpdateReq();
}


bool OkeanRenderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (CrtRenderer::setProperty(propertyName, values))
        return true;

    if (propertyName == "screenMemory") {
        m_screenMemory[0] = static_cast<Ram*>(g_emulation->findObject(values[0].asString()))->getDataPtr() + 0xC000;
        return true;
    } else if (propertyName == "screenMemory2") {
        m_screenMemory[1] = static_cast<Ram*>(g_emulation->findObject(values[0].asString()))->getDataPtr() + 0xC000;
        return true;
    } else if (propertyName == "colorMode") {
        if (values[0].asString() == "color1")
            setColorMode(ColorMode::Color1);
        else if (values[0].asString() == "color2")
            setColorMode(ColorMode::Color2);
        else if (values[0].asString() == "grayscale")
            setColorMode(ColorMode::GrayScale);
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


string OkeanRenderer::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "colorMode") {
        switch (m_colorMode) {
        case ColorMode::Color1:
            return "color1";
        case ColorMode::Color2:
            return "color2";
        case ColorMode::GrayScale:
            return "grayscale";
        }
    } else if (propertyName == "visibleArea") {
        return m_showBorder ? "yes" : "no";
    } /*else if (propertyName == "crtMode") {
            return u8"384\u00D7256@50.08Hz" ;
    }*/

    return "";
}


/*string OkeanRenderer::getDebugInfo()
{
    int line = (m_curLine + 55) % 312;
    stringstream ss;
    ss << "CRT:" << "\n";
    ss << "L: " << line;
    if (line >= 56)
        ss << " /" << (m_curLine + 311) % 312;
    return ss.str();
}*/


EmuKey OkeanKbdLayout::translateKey(PalKeyCode keyCode)
{
    switch (keyCode) {
    case PK_LCTRL:
    case PK_RCTRL:
        return EK_CTRL;
    case PK_F6:
        return EK_HELP;
    case PK_F7:
        return EK_SET;
    case PK_F8:
        return EK_GRAPH;
    case PK_F12:
        return EK_RESET;
    default:
        break;
    }

    return KbdLayout::translateKey(keyCode);
}


void OkeanKeyboard::initConnections()
{
    Keyboard::initConnections();

    REG_INPUT("ack", OkeanKeyboard::ack);

    REG_OUTPUT("strobe", m_strobeOutput);
    REG_OUTPUT("keyCode", m_keyCodeOutput);
}


bool OkeanKeyboard::processKeyCode(int keyCode)
{
    if (m_strobe)
        return true;

    if (keyCode >= 0x80)
        return true;

    m_code = keyCode;
    m_strobe = true;

    m_keyCodeOutput->setValue(m_code);
    m_strobeOutput->setValue(m_strobe);

    m_keyCodeOutput->setValue(m_code);
    m_strobeOutput->setValue(m_strobe);

    return true;
}


void OkeanKeyboard::processKey(EmuKey key, bool isPressed)
{
    if (isPressed) {
        if (m_strobe)
            return;
        m_strobe = true;
        switch (key) {
        case EK_LEFT:
            m_code = 0x08;
            break;
        case EK_RIGHT:
            m_code = 0x18;
            break;
        case EK_UP:
            m_code = 0x19;
            break;
        case EK_DOWN:
            m_code = 0x1A;
            break;
        case EK_F1:
            m_code = 0x01;
            break;
        case EK_F2:
            m_code = 0x02;
            break;
        case EK_F3:
            m_code = 0x04;
            break;
        case EK_F4:
            m_code = 0x05;
            break;
        case EK_F5:
            m_code = 0x06;
            break;
        case EK_HELP: // F6
            m_code = 0x07;
            break;
        case EK_SET: // F7
            m_code = 0x0A;
            break;
        case EK_GRAPH: // F8
            m_code = 0x0B   ;
            break;
        case EK_RESET: // F12
            m_code = 0x03   ;
            break;

        case EK_ESC:
            m_code = 0x1B;
            break;
        case EK_CR:
            m_code = 0x0D;
            break;
        case EK_BSP:
            m_code = 0x08;
            break;

        default:
            m_strobe = false;
        }
    }

    if (m_strobe) {
        m_keyCodeOutput->setValue(m_code);
        m_strobeOutput->setValue(true);
    }
}


void OkeanKeyboard::ack(bool val)
{
    if (val) {
        m_strobe = false;
        m_strobeOutput->setValue(m_strobe);
    }
}


void OkeanMatrixKeyboard::initConnections()
{
    Keyboard::initConnections();

    REG_INPUT("ack", OkeanMatrixKeyboard::ack);

    REG_OUTPUT("strobe", m_strobeOutput);
    REG_OUTPUT("rowData", m_rowDataOutput);
    REG_OUTPUT("column", m_columnOutput);
    REG_OUTPUT("shift", m_shiftOutput);
    REG_OUTPUT("ctrl", m_ctrlOutput);
}


void OkeanMatrixKeyboard::resetKeys()
{
    m_shift = false;
    m_ctrl = false;
    m_column = 0;
    m_rowData = 0;
    m_strobe = false;

    m_shiftOutput->setValue(m_shift);
    m_ctrlOutput->setValue(m_shift);
    m_columnOutput->setValue(m_column);
    m_rowDataOutput->setValue(m_rowData);
    m_strobeOutput->setValue(m_strobe);
}


void OkeanMatrixKeyboard::processKey(EmuKey key, bool isPressed)
{
    if (key == EK_SHIFT) {
        m_shift = isPressed;
        m_shiftOutput->setValue(m_shift);
        return;
    }

    if (key == EK_CTRL) {
        m_ctrl = isPressed;
        m_ctrlOutput->setValue(m_ctrl);
        return;
    }

    if (!isPressed || key == EK_NONE || m_strobe)
        return;

    // some keys are swaped on Okean keyboard compared to MS7007
    if (key == EK_SLASH)
        key = EK_COLON;
    else if (key == EK_COLON)
        key = EK_UNDSCR;
    else if (key == EK_UNDSCR)
        key = EK_SLASH;
    else if (key == EK_SEMICOLON)
        key = EK_CARET;
    else if (key == EK_CARET)
        key = EK_SEMICOLON;

    int i, j;

    for (i = 0; i < 8; i++)
        for (j = 0; j < 11; j++)
            if (key == m_keyMatrix[i][j])
                goto found;
    return;

    found:

    m_strobe = true;
    m_rowData = 1 << i;
    m_column = j;

    m_columnOutput->setValue(m_column);
    m_rowDataOutput->setValue(m_rowData);
    m_strobeOutput->setValue(m_strobe);
}


void OkeanMatrixKeyboard::ack(bool val)
{
    if (val) {
        m_strobe = false;
        m_strobeOutput->setValue(m_strobe);
    }
}


bool OkeanFileLoader::loadFile(const std::string &fileName, bool run)
{
    auto periodPos = fileName.find_last_of(".");
    string ext = periodPos != string::npos ? fileName.substr(periodPos) : fileName;
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    int fileSize;
    uint8_t* buf = palReadFile(fileName, fileSize, false);
    if (!buf)
        return false;

    if (ext == ".hex") {
        Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(m_platform->getCpu());

        if (run) {
            m_platform->reset();
            cpu->disableHooks();
            g_emulation->exec((int64_t)cpu->getKDiv() * m_skipTicks, true);
        }

        uint16_t minAddr;
        bool loaded = loadHex(buf, fileSize, minAddr);

        if (loaded && run) {
            cpu->enableHooks();
            cpu->setPC(minAddr);
        }
    }


    if (fileSize >= 65536 - 256) {
        delete[] buf;
        return false;
    }

    Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(m_platform->getCpu());

    if (run) {
        m_platform->reset();
        cpu->disableHooks();
        g_emulation->exec((int64_t)cpu->getKDiv() * m_skipTicks, true);
    }

    uint8_t* ptr = buf;
    for (int i = 0; i < fileSize; i++)
        m_as->writeByte(0x100 + i, *ptr++);

    if (run) {
        cpu->enableHooks();
        cpu->setPC(0x100);
    }

    return true;
}


bool OkeanFddRegisters::setProperty(const std::string &propertyName, const EmuValuesList &values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "fdc") {
        m_fdc = static_cast<Fdc1793*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


void OkeanFddRegisters::writeByte(int addr, uint8_t value)
{
    if (addr == 0)
        return;

    m_drive = ((value & 4) >> 2) ^ 1;
    m_head = (value & 0x20) >> 5;

    m_fdc->setDrive(m_drive);
    m_fdc->setHead(m_head);
}


uint8_t OkeanFddRegisters::readByte(int addr)
{
    if (addr)
        // port 25h
        return ((m_fdc->getIrq() ? 0 : 0x01) | (m_drive ? 0x04 : 0x0a) | (m_head ? 0x40 : 0) | 0x20);

    // port 24h
    int n = 8;
    bool drq = false;
    bool intr = false;
    while (!drq && !intr && n--) {
        drq = m_fdc->getDrq();
        intr = m_fdc->getIrq();
        m_fdc->readByte(0);
    }
    return drq;
}
