/*
 *  bashkiria-2m for Emu80 v. 4.x
 *  © Dmitry Tselikov <bashkiria-2m.narod.ru>, 2022
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

#include "Globals.h"
#include "Bashkiria.h"
#include "Emulation.h"
#include "EmuWindow.h"
#include "Platform.h"
#include "AddrSpace.h"
#include "SoundMixer.h"
#include "Cpu.h"
#include "TapeRedirector.h"
#include "WavReader.h"
#include "PrnWriter.h"

using namespace std;


void Bashkiria_2M_Core::draw()
{
    m_crtRenderer->renderFrame();
    m_window->drawFrame(m_crtRenderer->getPixelData());
    m_window->endDraw();
}


void Bashkiria_2M_Core::vrtc(bool isActive)
{
    m_pic->irq(0, !isActive);
}


void Bashkiria_2M_Core::inte(bool isActive)
{
    if(!m_pic) return;

    if (!isActive)
        m_pic->inte(false);
    else {
        Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_platform->getCpu());
        if (cpu->getInte())
            m_pic->inte(true);
    }
}


bool Bashkiria_2M_Core::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (PlatformCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        m_crtRenderer = static_cast<Bashkiria_2M_Renderer*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "pic") {
        m_pic = static_cast<Pic8259*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


Bashkiria_2M_Renderer::Bashkiria_2M_Renderer()
{
    setFrequency(15625);
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


void Bashkiria_2M_Renderer::renderFrame()
{
    if (m_showBorderChanged) m_showBorderChanged = false; else return;

    for(int i=0; i<2; ++i) {
        swapBuffers();
        if (m_showBorder) {
            m_sizeX = 417; m_sizeY = 288;
            m_aspectRatio = double(m_sizeY) * 4 / 3 / m_sizeX;
        } else {
            m_sizeX = 384; m_sizeY = 256;
            m_aspectRatio = 576.0 * 9 / 704 / 8;
        }
        m_bufSize = m_sizeX * m_sizeY;
        memset(m_pixelData, 0, m_bufSize * sizeof(uint32_t));
    }
}


void Bashkiria_2M_Renderer::operate()
{
    if (m_line<256) {
        int addr = (m_page==0 ? 0x1000 : 0x9000) + ((m_line+m_scrollAct)&0xFF);
        int offsetX = m_showBorder?21:0, offsetY = m_showBorder?10:0;
        uint32_t* nColor = m_palette[m_colorMode ? 1 : 0];
        for (int col = 0; col < 48; col++,addr+=256) {
            uint16_t b1 = m_screenMemory[addr];
            uint16_t b2 = m_screenMemory[addr+0x4000]<<1;
            for (int pt = 0; pt < 8; pt++, b1 >>= 1, b2 >>= 1)
                m_pixelData[(m_line + offsetY) * m_sizeX + col * 8 + pt + offsetX] = nColor[(b1&1)|(b2&2)];
        }
        m_curClock += m_kDiv; ++m_line;
    } else if(m_line<312) {
        m_platform->getCore()->vrtc(true);
        m_curClock += m_kDiv*56; m_line += 56;
    } else {
        swapBuffers(); m_scrollAct = m_scroll;
        m_platform->getCore()->vrtc(false); m_line = 0;
    }
}


void Bashkiria_2M_Renderer::toggleColorMode()
{
    m_colorMode = !m_colorMode;
}


void Bashkiria_2M_Renderer::toggleCropping()
{
    m_showBorder = !m_showBorder;
    m_showBorderChanged = true;
}


void Bashkiria_2M_Renderer::setPalette(int addr, uint8_t value)
{
    value = ~value;
    for(int i=0; i<2; ++i) {
        uint8_t c = i==0 ? (value&3)*21 : value>>2;
        uint32_t color = 0, level[4]={0,90,180,255};
        for(int j=0; j<3; ++j,c>>=2) {
            color <<= 8; color |= level[c&3];
        }
        m_palette[i][addr&3] = color;
    }
}


bool Bashkiria_2M_Renderer::setProperty(const string& propertyName, const EmuValuesList& values)
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
    } else if (propertyName == "colorMode") {
        if (values[0].asString() == "mono")
            m_colorMode = false;
        else if (values[0].asString() == "color")
            m_colorMode = true;
        else
            return false;
        return true;
    }
    return false;
}


string Bashkiria_2M_Renderer::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "visibleArea") {
        return m_showBorder ? "yes" : "no";
    } else if (propertyName == "crtMode") {
        return u8"384\u00D7256@50Hz" ;
    } else if (propertyName == "colorMode") {
        return m_colorMode ? "color" : "mono";
    }

    return "";
}


void Bashkiria_2M_Palette::writeByte(int addr, uint8_t value)
{
    if (m_renderer) m_renderer->setPalette(addr, value);
}


bool Bashkiria_2M_Palette::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "b2m_renderer") {
        m_renderer = static_cast<Bashkiria_2M_Renderer*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


bool Bashkiria_2M_Ppi8255Circuit1::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "renderer") {
        m_renderer = static_cast<Bashkiria_2M_Renderer*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "mapper") {
        m_addrSpaceMapper = static_cast<AddrSpaceMapper*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


void Bashkiria_2M_Ppi8255Circuit1::setPortA(uint8_t value)
{
    m_printerData = value;
}


void Bashkiria_2M_Ppi8255Circuit1::setPortB(uint8_t value)
{
    m_renderer->setScroll(value);
}


void Bashkiria_2M_Ppi8255Circuit1::setPortC(uint8_t value)
{
    m_renderer->setVideoPage(value>>7);

    m_platform->getCore()->tapeOut((value & 0x40)!=0);

    bool newStrobe = (value & 0x10)!=0;
    if (m_printerStrobe && !newStrobe) {
        g_emulation->getPrnWriter()->printByte(~m_printerData);
    }
    m_printerStrobe = newStrobe;

    m_addrSpaceMapper->setCurPage(value&7);
}


uint8_t Bashkiria_2M_Ppi8255Circuit2::getPortA()
{
    return 0xFF;
}


void Bashkiria_2M_Ppi8255Circuit2::setPortB(uint8_t)//value)
{
}


void Bashkiria_2M_Ppi8255Circuit2::setPortC(uint8_t value)
{
    if(m_fdc) {
        m_fdc->setHead(value&2?0:1);
        m_fdc->setDrive(value&4?1:0);
    }
}


bool Bashkiria_2M_Ppi8255Circuit2::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "fdc") {
        m_fdc = static_cast<Fdc1793*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


void Bashkiria_2M_PitIrqWatchdog::writeByte(int addr, uint8_t value)
{
    if (m_pit) m_pit->writeByte(addr, value);
}


uint8_t Bashkiria_2M_PitIrqWatchdog::readByte(int addr)
{
    return m_pit ? m_pit->readByte(addr) : 0xFF;
}


bool Bashkiria_2M_PitIrqWatchdog::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "pit") {
        m_pit = static_cast<Pit8253*>(g_emulation->findObject(values[0].asString()));
        if (m_pit) m_pit->getCounter(0)->setExtClockMode(true);
        return true;
    } else if (propertyName == "pic") {
        m_pic = static_cast<Pic8259*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


void Bashkiria_2M_PitIrqWatchdog::operate()
{
    if (m_pit) {
        Pit8253Counter* cnt0 = m_pit->getCounter(0);
        Pit8253Counter* cnt2 = m_pit->getCounter(2);

        cnt2->updateState();
        cnt0->operateForTicks(cnt2->getSumOutTicks());

        if (m_pic) {
            m_pic->irq(1, cnt0->getOut());
        }

        //cnt0->resetStats(); ???
        cnt2->resetStats();
    }
    m_curClock += 13*m_kDiv;
}


Bashkiria_2M_Keyboard::Bashkiria_2M_Keyboard()
{
    Bashkiria_2M_Keyboard::resetKeys();
}


void Bashkiria_2M_Keyboard::resetKeys()
{
    for (int i = 0; i < 11; i++)
        m_keys[i] = 0;
}


void Bashkiria_2M_Keyboard::processKey(EmuKey key, bool isPressed)
{
    if (key == EK_NONE)
        return;

    for (int i = 0; i < 11; i++)
        for (int j = 0; j < 8; j++)
            if (key == m_keyMatrix[i][j]) {
                if (isPressed)
                    m_keys[i] |= (1 << j);
                else
                    m_keys[i] &= ~(1 << j);
                break;
            }
}


uint8_t Bashkiria_2M_Keyboard::getMatrixData(int mask)
{
    uint8_t val = 0;
    for (int i = 0; i < 11; i++) {
        if (mask & 1)
            val |= m_keys[i];
        mask >>= 1;
    }

    return val;
}


EmuKey Bashkiria_2M_KbdLayout::translateKey(PalKeyCode keyCode)
{
    switch (keyCode) {
    case PK_KP_0: return EK_NP_0;
    case PK_KP_1: return EK_NP_1;
    case PK_KP_2: return EK_NP_2;
    case PK_KP_3: return EK_NP_3;
    case PK_KP_4: return EK_NP_4;
    case PK_KP_5: return EK_NP_5;
    case PK_KP_6: return EK_NP_6;
    case PK_KP_7: return EK_NP_7;
    case PK_KP_8: return EK_NP_8;
    case PK_KP_9: return EK_NP_9;
    case PK_KP_PERIOD: return EK_NP_PERIOD;
    case PK_KP_DIV:    return EK_DEL;
    case PK_KP_MUL:    return EK_INS;
    case PK_KP_MINUS:  return EK_CLEAR;
    case PK_DOWN:  return EK_NP_2;
    case PK_LEFT:  return EK_NP_4;
    case PK_RIGHT: return EK_NP_6;
    case PK_UP:    return EK_NP_8;
    case PK_DEL:   return EK_LF;
    case PK_LALT:  return EK_FIX;
    case PK_PGUP:  return EK_SEL;
    case PK_PGDN:  return EK_LANG;
    case PK_MENU:  return EK_MENU;
    case PK_F12:   return EK_STOP;
    case PK_F6:    return EK_F6;
    case PK_F7:    return EK_F7;
    case PK_F8:    return EK_F8;
    case PK_F9:    return EK_F9;
    default:
        break;
    }

    EmuKey key = translateCommonKeys(keyCode);
    if (key != EK_NONE)
        return key;

    switch (keyCode) {
    case PK_LCTRL: return EK_CTRL;
    default:       return EK_NONE;
    }
}


EmuKey Bashkiria_2M_KbdLayout::translateUnicodeKey(unsigned unicodeKey, PalKeyCode keyCode, bool& shift, bool& lang)
{
    if (keyCode == PK_KP_MUL || keyCode == PK_KP_PLUS || keyCode == PK_KP_MINUS || keyCode == PK_KP_DIV)
        return EK_NONE;

    if (unicodeKey == L'_')
        return EK_UNDSCR;

    return translateCommonUnicodeKeys(unicodeKey, shift, lang);
}


void Bashkiria_2M_KbdMem::writeByte(int, uint8_t)
{
}


uint8_t Bashkiria_2M_KbdMem::readByte(int addr)
{
    return m_kbd ? m_kbd->getMatrixData(addr&0x100 ? (addr&7)<<8 : addr&0xFF) : 0xFF;
}


bool Bashkiria_2M_KbdMem::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "kbd") {
        m_kbd = static_cast<Bashkiria_2M_Keyboard*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


int Bashkiria_2M_Pit8253SoundSource::calcValue()
{
    int res = 0;

    if (m_pit) {
        Pit8253Counter* cnt1 = m_pit->getCounter(1);
        cnt1->updateState();
        res = cnt1->getAvgOut();
        cnt1->resetStats();
    }

    return res * m_ampFactor;
}


void Bashkiria_2M_Pit8253SoundSource::setGate(bool gate)
{
    if (m_pit) m_pit->getCounter(1)->setGate(gate);
}


void Bashkiria_2M_Spi8251::writeByte(int addr, uint8_t value)
{
    if (m_snd && (addr&1)==1) m_snd->setGate((value&0x20)==0);
}


bool Bashkiria_2M_Spi8251::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "b2m_soundsource") {
        m_snd = static_cast<Bashkiria_2M_Pit8253SoundSource*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}

