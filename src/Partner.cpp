/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2024
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

#include "Pal.h"

#include "Partner.h"
#include "Emulation.h"
#include "Globals.h"
#include "EmuWindow.h"
#include "SoundMixer.h"
#include "Cpu.h"
#include "Dma8257.h"
#include "Fdc1793.h"
#include "RkKeyboard.h"
#include "WavReader.h"
#include "Memory.h"

using namespace std;


// Partner implementation

void PartnerCore::attachCpu(Cpu8080Compatible* cpu)
{
    m_cpu = cpu;
}


void PartnerCore::attach8275Renderer(Crt8275Renderer* crtRenderer)
{
    m_crtRenderer = crtRenderer;
}


void PartnerCore::attach8275McpgRenderer(Crt8275Renderer* crtMcpgRenderer)
{
    m_crtMcpgRenderer = crtMcpgRenderer;
}


void PartnerCore::attachMcpgSelector(PartnerMcpgSelector* mcpgSelector)
{
    m_mcpgSelector = mcpgSelector;
}


void PartnerCore::reset()
{
    m_intReq = false;
}


void PartnerCore::inte(bool isActive)
{
    if (isActive && m_intReq && m_cpu->getInte()) {
        m_intReq = false;
        m_cpu->intRst(6);
    }
}



void PartnerCore::vrtc(bool isActive)
{
    if (isActive) {
        m_intReq = true;
        if (m_cpu->getInte()) {
            m_intReq = false;
            m_cpu->intRst(6);
        }

        m_crtRenderer->renderFrame();
        if (m_mcpgSelector->getMcpgEnabled())
            m_crtMcpgRenderer->renderFrame();
    }

    if (isActive)
        g_emulation->screenUpdateReq();
}



void PartnerCore::draw()
{
    m_window->drawFrame(m_crtRenderer->getPixelData());
    if (m_mcpgSelector->getMcpgEnabled())
        m_window->drawOverlay(m_crtMcpgRenderer->getPixelData());
    m_window->endDraw();
}



void PartnerCore::setBeepGate(bool isSet)
{
    m_beepGate = !isSet;
    m_beepSoundSource->setValue(m_beep && m_beepGate ? 1 : 0);
}



void PartnerCore::hrtc(bool isActive, int lc)
{
    if (!isActive) {
        m_beep = !(lc & 4);
        m_beepSoundSource->setValue(m_beep && m_beepGate ? 1 : 0);
    }
}



bool PartnerCore::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (PlatformCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attach8275Renderer(static_cast<Crt8275Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "mcpgCrtRenderer") {
        attach8275McpgRenderer(static_cast<Crt8275Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "mcpgSelector") {
        attachMcpgSelector(static_cast<PartnerMcpgSelector*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "cpu") {
        attachCpu(static_cast<Cpu8080Compatible*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "beepSoundSource") {
        m_beepSoundSource = static_cast<GeneralSoundSource*>(g_emulation->findObject(values[0].asString()));
        return true;
    }
    return false;
}



PartnerAddrSpace::PartnerAddrSpace(string fileName)
{
    AddressableDevice* ns = new NullSpace(0xFF);
    for (int i = 0; i < 9; i++)
        m_memBlocks[i] = ns;

    m_buf = new uint8_t [512];
    if (palReadFromFile(fileName, 0, 512, m_buf) != 512) {
        delete[] m_buf;
        m_buf = nullptr;
    }

    int b,j;
    if (m_buf)
        for (int i = 0; i < 512; i++) {
            b = m_buf[i];
            j = 0;
            while (b & 1 && j < 8) {
                b >>= 1;
                j++;
            }
            if (j == 5 && (m_buf[i] & 0x40) == 0)
                j = 6;
            m_buf[i] = j;
        }
}



PartnerAddrSpace::~PartnerAddrSpace()
{
    delete[] m_buf;
    delete m_memBlocks[8];
}



uint8_t PartnerAddrSpace::readByte(int addr)
{
    int lineNum = m_buf[m_mapNum * 32 + ((addr & 0xf800) >> 11)];
    return m_memBlocks[lineNum]->readByte(addr);
    //return m_memBlocks[m_buf[m_mapNum * 32 + ((addr & 0xf800) >> 7)]]->readByte(addr & 0xf800);
}



void PartnerAddrSpace::writeByte(int addr, uint8_t value)
{
    m_memBlocks[m_buf[m_mapNum * 32 + ((addr & 0xf800) >> 11)]]->writeByte(addr, value);
}



void PartnerAddrSpace::setMemBlock(int blockNum, AddressableDevice* memBlock)
{
    m_memBlocks[blockNum] = memBlock;
}


bool PartnerAddrSpace::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "memBlock") {
        if (values[0].isInt()) {
            setMemBlock(values[0].asInt(), static_cast<AddressableDevice*>(g_emulation->findObject(values[1].asString())));
            return true;
        }
    }
    return false;
}


bool PartnerAddrSpaceSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "addrSpace") {
        attachPartnerAddrSpace(static_cast<PartnerAddrSpace*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


void PartnerRamUpdater::attachDma(Dma8257* dma, int channel)
{
    m_dma = dma;
    m_dmaChannel = channel;
}



void PartnerRamUpdater::operate()
{
    uint8_t temp;
    m_dma->dmaRequest(m_dmaChannel, temp);
    m_curClock += 18 * m_kDiv;
}


bool PartnerRamUpdater::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "dma") {
        if (values[1].isInt()) {
            attachDma(static_cast<Dma8257*>(g_emulation->findObject(values[0].asString())), values[1].asInt());
            return true;
        }
    }

    return false;
}


