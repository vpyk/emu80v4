/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2018
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

#include "Eureka.h"
#include "Emulation.h"
#include "SoundMixer.h"

using namespace std;


EurekaCore::EurekaCore()
{
    m_inteSoundSource = new GeneralSoundSource;
}



EurekaCore::~EurekaCore()
{
    delete m_inteSoundSource;
}


void EurekaCore::inte(bool isActive)
{
    m_inteSoundSource->setValue(isActive ? 1 : 0);
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
    m_useRomDisk = value && 0x80;
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
    m_aspectRatio = m_prevAspectRatio = 12. / 13.;
    m_bufSize = m_prevBufSize = m_sizeX * m_sizeY;
    m_pixelData = new uint32_t[m_bufSize];
    m_prevPixelData = new uint32_t[m_prevBufSize];
    memset(m_pixelData, 0, m_bufSize * sizeof(uint32_t));
    memset(m_prevPixelData, 0, m_prevBufSize * sizeof(uint32_t));
}


void EurekaRenderer::renderFrame()
{
    swapBuffers();

    if (m_colorMode) {
        // color mode
        for (int row = 0; row < 256; row++)
            for (int col = 0; col < 48; col++) {
                int addr = col * 256 + row;
                uint8_t bt = m_videoRam[addr];
                for (int pt = 0; pt < 4; pt++, bt <<= 2) {
                    uint32_t color = eurekaPalette[(bt & 0xC0) >> 6];
                    m_pixelData[row * 384 + col * 8 + pt * 2] = color;
                    m_pixelData[row * 384 + col * 8 + pt * 2 + 1] = color;
                }
            }
    } else {
        // b&w mode
        for (int row = 0; row < 256; row++)
            for (int col = 0; col < 48; col++) {
                int addr = col * 256 + row;
                uint8_t bt = m_videoRam[addr];
                for (int pt = 0; pt < 8; pt++, bt <<= 1)
                    m_pixelData[row * 384 + col * 8 + pt] = (bt & 0x80) ? 0xC0C0C0 : 0x000000;
            }
    }
}


bool EurekaRenderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "videoRam") {
        attachVideoRam(static_cast<Ram*>(g_emulation->findObject(values[0].asString())));
        return true;
    }
    return false;
}
