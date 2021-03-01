/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2020
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

#include "Lvov.h"
#include "Emulation.h"
#include "EmuWindow.h"
#include "Platform.h"
#include "AddrSpace.h"
#include "SoundMixer.h"
#include "Cpu.h"
#include "TapeRedirector.h"

using namespace std;


void LvovCore::draw()
{
    m_crtRenderer->renderFrame();
    m_window->drawFrame(m_crtRenderer->getPixelData());
    m_window->endDraw();
}


void LvovCore::attachCrtRenderer(CrtRenderer* crtRenderer)
{
    m_crtRenderer = crtRenderer;
}


bool LvovCore::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (PlatformCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<LvovRenderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }
    return false;
}


LvovRenderer::LvovRenderer()
{
    const int pixelFreq = 5; // MHz
    const int maxBufSize = 261 * 288; // 626 = 704 / 13.5 * pixelFreq

    m_sizeX = m_prevSizeX = 256;
    m_sizeY = m_prevSizeY = 256;
    m_aspectRatio = m_prevAspectRatio = 5184. / 704 / pixelFreq;
    m_bufSize = m_prevBufSize = m_sizeX * m_sizeY;
    m_pixelData = new uint32_t[maxBufSize];
    m_prevPixelData = new uint32_t[maxBufSize];
    memset(m_pixelData, 0, m_bufSize * sizeof(uint32_t));
    memset(m_prevPixelData, 0, m_prevBufSize * sizeof(uint32_t));
}


void LvovRenderer::renderFrame()
{
    swapBuffers();

    int offsetX = 0;
    int offsetY = 0;

    if (m_showBorder) {
        m_sizeX = 261;
        m_sizeY = 288;
        memset(m_pixelData, 0, m_sizeX * m_sizeY * sizeof(uint32_t));
        offsetX = 0;
        offsetY = 25;
        m_aspectRatio = double(m_sizeY) * 4 / 3 / m_sizeX;
    } else {
        m_sizeX = 256;
        m_sizeY = 256;
        offsetX = offsetY = 0;
        m_aspectRatio = 576.0 * 9 / 704 / 5;
    }

    for (int row = 0; row < 256; row++)
        for (int col = 0; col < 64; col++) {
            int addr = row * 64 + col;
            uint8_t bt = m_screenMemory[addr];
            for (int p = 0; p < 4; p++) {
                int colorBits = ((bt & 0x80) >> 6) | ((bt & 0x08) >> 3);
                int r, g, b;
                switch (colorBits) {
                case 0:
                    r = (m_paletteByte & 8) == (m_paletteByte >> 1 & 8);
                    g = (m_paletteByte >> 5 & 1);
                    b = (m_paletteByte & 4) == (m_paletteByte >> 4 & 4);
                    break;
                case 1:
                    r = (m_paletteByte & 1) == (m_paletteByte >> 4 & 1);
                    g = (m_paletteByte >> 5 & 1);
                    b = ~(m_paletteByte >> 6) & 1;
                    break;
                case 2:
                    r = m_paletteByte >> 4 & 1;
                    g = ~(m_paletteByte >> 5) & 1;
                    b = m_paletteByte >> 6 & 1;
                    break;
                default: //case 3:
                    r = ~(m_paletteByte >> 4) & 1;
                    g = (m_paletteByte & 2) == (m_paletteByte >> 4 & 2);
                    b = (m_paletteByte >> 6 & 1);
                    break;
                }
                uint32_t color = lvovPalette[r << 2 | g << 1 | b ];
                //uint32_t color = lvovPalette[((bt & 0x80) >> 6) | ((bt & 0x08) >> 3)];
                bt <<= 1;
                m_pixelData[(row + offsetY) * m_sizeX + col * 4 + p + offsetX] = color;
            }
        }
}


void LvovRenderer::toggleCropping()
{
    m_showBorder = !m_showBorder;
}


bool LvovRenderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "screenMemory") {
        attachScreenMemory(static_cast<Ram*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "visibleArea") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_showBorder = values[0].asString() == "yes";
            return true;
        }
    }
    return false;
}


string LvovRenderer::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "visibleArea") {
        return m_showBorder ? "yes" : "no";
    } else if (propertyName == "crtMode") {
            return u8"256\u00D7256@48.83Hz" ;
    }

    return "";
}