void PartnerModuleSelector::attachAddrSpaceMappers(AddrSpaceMapper* romWinAddrSpaceMapper, AddrSpaceMapper* ramWinAddrSpaceMapper, AddrSpaceMapper* devWinAddrSpaceMapper)
{
    m_romWinAddrSpaceMapper = romWinAddrSpaceMapper;
    m_ramWinAddrSpaceMapper = ramWinAddrSpaceMapper;
    m_devWinAddrSpaceMapper = devWinAddrSpaceMapper;
}


void PartnerModuleSelector::writeByte(int, uint8_t value)
{
    int moduleNum;
    if (~value & 1)
        moduleNum = 0;
    else if (~value & 2)
        moduleNum = 1;
    else if (~value & 4)
        moduleNum = 2;
    else if (~value & 8)
        moduleNum = 3;
    else
        moduleNum = 5;  // non-existent module number
    m_romWinAddrSpaceMapper->setCurPage(moduleNum);
    m_ramWinAddrSpaceMapper->setCurPage(moduleNum);
    m_devWinAddrSpaceMapper->setCurPage(moduleNum);
}


bool PartnerModuleSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "mappers") {
            attachAddrSpaceMappers(
                static_cast<AddrSpaceMapper*>(g_emulation->findObject(values[0].asString())),
                static_cast<AddrSpaceMapper*>(g_emulation->findObject(values[1].asString())),
                static_cast<AddrSpaceMapper*>(g_emulation->findObject(values[2].asString())));
            return true;
    }

    return false;
}


void PartnerMcpgSelector::writeByte(int, uint8_t value)
{
    m_isMcpgEnabled = !(value & 0x80);
}


uint8_t PartnerPpi8255Circuit::getPortC()
{
    return (m_kbd->getCtrlKeys() & 0x70) | (g_emulation->getWavReader()->getCurValue() ? 0x80 : 0x00);
}


void PartnerPpi8255Circuit::setPortC(uint8_t value)
{
    m_tapeSoundSource->setValue(value & 1);
    m_core->tapeOut(value & 1);

    m_core->setBeepGate(value & 2);
    return;
}


void PartnerPpi8255Circuit::attachCore(PartnerCore* core)
{
    m_core = core;
}


bool PartnerPpi8255Circuit::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (RkPpi8255Circuit::setProperty(propertyName, values))
        return true;

    if (propertyName == "core") {
            attachCore(static_cast<PartnerCore*>(g_emulation->findObject(values[0].asString())));
            return true;
    }

    return false;
}


PartnerRenderer::PartnerRenderer()
{
    m_fntCharWidth = 6;
    m_fntCharHeight = 8;
    m_fntLcMask = 0x7;

    m_ltenOffset = true;
    m_rvvOffset  = true;
    m_hgltOffset = false;
    m_gpaOffset  = false;

    m_useRvv     = true;

    m_customDraw = false;
}


