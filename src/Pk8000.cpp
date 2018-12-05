/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2018
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
//#include "SoundMixer.h"
#include "Memory.h"
#include "AddrSpace.h"
#include "Cpu.h"

using namespace std;


Pk8000Core::Pk8000Core()
{
    // ...
}



Pk8000Core::~Pk8000Core()
{
    // ...
}


void Pk8000Core::draw()
{
    m_crtRenderer->renderFrame();
    m_window->drawFrame(m_crtRenderer->getPixelData());
    m_window->endDraw();
}


void Pk8000Core::inte(bool isActive)
{
    //m_beepSoundSource->setValue(isActive ? 1 : 0);
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
    for (int i = 0; i < 4; i++)
        m_screenMemoryBanks[i] = nullptr;

    m_sizeX = m_prevSizeX = 256;
    m_sizeY = m_prevSizeY = 192;
    //m_aspectRatio = m_prevAspectRatio = 576.0 * 9 / 704 / 10;
    m_aspectRatio = m_prevAspectRatio = 1;
    m_bufSize = m_prevBufSize = m_sizeX * m_sizeY;
    m_pixelData = new uint32_t[m_bufSize];
    m_prevPixelData = new uint32_t[m_prevBufSize];
    memset(m_pixelData, 0, m_bufSize * sizeof(uint32_t));
    memset(m_prevPixelData, 0, m_prevBufSize * sizeof(uint32_t));
}


void Pk8000Renderer::attachScreenMemoryBank(int bank, Ram* screenMemoryBank)
{
    if (bank >= 0 && bank < 4)
        m_screenMemoryBanks[bank] = screenMemoryBank->getDataPtr();
}


void Pk8000Renderer::setScreenBank(unsigned bank)
{
    if (bank < 4)
        m_bank = bank;
}


void Pk8000Renderer::setMode(unsigned mode)
{
    if (mode < 3)
        m_mode = mode;
}


void Pk8000Renderer::renderFrame()
{
    swapBuffers();

    switch (m_mode) {
    case 0:
        m_sizeX = 240;
        for (int row = 0; row < 24; row++)
            for (int pos = 0; pos < 40; pos++) {
                uint8_t chr = m_screenMemoryBanks[m_bank][(m_txtBase & ~0x0400) + row * 64 + pos];
                for (int line = 0; line < 8; line++) {
                    uint8_t bt = m_screenMemoryBanks[m_bank][m_sgBase + chr * 8 + line];
                    for (int i = 0; i < 6; i++) {
                        uint32_t color = bt & 0x80 ? m_fgColor : m_bgColor;
                        m_pixelData[40 * 6 * 8 * row + 40 * 6 * line + pos * 6 + i] = color;
                        bt <<= 1;
                    }
                }
            }
        break;
    case 1:
        m_sizeX = 256;
        for (int row = 0; row < 24; row++)
            for (int pos = 0; pos < 32; pos++) {
                uint8_t chr = m_screenMemoryBanks[m_bank][m_txtBase + row * 32 + pos];
                unsigned colorCode = m_screenMemoryBanks[m_bank][0x400 + (chr >> 3)];
                uint32_t fgColor = c_pk8000ColorPalette[colorCode & 0x0F];
                uint32_t bgColor = c_pk8000ColorPalette[colorCode >> 4];
                for (int line = 0; line < 8; line++) {
                    uint8_t bt = m_screenMemoryBanks[m_bank][m_sgBase + chr * 8 + line];
                    for (int i = 0; i < 8; i++) {
                        uint32_t color = bt & 0x80 ? fgColor : bgColor;
                        m_pixelData[32 * 8 * 8 * row + 32 * 8 * line + pos * 8 + i] = color;
                        bt <<= 1;
                    }
                }
            }
        break;
    case 2:
        m_sizeX = 256;
        for (int part = 0; part < 3; part++)
            for (int row = 0; row < 8; row++)
                for (int pos = 0; pos < 32; pos++) {
                    uint8_t chr = m_screenMemoryBanks[m_bank][m_sgBase + part * 256 + row * 32 + pos];
                    for (int line = 0; line < 8; line++) {
                        unsigned colorCode = m_screenMemoryBanks[m_bank][m_colBase + part * 0x800 + chr * 8 + line];
                        uint32_t fgColor = c_pk8000ColorPalette[colorCode & 0x0F];
                        uint32_t bgColor = c_pk8000ColorPalette[colorCode >> 4];
                        uint8_t bt = m_screenMemoryBanks[m_bank][m_grBase + part * 0x800 + chr * 8 + line];
                        for (int i = 0; i < 8; i++) {
                            uint32_t color = bt & 0x80 ? fgColor : bgColor;
                            m_pixelData[8 * 32 * 8 * 8 * part + 32 * 8 * 8 * row + 32 * 8 * line + pos * 8 + i] = color;
                            bt <<= 1;
                        }
                    }
                }

        break;
    default:
        break;
    }
}


