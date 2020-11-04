/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2020
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

#include "Orion.h"
#include "Emulation.h"
#include "Platform.h"
#include "Globals.h"
#include "EmuWindow.h"
#include "SoundMixer.h"
#include "Memory.h"
#include "AddrSpace.h"
#include "Fdc1793.h"
#include "Cpu.h"

using namespace std;


void OrionCore::draw()
{
    m_crtRenderer->renderFrame();
    m_window->drawFrame(m_crtRenderer->getPixelData());
    m_window->endDraw();
}


void OrionCore::inte(bool isActive)
{
    m_beepSoundSource->setValue(isActive ? 1 : 0);
}


void OrionCore::attachCrtRenderer(OrionRenderer* crtRenderer)
{
    m_crtRenderer = crtRenderer;
}


bool OrionCore::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (PlatformCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<OrionRenderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "beepSoundSource") {
        m_beepSoundSource = static_cast<GeneralSoundSource*>(g_emulation->findObject(values[0].asString()));
        return true;
    }
    return false;
}


OrionRenderer::OrionRenderer()
{
    m_sizeX = m_prevSizeX = 384;
    m_sizeY = m_prevSizeY = 256;
    m_aspectRatio = m_prevAspectRatio = 576.0 * 9 / 704 / 10;
    m_bufSize = m_prevBufSize = m_sizeX * m_sizeY;
    int maxBufSize = 521 * 288;
    m_pixelData = new uint32_t[maxBufSize];
    m_prevPixelData = new uint32_t[maxBufSize];
    memset(m_pixelData, 0, m_bufSize * sizeof(uint32_t));
    memset(m_prevPixelData, 0, m_prevBufSize * sizeof(uint32_t));
}


void OrionRenderer::attachScreenMemory(Ram* screenMemory)
{
    m_screenMemory = screenMemory->getDataPtr();
}


void OrionRenderer::attachColorMemory(Ram* colorMemory)
{
    m_colorMemory = colorMemory->getDataPtr();
}


void OrionRenderer::setScreenBase(uint16_t base)
{
    m_screenBase = base;
}


void OrionRenderer::setColorModeByte(uint8_t modeByte)
{
    m_colorMode = (OrionColorMode)(modeByte >> 1);
    m_palette = modeByte & 1;
}

void OrionRenderer::renderFrame()
{
    swapBuffers();

    int offsetX = 0;
    int offsetY = 0;

    if (m_showBorder) {
        m_sizeX = 521;
        m_sizeY = 288;
        memset(m_pixelData, 0, m_sizeX * m_sizeY * sizeof(uint32_t));
        offsetX = 76;
        offsetY = 5;
        m_aspectRatio = double(m_sizeY) * 4 / 3 / m_sizeX;
    } else {
        m_sizeX = 384;
        m_sizeY = 256;
        offsetX = offsetY = 0;
        m_aspectRatio = 576.0 * 9 / 704 / 10;
    }

    int offset = offsetY * m_sizeX + offsetX;

    for (int row = 0; row < 256; row++)
        for (int col = 0; col < 48; col++) {
            int addr = m_screenBase + col * 256 + row;
            if (m_colorMode != OCM_4COLOR) {
                uint8_t bt = m_screenMemory[addr];
                int fgColor, bgColor;
                if (m_colorMode == OCM_MONO) {
                    if (!m_palette) {
                        fgColor = 2;
                        bgColor = 0;
                    } else {
                        fgColor = 6;
                        bgColor = 3;
                    }
                } else if (m_colorMode == OCM_BLANK) {
                    fgColor = bgColor = 0;
                } else { //if (m_colorMode == OCM_16COLOR) {
                    fgColor = m_colorMemory[addr] & 0xF;
                    bgColor = (m_colorMemory[addr] & 0xF0) >> 4;
                }
                if (!m_isColorMode) {
                    fgColor = fgColor & 0x2 ? 7 : 0;
                    bgColor = bgColor & 0x2 ? 7 : 0;
                }
                for (int pt = 0; pt < 8; pt++, bt<<=1)
                    m_pixelData[offset + row * m_sizeX + col * 8 + pt] = (bt & 0x80) ? orion16ColorPalette[fgColor] : orion16ColorPalette[bgColor];
                } else {
                // 4 color mode
                    uint8_t bt1 = m_screenMemory[addr];
                    uint8_t bt2 = m_colorMemory[addr];
                    for (int pt = 0; pt < 8; pt++, bt1<<=1, bt2<<=1) {
                        int color = ((bt1 & 0x80) >> 6) | ((bt2 & 0x80) >> 7);
                        if (!m_isColorMode)
                            color = color & 0x2 ? 4 : 0;
                        m_pixelData[offset + row * m_sizeX + col * 8 + pt] = orion4ColorPalettes[m_palette][color];
                    }
                }
        }
}