uint32_t PartnerRenderer::getCurFgColor(bool, bool, bool)
{
    return 0xC0C0C0;
}


uint32_t PartnerRenderer::getCurBgColor(bool, bool, bool)
{
    return 0x000000;
}


const uint8_t* PartnerRenderer::getCurFontPtr(bool gpa0, bool gpa1, bool hglt)
{
    return m_font + ((gpa0 ? 1 : 0) + (gpa1 ? 2 : 0) + (hglt ? 4 : 0)) * 1024;
}


const uint8_t* PartnerRenderer::getAltFontPtr(bool gpa0, bool gpa1, bool hglt)
{
    return m_altFont + ((gpa0 ? 1 : 0) + (gpa1 ? 2 : 0) + (hglt ? 4 : 0)) * ((8+12+16)*128);
}


wchar_t PartnerRenderer::getUnicodeSymbol(uint8_t chr, bool gpa0, bool gpa1, bool hglt)
{
    int font = (gpa0 ? 1 : 0) + (gpa1 ? 2 : 0) + (hglt ? 4 : 0);

    if (font == 3)
        return c_rkSymbols[chr];
    else if (font < 2)
        return c_partnerSymbols[font * 128 + chr];
    else
        return (chr || font == 7) ? u'·' : u' ';
}


bool PartnerRenderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (Crt8275Renderer::setProperty(propertyName, values))
        return true;

    return false;
}


PartnerMcpgRenderer::PartnerMcpgRenderer()
{
    m_fntCharWidth = 4;
    m_fntCharHeight = 8;
    m_fntLcMask = 0x7;

    m_ltenOffset = true;
    m_rvvOffset  = true;
    m_hgltOffset = false;
    m_gpaOffset  = false;

    m_useRvv     = true;

    m_customDraw = true;
}


void PartnerMcpgRenderer::attachMcpgRam(Ram* mcpgRam)
{
    m_fontPtr = mcpgRam->getDataPtr();
}


uint32_t PartnerMcpgRenderer::getCurFgColor(bool, bool, bool)
{
    return 0xFFFFFF;
}


uint32_t PartnerMcpgRenderer::getCurBgColor(bool, bool, bool)
{
    return 0x000000;
}


void PartnerMcpgRenderer::customDrawSymbolLine(uint32_t* linePtr, uint8_t symbol, int line, bool lten, bool vsp, bool rvv, bool gpa0, bool gpa1, bool hglt)
{
    int col[4];

    int fntOffs = (rvv ? 0x400 : 0) + symbol * 8 + (line & 0x7);
    col[1] = m_fontPtr[fntOffs] & 0x7;
    col[0] = (m_fontPtr[fntOffs] & 0x38) >> 3;
    col[3] = m_fontPtr[fntOffs + 0x800] & 0x7;
    col[2] = (m_fontPtr[fntOffs + 0x800] & 0x38) >> 3;

    uint32_t bgColor = (gpa0 ? 0xFF0000 : 0) + (gpa1 ? 0x00FF00 : 0) + (hglt ? 0x0000FF : 0) + 0xFF000000;

    for (int pt = 0; pt < 4; pt++) {
        uint32_t color;
        if (col[pt] == 7 || (vsp & !lten))
            color = bgColor;
        else
            color = (col[pt] & 1 ? 0 : 0xFF0000) + (col[pt] & 2 ? 0 : 0x00FF00) + (col[pt] & 4 ? 0 : 0x0000FF) + 0xFF000000;
        if (gpa0 && gpa1 && !hglt && ((symbol & 0xC0) != 0xC0) /*&& ((symbol & 0xC0) != 0x80)*/)
            color = 0x00000000; //transparent
        linePtr[pt] = color;
    }
}


bool PartnerMcpgRenderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (Crt8275Renderer::setProperty(propertyName, values))
        return true;

    if (propertyName == "ram") {
        attachMcpgRam(static_cast<Ram*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


void PartnerFddControlRegister::writeByte(int, uint8_t value)
{
    m_fdc->setDrive(value & 8 ? 1 : 0);
    m_fdc->setHead((value & 0x80) >> 7);
}


bool PartnerFddControlRegister::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "fdc") {
        attachFdc1793(static_cast<Fdc1793*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}
