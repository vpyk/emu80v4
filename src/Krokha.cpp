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

#include "Krokha.h"
#include "Emulation.h"
#include "Globals.h"
#include "EmuWindow.h"
#include "Platform.h"
#include "Cpu.h"
#include "Memory.h"

using namespace std;


void KrokhaCore::draw()
{
    m_window->drawFrame(m_crtRenderer->getPixelData());
    m_window->endDraw();
}


void KrokhaCore::vrtc(bool isActive)
{
    // actually this is not VTRC, but this proc is called onece per frame on interrupt
    if (isActive) {
        Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_platform->getCpu());
        m_intReq = true;
        if (cpu->getInte()) {
            m_intReq = false;
            cpu->intRst(7);
        }
    }
}


void KrokhaCore::inte(bool isActive)
{
    Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_platform->getCpu());
    if (isActive && m_intReq && cpu->getInte()) {
        m_intReq = false;
        cpu->intRst(7);
    }
}


bool KrokhaCore::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (PlatformCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        m_crtRenderer = static_cast<CrtRenderer*>(g_emulation->findObject(values[0].asString()));
        return true;
    }
    return false;
}


KrokhaRenderer::KrokhaRenderer()
{
    m_frameBuf = new uint32_t[384 * 256];
    memset(m_frameBuf, 0, 384 * 256 * sizeof(uint32_t));

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


KrokhaRenderer::~KrokhaRenderer()
{
    delete[] m_frameBuf;
}


void KrokhaRenderer::toggleColorMode()
{
    m_colorMode = ! m_colorMode;
}


void KrokhaRenderer::renderLine(int line)
{
    for (int col = 0; col < 48; col++) {
        uint8_t chr = m_screenMemory[col * 32 + line / 8];
        uint8_t bt = m_font[8 * chr + line % 8];
        uint32_t fgColor = m_colorMode ? c_krokhaColorPalette[chr >> 5] : 0xC0C0C0;

        for (int pt = 0; pt < 8; pt++, bt <<= 1)
            m_frameBuf[line * 384 + col * 8 + pt] = (bt & 0x80) ? fgColor : 0;
    }
}


void KrokhaRenderer::renderFrame()
{
    swapBuffers();

    if (m_showBorder) {
        m_sizeX = 417;
        m_sizeY = 288;
        m_offsetX = 21;
        m_offsetY = 10;
        m_aspectRatio = double(m_sizeY) * 4 / 3 / m_sizeX;

        memset(m_pixelData, 0, m_sizeX * m_sizeY * sizeof(uint32_t));
        for (int i = 0; i < 256; i++)
            memcpy(m_pixelData + m_sizeX * (i + m_offsetY) + m_offsetX, m_frameBuf + i * 384, 384 * sizeof(uint32_t));
    } else {
        m_sizeX = 384;
        m_sizeY = 256;
        m_offsetX = m_offsetY = 0;
        m_aspectRatio = 576.0 * 9 / 704 / 8;

        memcpy(m_pixelData, m_frameBuf, m_sizeX * m_sizeY * sizeof(uint32_t));
    }
}


void KrokhaRenderer::operate()
{
    if (m_curLine < 256)
        renderLine(m_curLine);
    else if (m_curLine == 256) {
        m_platform->getCore()->vrtc(true);
    } else if (m_curLine == 281) {
        renderFrame();
        g_emulation->screenUpdateReq();
    }
    if (++m_curLine == 312)
        m_curLine = 0;

    m_curClock += g_emulation->getFrequency() * 512 / 8000000;

}


void KrokhaRenderer::toggleCropping()
{
    m_showBorder = !m_showBorder;
}


bool KrokhaRenderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (CrtRenderer::setProperty(propertyName, values))
        return true;

    if (propertyName == "screenMemory") {
        m_screenMemory = static_cast<Ram*>(g_emulation->findObject(values[0].asString()))->getDataPtr();
        return true;
    } else if (propertyName == "font") {
        m_font = palReadFile(values[0].asString(), m_fontSize);
        return true;
    } else if (propertyName == "colorMode") {
        if (values[0].asString() == "mono")
            m_colorMode = false;
        else if (values[0].asString() == "color")
            m_colorMode = true;
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


string KrokhaRenderer::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "colorMode") {
        return m_colorMode ? "color" : "mono";
    } else if (propertyName == "visibleArea") {
        return m_showBorder ? "yes" : "no";
    } else if (propertyName == "crtMode") {
            return u8"384\u00D7256@50.08Hz" ;
    }

    return "";
}


KrokhaJoystick::KrokhaJoystick()
{
    KrokhaJoystick::resetKeys();
}


void KrokhaJoystick::initConnections()
{
    m_keysOutput = registerOutput("keys");
}


void KrokhaJoystick::resetKeys()
{
    m_keys = 0;
}


void KrokhaJoystick::processKey(EmuKey key, bool isPressed)
{
    int bit;

    switch (key) {
    case EK_SPACE:
    case EK_CR:
        bit = 1;
        break;
    case EK_UP:
        bit = 2;
        break;
    case EK_DOWN:
        bit = 4;
        break;
    case EK_RIGHT:
        bit = 8;
        break;
    case EK_LEFT:
        bit = 16;
        break;
    default:
        return;
    }

    if (isPressed)
        m_keys |= bit;
    else
        m_keys &= ~bit;

    m_keysOutput->setValue(~m_keys);
}
