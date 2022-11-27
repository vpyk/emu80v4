/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2022
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

#include "Ut88.h"
#include "Emulation.h"
#include "EmuWindow.h"
#include "Memory.h"
#include "Cpu8080.h"

using namespace std;

void Ut88Core::draw()
{
    m_crtRenderer->renderFrame();
    m_window->drawFrame(m_crtRenderer->getPixelData());
    m_window->endDraw();
}


void Ut88Core::attachCrtRenderer(Ut88Renderer* crtRenderer)
{
    m_crtRenderer = crtRenderer;
}


bool Ut88Core::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (PlatformCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<Ut88Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }
    return false;
}


Ut88Renderer::Ut88Renderer()
{
    m_sizeX = m_prevSizeX = 384;
    m_sizeY = m_prevSizeY = 320;
    m_aspectRatio = m_prevAspectRatio = 12. / 13.;
    m_bufSize = m_prevBufSize = m_sizeX * m_sizeY;
    m_pixelData = new uint32_t[512 * 512]; // altRenderer requires more mem
    m_prevPixelData = new uint32_t[512 * 512];
    memset(m_pixelData, 0, m_bufSize * sizeof(uint32_t));
    memset(m_prevPixelData, 0, m_prevBufSize * sizeof(uint32_t));
}


void Ut88Renderer::attachScreenMemory(Ram* screenMemory)
{
    m_screenMemory = screenMemory->getDataPtr();
}


void Ut88Renderer::primaryRenderFrame()
{
    m_sizeX = 384;
    m_sizeY = 320;
    m_bufSize = m_sizeX * m_sizeY;

    for (int row = 0; row < 32; row++)
        for (int col = 0; col < 64; col++) {
            int addr = row * 64 + col;
            bool rvv = col != 63 && m_screenMemory[addr + 1] & 0x80;
            uint8_t* fontPtr = m_font + (m_screenMemory[addr] & 0x7f) * 8;
            for (int l = 0; l < 8; l++) {
                uint8_t bt = fontPtr[l] << 2;
                for (int pt = 0; pt < 6; pt++) {
                    bool pixel = (bt & 0x80);
                    if (rvv)
                        pixel = !pixel;
                    bt <<= 1;
                    m_pixelData[row * 384 * 10 + l * 384 + col * 6 + pt] = pixel ? 0 : 0xC0C0C0;
                }
            }
            for (int l = 8; l < 10; l++)
                for (int pt = 0; pt < 6; pt++)
                    m_pixelData[row * 384 * 10 + l * 384 + col * 6 + pt] = rvv ? 0xC0C0C0 : 0;
        }
}


void Ut88Renderer::altRenderFrame()
{
    m_sizeX = 512;
    m_sizeY = 512;
    m_bufSize = m_sizeX * m_sizeY;

    for (int row = 0; row < 32; row++)
        for (int col = 0; col < 64; col++) {
            int addr = row * 64 + col;
            bool rvv = col != 63 && m_screenMemory[addr + 1] & 0x80;
            uint8_t* fontPtr = m_altFont + (m_screenMemory[addr] & 0x7f) * 16;
            for (int l = 0; l < 16; l++) {
                uint8_t bt = fontPtr[l];
                for (int pt = 0; pt < 8; pt++) {
                    bool pixel = (bt & 0x80);
                    if (!rvv)
                        pixel = !pixel;
                    bt <<= 1;
                    m_pixelData[row * 512 * 16 + l * 512 + col * 8 + pt] = pixel ? 0 : 0xC0C0C0;
                }
            }
        }
}


const char* Ut88Renderer::getTextScreen()
{
    char16_t* wTextArray = new char16_t[32 * 64];

    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            uint8_t chr = m_screenMemory[y * 64 + x] & 0x7f;
            char16_t wchr = c_mikroSymbols[chr];
            wTextArray[y * 64 + x] = wchr;
        }
    }

    return generateTextScreen(wTextArray, 64, 32);
}


bool Ut88Renderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (TextCrtRenderer::setProperty(propertyName, values))
        return true;

    if (propertyName == "screenMemory") {
        attachScreenMemory(static_cast<Ram*>(g_emulation->findObject(values[0].asString())));
        return true;
    }
    return false;
}


void Ut88AddrSpaceMapper::writeByte(int addr, uint8_t value)
{
    if (m_cpu->checkForStackOperation())
        AddrSpaceMapper::writeByte(addr, value);
    else
        m_pages[0]->writeByte(addr, value);
}


uint8_t Ut88AddrSpaceMapper::readByte(int addr)
{
    if (m_cpu->getStatusWord() & 0x04)
        return AddrSpaceMapper::readByte(addr);
    else
        return m_pages[0]->readByte(addr);
}


bool Ut88AddrSpaceMapper::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddrSpaceMapper::setProperty(propertyName, values))
        return true;

    if (propertyName == "cpu") {
        m_cpu = static_cast<Cpu8080*>(g_emulation->findObject(values[0].asString()));
        return true;
    }
    return false;
}


void Ut88MemPageSelector::writeByte(int, uint8_t value)
{
    if ((value & 0x0f) == 0x0f)
        m_addrSpaceMapper->setCurPage(0);
    else if (!(value & 0x01))
        m_addrSpaceMapper->setCurPage(1);
    else if (!(value & 0x02))
        m_addrSpaceMapper->setCurPage(2);
    else if (!(value & 0x04))
        m_addrSpaceMapper->setCurPage(4);
    else if (!(value & 0x08))
        m_addrSpaceMapper->setCurPage(8);
};


bool Ut88MemPageSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "mapper") {
        attachAddrSpaceMapper(static_cast<AddrSpaceMapper*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}