bool LvovPpi8255Circuit1::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "renderer") {
        attachRenderer(static_cast<LvovRenderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "tapeSoundSource") {
        m_tapeSoundSource = static_cast<GeneralSoundSource*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "mapper") {
        attachAddrSpaceMapper(static_cast<AddrSpaceMapper*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


void LvovPpi8255Circuit1::setPortC(uint8_t value)
{
    if (m_tapeSoundSource)
        m_tapeSoundSource->setValue(value & 1);
    m_platform->getCore()->tapeOut(value & 1);

    m_addrSpaceMapper->setCurPage(value >> 1 & 1);
}


void LvovPpi8255Circuit1::setPortB(uint8_t value)
{
    m_renderer->setPaletteByte(value & 0x7F);
}


void LvovPpi8255Circuit2::setPortA(uint8_t value)
{
    m_kbd->setMatrix1Mask(value);
}


uint8_t LvovPpi8255Circuit2::getPortB()
{
    return m_kbd->getMatrix1Data();
}


void LvovPpi8255Circuit2::setPortC(uint8_t value)
{
    m_kbd->setMatrix2Mask(value & 0x0F);
}


uint8_t LvovPpi8255Circuit2::getPortC()
{
    return m_kbd->getMatrix2Data() << 4;
}


bool LvovPpi8255Circuit2::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "lvovKeyboard") {
        attachLvovKeyboard(static_cast<LvovKeyboard*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


LvovKeyboard::LvovKeyboard()
{
    resetKeys();
}


void LvovKeyboard::resetKeys()
{
    for (int i = 0; i < 8; i++)
        m_keys1[i] = 0;

    for (int i = 0; i < 4; i++)
        m_keys2[i] = 0;

    m_mask1 = 0;
    m_mask2 = 0;
}

void LvovKeyboard::processKey(EmuKey key, bool isPressed)
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
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
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



void LvovKeyboard::setMatrix1Mask(uint8_t mask)
{
    m_mask1 = ~mask;
}


void LvovKeyboard::setMatrix2Mask(uint8_t mask)
{
    m_mask2 = ~mask;
}


uint8_t LvovKeyboard::getMatrix1Data()
{
    uint8_t val = 0;
    uint8_t mask = m_mask1;
    for (int i = 0; i < 8; i++) {
        if (mask & 1)
            val |= m_keys1[i];
        mask >>= 1;
    }

    return ~val;
}


uint8_t LvovKeyboard::getMatrix2Data()
{
    uint8_t val = 0;
    uint8_t mask = m_mask2;
    for (int i = 0; i < 4; i++) {
        if (mask & 1)
            val |= m_keys2[i];
        mask >>= 1;
    }

    return ~val;
}


EmuKey LvovKbdLayout::translateKey(PalKeyCode keyCode)
{
    switch (keyCode) {
    case PK_ESC:
        return EK_CLEAR;
    case PK_PGUP:
        return EK_RUS;
    case PK_KP_PLUS:
        return EK_F0;
    case PK_KP_MUL:
        return EK_GT;
    case PK_KP_MINUS:
        return EK_UNDSCR;
    case PK_INS:
        return EK_LAT;
    default:
        break;
    }

    EmuKey key = translateCommonKeys(keyCode);
    if (key != EK_NONE)
        return key;

    switch (keyCode) {
    case PK_F6:
        return EK_FG;
    case PK_F7:
        return EK_FB;
    case PK_F8:
        return EK_FR;
    case PK_F9:
        return EK_SPK;
    case PK_F10:
        return EK_CD;
    case PK_F11:
        return EK_PRN;
    case PK_F12:
        return EK_SCR;
    case PK_DEL:
        return EK_VR;

    default:
        return EK_NONE;
    }
}


EmuKey LvovKbdLayout::translateUnicodeKey(unsigned unicodeKey, PalKeyCode keyCode, bool& shift, bool& lang)
{
    if (keyCode == PK_KP_MUL || keyCode == PK_KP_PLUS || keyCode == PK_KP_MINUS)
        return EK_NONE;

    if (unicodeKey == L'_')
        return EK_UNDSCR;

    return translateCommonUnicodeKeys(unicodeKey, shift, lang);
}


int LvovCpuWaits::getCpuWaitStates(int /*memTag*/, int opcode, int /*normalClocks*/)
{
    if (opcode == 0xD3 || opcode == 0xDB) {
        m_curWaits++;
        if (m_curWaits == 2)
            m_curWaits = 0;
        return m_curWaits ? 1 : 0;
    }

    return 0;
}


int LvovCpuCycleWaits::getCpuCycleWaitStates(int memTag)
{
    if (memTag == 1) {
        m_curWaits++;
        if (m_curWaits == 2/*4*/)
            m_curWaits = 0;
        return 2 + (m_curWaits ? 0 : 1);
    }

    return 0;
}


bool LvovFileLoader::loadFile(const std::string& fileName, bool run)
{
    static const uint8_t headerSeqCas[8] = {0x1F, 0xA6, 0xDE, 0xBA, 0xCC, 0x13, 0x7D, 0x74};
    static const uint8_t headerSeqLvt[9] = {0x4C, 0x56, 0x4F, 0x56, 0x2F, 0x32, 0x2E, 0x30, 0x2F};
    static const uint8_t headerSeqLvtDump[16] = {0x4C, 0x56, 0x4F, 0x56, 0x2F, 0x44, 0x55, 0x4D, 0x50, 0x2F, 0x32, 0x2E, 0x30, 0x2F, 0x48, 0x2B};

    m_fileName = fileName;

    m_buf = palReadFile(m_fileName, m_fileSize, false);
    if (!m_buf)
        return false;

    m_fullSize = m_fileSize;

    if (m_fileSize < 9) {
        delete[] m_buf;
        return false;
    }

    m_ptr = m_buf;

    if (memcmp(m_ptr, headerSeqLvt, 9) == 0) {
        // load Lvt
        if (m_fileSize < 16) {
            delete[] m_buf;
            return false;
        }

        m_ptr += 9;
        m_fileSize -= 9;

        uint8_t type = *m_ptr;

        m_ptr += 7;
        m_fileSize -= 7;

        if (type == 0xD0)
            return loadBinary(run);
        else if (type == 0xD3)
            return loadBasic(run);

    } else if (memcmp(m_ptr, headerSeqLvtDump, 16) == 0) {
        // load Sav (dump)
        if (m_fileSize < 82219 - 14) {
            delete[] m_buf;
            return false;
        }

        // skip signature
        m_ptr += 17;

        loadDump(run);

        return true;

    } else if (memcmp(m_ptr, headerSeqCas, 8) == 0) {
        // load Cas
        if (m_fileSize < 40) {
            delete[] m_buf;
            return false;
        }

        m_ptr += 8;
        m_fileSize -= 8;

        uint8_t type = *m_ptr;

        if (type != 0xD0 && type != 0xD3) {
            delete[] m_buf;
            return false;
        }

        for (int i = 0; i < 10; i++)
            if (*m_ptr != type) {
                delete[] m_buf;
                return false;
            }

        m_ptr += 16;
        m_fileSize -= 16;

        if (*m_ptr != headerSeqCas[0]) {
            m_ptr += 8;
            m_fileSize -= 8;
        }

        if (m_fileSize < 8 || memcmp(m_ptr, headerSeqCas, 8) != 0) {
            delete[] m_buf;
            return false;
        }

        m_ptr += 8;
        m_fileSize -= 8;

        if (type == 0xD0)
            return loadBinary(run);
        else if (type == 0xD3)
            return loadBasic(run);
    } else {
        delete[] m_buf;
        return false;
    }

    return true;
}


bool LvovFileLoader::loadBinary(bool run)
{
    if (m_fileSize < 7) {
        delete[] m_buf;
        return false;
    }

    uint16_t begAddr = (m_ptr[1] << 8) | m_ptr[0];
    uint16_t endAddr = (m_ptr[3] << 8) | m_ptr[2];
    uint16_t startAddr = (m_ptr[5] << 8) | m_ptr[4];

    m_ptr += 6;
    m_fileSize -= 6;

    uint16_t progLen = endAddr - begAddr + 1;

    if (progLen > m_fileSize) {
        delete[] m_buf;
        return false;
    }

    if (run) {
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
        m_as->writeByte(addr, *m_ptr++);

    m_fileSize -= (endAddr - begAddr + 1);

    if (run && m_allowMultiblock && m_tapeRedirector)
        setupMultiblock();

    return true;
}


bool LvovFileLoader::loadBasic(bool run)
{
    if (m_fileSize < 1) {
        delete[] m_buf;
        return false;
    }

    Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(m_platform->getCpu());

    m_platform->reset();
    if (cpu) {
        cpu->disableHooks();
        g_emulation->exec((int64_t)cpu->getKDiv() * m_skipTicks);
        cpu->enableHooks();
    }

    unsigned addr;
    for (addr = m_as->readByte(0x0243) | m_as->readByte(0x0244) << 8; m_fileSize; addr++, m_fileSize--)
        m_as->writeByte(addr, *m_ptr++);
    m_as->writeByte(0x0245, addr & 0xFF);
    m_as->writeByte(0x0246, addr >> 8);
    m_as->writeByte(0x0247, addr & 0xFF);
    m_as->writeByte(0x0248, addr >> 8);
    m_as->writeByte(0x0249, addr & 0xFF);
    m_as->writeByte(0x024A, addr >> 8);

    if (run) {
        m_as->writeByte(0xBE10, 0x0D);
        m_as->writeByte(0xBE11, 0x4E);
        m_as->writeByte(0xBE12, 0x55);
        m_as->writeByte(0xBE13, 0x52);
        m_as->writeByte(0xBE14, 0x04);
    }
    cpu->setSP(0xAFC1);
    cpu->setPC(0x02FD);

    if (run && m_allowMultiblock && m_tapeRedirector) {
        setupMultiblock();

        // Workaround to suppress closing file by CloseFileHook
        cpu->disableHooks();
        g_emulation->exec((int64_t)cpu->getKDiv() * 500000);
        cpu->enableHooks();
    }

    return true;
}


void LvovFileLoader::loadDump(bool run)
{
    m_platform->reset();
    Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(m_platform->getCpu());
    if (cpu) {
        cpu->disableHooks();
        g_emulation->exec((int64_t)cpu->getKDiv() * m_skipTicks);
        cpu->enableHooks();

        for (unsigned addr = 0; addr <= 0xFFFF; addr++)
            m_as->writeByte(addr, *m_ptr++);

        for (unsigned addr = 0; addr <= 0x3FFF; addr++)
            m_video->writeByte(addr, *m_ptr++);

        m_io->writeByte(0xC3, m_ptr[0xC3]);
        m_io->writeByte(0xC0, m_ptr[0xC0]);
        m_io->writeByte(0xC1, m_ptr[0xC1]);
        m_io->writeByte(0xC2, m_ptr[0xC2]);

        m_io->writeByte(0xD3, m_ptr[0xD3]);
        m_io->writeByte(0xD0, m_ptr[0xD0]);
        m_io->writeByte(0xD1, m_ptr[0xD1]);
        m_io->writeByte(0xD2, m_ptr[0xD2]);

        m_ptr += 0x100;

        uint16_t rp = *m_ptr++ << 8;
        rp |= *m_ptr++;
        cpu->setBC(rp);

        rp = *m_ptr++ << 8;
        rp |= *m_ptr++;
        cpu->setDE(rp);

        rp = *m_ptr++ << 8;
        rp |= *m_ptr++;
        cpu->setHL(rp);

        rp = *m_ptr++ << 8;
        rp |= *m_ptr++;
        cpu->setAF(rp);

        rp = *m_ptr++;
        rp |= *m_ptr++ << 8;
        cpu->setSP(rp);

        rp = *m_ptr++;
        rp |= *m_ptr++ << 8;
        cpu->setPC(rp);

        if (!run)
            m_platform->reset();
    }
}


void LvovFileLoader::setupMultiblock()
{
    string ext = m_fileName.substr(m_fileName.size() - 4, 4);
    if (ext.substr(0, 3) != ".lv" && ext.substr(0, 3) != ".LV") {
        // CAS
        if (m_fileSize > 0) {
            m_tapeRedirector->assignFile(m_fileName, "r");
            m_tapeRedirector->openFile();
            m_tapeRedirector->assignFile("", "r");
            m_tapeRedirector->setFilePos(m_fullSize - m_fileSize);
        }
    } else {
        // LVT
        char letter = ext[3];
        if (letter == 't' || letter == 'T')
            letter = '0';
        else
            letter += 1;
        m_fileName[m_fileName.size() - 1] = letter;
        m_tapeRedirector->assignFile(m_fileName, "r");
        m_tapeRedirector->openFile();
        m_tapeRedirector->assignFile("", "r");
    }
}


bool LvovFileLoader::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (FileLoader::setProperty(propertyName, values))
        return true;

    if (propertyName == "ioAddrSpace") {
        m_io = static_cast<AddressableDevice*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "videoAddrSpace") {
        m_video = static_cast<AddressableDevice*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}