bool Pk8000Renderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "screenMemoryBank") {
        attachScreenMemoryBank(values[0].asInt(), static_cast<Ram*>(g_emulation->findObject(values[1].asString())));
        return true;
    }

    return false;
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


void Pk8000Ppi8255Circuit1::setPortA(uint8_t value)
{
    m_addrSpaceMappers[0]->setCurPage(value & 0x03);
    m_addrSpaceMappers[1]->setCurPage((value & 0x0C) >> 2);
    m_addrSpaceMappers[2]->setCurPage((value & 0x30) >> 4);
    m_addrSpaceMappers[3]->setCurPage((value & 0xC0) >> 6);
};


uint8_t Pk8000Ppi8255Circuit1::getPortB()
{
    if (m_kbd)
        return m_kbd->getMatrixRowState();
    else
        return 0xFF;

}


void Pk8000Ppi8255Circuit1::setPortC(uint8_t value)
{
    if (m_kbd)
        m_kbd->setMatrixRowNo(value & 0x0F);
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
    }

    return false;
}


void Pk8000Ppi8255Circuit2::setPortA(uint8_t value)
{
    if (m_renderer) {
        if (value & 0x10)
            m_renderer->setMode(2);
        else
            m_renderer->setMode(value & 0x20 ? 0 : 1);
        m_renderer->setScreenBank((value & 0xc0) >> 6);
    }
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


void Pk8000ColorSelector::writeByte(int, uint8_t value)
{
    m_value = value;
    if (m_renderer) {
        m_renderer->setFgColor(value & 0x0F);
        m_renderer->setBgColor(value >> 4);
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


void Pk8000TxtBufSelector::writeByte(int, uint8_t value)
{
    m_value = value;
    if (m_renderer) {
        m_renderer->setTextBufferBase((value & 0x0F) << 10);
    }
}


bool Pk8000TxtBufSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<Pk8000Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


void Pk8000SymGenBufSelector::writeByte(int, uint8_t value)
{
    m_value = value;
    if (m_renderer) {
        m_renderer->setSymGenBufferBase((value & 0x0E) << 10);
    }
}


bool Pk8000SymGenBufSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<Pk8000Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


void Pk8000GrBufSelector::writeByte(int, uint8_t value)
{
    m_value = value;
    if (m_renderer) {
        m_renderer->setGraphicsBufferBase((value & 0x08) << 10);
    }
}


bool Pk8000GrBufSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<Pk8000Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


void Pk8000ColBufSelector::writeByte(int, uint8_t value)
{
    m_value = value;
    if (m_renderer) {
        m_renderer->setColorBufferBase((value & 0x08) << 10);
    }
}


bool Pk8000ColBufSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
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

    for (int i = 0; i < 10; i++)
        if (*ptr != 0xD0) {
            delete[] buf;
            return false;
        }

    ptr += 16;
    fileSize -= 16;

    if (*ptr != 0x1F) {
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

    ptr += 6;
    fileSize -= 6;

    endAddr--; // PK8000 feature?

    uint16_t progLen = endAddr - begAddr + 1;

    if (progLen > fileSize) {
        delete[] buf;
        return false;
    }

    for (unsigned addr = begAddr; addr <= endAddr; addr++)
        m_as->writeByte(addr, *ptr++);

    fileSize -= (endAddr - begAddr + 1);

    // Find next block
/*    if (m_allowMultiblock && m_tapeRedirector && fileSize > 0)
        while (fileSize > 0 && (*ptr) != 0xE6) {
            ++ptr;
            --fileSize;
        }*/
    //if (fileSize > 0)
    //    --fileSize;

    if (run) {
        m_platform->reset();
        Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(m_platform->getCpu());
        if (cpu) {
            cpu->disableHooks();
            g_emulation->exec((int64_t)cpu->getKDiv() * m_skipTicks);
            cpu->enableHooks();
            cpu->setPC(startAddr);
            /*if (m_allowMultiblock && m_tapeRedirector && fileSize > 0) {
                m_tapeRedirector->assignFile(fileName, "r");
                m_tapeRedirector->openFile();
                m_tapeRedirector->assignFile("", "r");
                m_tapeRedirector->setFilePos(fullSize - fileSize);
            }*/
        }
    }

    return true;
}


Pk8000Keyboard::Pk8000Keyboard()
{
    resetKeys();
}


void Pk8000Keyboard::resetKeys()
{
    for (int i = 0; i < 10; i++)
        m_keys[i] = 0;
    m_rowNo = 0;
}


void Pk8000Keyboard::processKey(EmuKey key, bool isPressed)
{
    if (key == EK_NONE)
        return;

    int i, j;

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
