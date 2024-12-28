/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2018-2024
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

#include "Globals.h"
#include "Eureka.h"
#include "Emulation.h"
#include "SoundMixer.h"

using namespace std;


void EurekaCore::inte(bool isActive)
{
    m_inteSoundSource->setValue(isActive ? 1 : 0);
}


bool EurekaCore::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (SpecCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "beepSoundSource") {
        m_inteSoundSource = static_cast<GeneralSoundSource*>(g_emulation->findObject(values[0].asString()));
        return true;
    }
    return false;
}


EurekaPpi8255Circuit::EurekaPpi8255Circuit(std::string romDiskName)
{
    m_romDisk = new SpecRomDisk(romDiskName);
}


EurekaPpi8255Circuit::~EurekaPpi8255Circuit()
{
    delete m_romDisk;
}


void EurekaPpi8255Circuit::setPortA(uint8_t value)
{
    SpecPpi8255Circuit::setPortA(value);
    m_romDisk->setPortA(value);
}


void EurekaPpi8255Circuit::setPortC(uint8_t value)
{
    SpecPpi8255Circuit::setPortC(value);
    m_romDisk->setPortC(value & 0x7f);
    m_useRomDisk = value & 0x80;
    m_renderer->setColorMode(value & 0x40);
}


uint8_t EurekaPpi8255Circuit::getPortB()
{
    uint8_t res = SpecPpi8255Circuit::getPortB();
    if (m_useRomDisk)
        res &= m_romDisk->getPortB();
    return res;
}


bool EurekaPpi8255Circuit::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (SpecPpi8255Circuit::setProperty(propertyName, values))
        return true;

    if (propertyName == "videoRam")
        return false;
    if (SpecPpi8255Circuit::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachEurekaRenderer(static_cast<EurekaRenderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


EurekaRenderer::EurekaRenderer()
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


void EurekaRenderer::renderFrame()
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

    int offset = offsetY * m_sizeX + offsetX;

    if (m_colorMode) {
        // color mode
        for (int row = 0; row < 256; row++)
            for (int col = 0; col < 48; col++) {
                int addr = col * 256 + row;
                uint8_t bt = m_videoRam[addr];
                for (int pt = 0; pt < 4; pt++, bt <<= 2) {
                    uint32_t color = eurekaPalette[(bt & 0xC0) >> 6];
                    m_pixelData[offset + row * m_sizeX + col * 8 + pt * 2] = color;
                    m_pixelData[offset + row * m_sizeX + col * 8 + pt * 2 + 1] = color;
                }
            }
    } else {
        // b&w mode
        for (int row = 0; row < 256; row++)
            for (int col = 0; col < 48; col++) {
                int addr = col * 256 + row;
                uint8_t bt = m_videoRam[addr];
                for (int pt = 0; pt < 8; pt++, bt <<= 1)
                    m_pixelData[offset + row * m_sizeX + col * 8 + pt] = (bt & 0x80) ? 0xC0C0C0 : 0x000000;
            }
    }
}


void EurekaRenderer::operate()
{
    renderFrame();
    m_curClock += g_emulation->getFrequency() * 512 * 312 / 8000000; // 8 MHz pixelclock, 312 scanlines, 512 pixels wide
    g_emulation->screenUpdateReq(); // transfer to Core
}


void EurekaRenderer::toggleCropping()
{
    m_showBorder = !m_showBorder;
}


bool EurekaRenderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (CrtRenderer::setProperty(propertyName, values))
        return true;

    if (propertyName == "videoRam") {
        attachVideoRam(static_cast<Ram*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "visibleArea") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_showBorder = values[0].asString() == "yes";
            return true;
        }
    }
    return false;
}


string EurekaRenderer::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "visibleArea") {
        return m_showBorder ? "yes" : "no";
    } else if (propertyName == "crtMode") {
            return m_colorMode ? u8"Color 384\u00D7256@50.08Hz" : u8"Mono 384\u00D7256@50.08Hz";
    }

    return "";
}