void OrionRenderer::toggleCropping()
{
    m_showBorder = !m_showBorder;
}


bool OrionRenderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "screenMemory") {
        attachScreenMemory(static_cast<Ram*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "colorMemory") {
        attachColorMemory(static_cast<Ram*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "colorMode") {
        if (values[0].asString() == "mono")
            m_isColorMode = false;
        else if (values[0].asString() == "color")
            m_isColorMode = true;
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


string OrionRenderer::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "colorMode")
        return m_isColorMode ? "color" : "mono";
    else if (propertyName == "visibleArea")
        return m_showBorder ? "yes" : "no";
    else if (propertyName == "crtMode")
        switch (m_colorMode) {
        case OCM_MONO:
            return u8"2-color 384\u00D7256@50.08Hz";
        case OCM_4COLOR:
            return u8"4-color 384\u00D7256@50.08Hz";
        case OCM_16COLOR:
            return u8"16-color 384\u00D7256@50.08Hz";
        default:
            return "384\u00D7256@50.08Hz";
        }

    return "";
}


void OrionMemPageSelector::writeByte(int, uint8_t value)
{
    m_addrSpaceMapper->setCurPage(value & 0x3);
};


bool OrionMemPageSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "mapper") {
        attachAddrSpaceMapper(static_cast<AddrSpaceMapper*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


void OrionScreenSelector::writeByte(int, uint8_t value)
{
    if (m_renderer)
        m_renderer->setScreenBase((~value & 0x3) << 14);
};


bool OrionScreenSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<OrionRenderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


void OrionColorModeSelector::writeByte(int, uint8_t value)
{
    if (m_renderer)
        m_renderer->setColorModeByte(value & 0x7);
};


bool OrionColorModeSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<OrionRenderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


void OrionFddControlRegister::writeByte(int, uint8_t value)
{
    m_fdc->setDrive(value & 1);
    if (m_type == OFT_STANDARD)
        m_fdc->setHead(((value & 0x10) >> 4) ^ 1);
    else // m_type == OFT_SPDOS
        m_fdc->setHead((value & 0x2) >> 1);
}


bool OrionFddControlRegister::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "type") {
        if (values[0].asString() == "standart") {
            m_type = OFT_STANDARD;
            return true;
        } else if (values[0].asString() == "spdos") {
            m_type = OFT_SPDOS;
            return true;
        }
        return true;
    } else if (propertyName == "fdc") {
        attachFdc1793(static_cast<Fdc1793*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


uint8_t OrionFddQueryRegister::readByte(int)
{
    return (m_fdc->getDrq() ? 0 : 1) | (m_fdc->getIrq() ? 0 : 0x80);
}

bool OrionFddQueryRegister::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "fdc") {
        attachFdc1793(static_cast<Fdc1793*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


bool OrionFileLoader::loadFile(const std::string& fileName, bool run)
{
    int fileSize;
    uint8_t* buf = palReadFile(fileName, fileSize, false);
    if (!buf)
        return false;

    if (fileSize < 32) {
        delete[] buf;
        return false;
    }

    uint8_t* ptr = buf;

    int bruOffset = 0;

    int len = (ptr[0x0b] << 8) | ptr[0x0a];;

    if (len == 0) {
        // RKO
        bruOffset = 0x4d;

        if (fileSize < bruOffset + 32) {
            delete[] buf;
            return false;
        }

        fileSize -= bruOffset;
        ptr += bruOffset;
        len = (ptr[0x0b] << 8) | ptr[0x0a];
    }

    len = (((len - 1) | 0xf ) + 17);

    if (fileSize < len) {
        delete[] buf;
        return false;
    }

    if (run) {
        if (len < 16) {
            delete[] buf;
            return false;
        }
        uint16_t begAddr = (ptr[0x09] << 8) | ptr[0x08];
        uint16_t nBytes = (ptr[0x0b] << 8) | ptr[0x0a];
        len -= 16;
        ptr += 16;
        if (len < nBytes) {
            delete[] buf;
            return false;
        }


        m_platform->reset();
        Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(m_platform->getCpu());
        if (cpu) {
            g_emulation->exec((uint64_t)cpu->getKDiv() * 3000000);
            cpu->setPC(begAddr);
        }

        for (int i = 0; i < nBytes; i++)
            m_as->writeByte(begAddr++, *ptr++);
    } else {
        for (uint16_t addr = 0; addr < len; addr++)
            m_ramDisk->writeByte(addr, *ptr++);
        m_ramDisk->writeByte(len, 0xff);
    }

    delete[] buf;

    return true;
}


bool OrionFileLoader::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (FileLoader::setProperty(propertyName, values))
        return true;

    if (propertyName == "ramDiskAddrSpace") {
        m_ramDisk = static_cast<AddressableDevice*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}
