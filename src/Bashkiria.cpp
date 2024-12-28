/*
 *  bashkiria-2m for Emu80 v. 4.x
 *  © Dmitry Tselikov <bashkiria-2m.narod.ru>, 2022
 *  © Viktor Pykhonin <pyk@mail.ru>, 2024
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
#include "Cpu.h"
#include "PrnWriter.h"

using namespace std;


void Bashkiria2mCore::draw()
{
    m_window->drawFrame(m_crtRenderer->getPixelData());
    m_window->endDraw();
}


void Bashkiria2mCore::vrtc(bool isActive)
{
    if (isActive)
        g_emulation->screenUpdateReq();

    m_pic->irq(0, !isActive);
}


void Bashkiria2mCore::inte(bool isActive)
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


void Bashkiria2mCore::timer(int /*id*/, bool isActive)
{
    if (!isActive)
        return;

    // signal front
    Pit8253Counter* cnt0 = m_pit->getCounter(0);
    Pit8253Counter* cnt2 = m_pit->getCounter(2);

    cnt0->operateForTicks(cnt2->getSumOutTicks());

    if (m_pic) {
        m_pic->irq(1, cnt0->getOut());
    }

    cnt2->resetStats();
}


bool Bashkiria2mCore::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (PlatformCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        m_crtRenderer = static_cast<Bashkiria2mRenderer*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "pic") {
        m_pic = static_cast<Pic8259*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "pit") {
        m_pit = static_cast<Pit8253*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


Bashkiria2mRenderer::Bashkiria2mRenderer()
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

    m_frameBuf = new uint32_t[maxBufSize];
}


Bashkiria2mRenderer::~Bashkiria2mRenderer()
{
    delete[] m_frameBuf;
}


void Bashkiria2mRenderer::renderFrame()
{
    memcpy(m_pixelData, m_frameBuf, m_sizeX * m_sizeY * sizeof(uint32_t));
    swapBuffers();

    if (m_showBorder) {
        m_sizeX = 417; m_sizeY = 288;
        m_aspectRatio = double(m_sizeY) * 4 / 3 / m_sizeX;
    } else {
        m_sizeX = 384; m_sizeY = 256;
        m_aspectRatio = 576.0 * 9 / 704 / 8;
    }
    m_bufSize = m_sizeX * m_sizeY;
    memset(m_frameBuf, 0, m_bufSize * sizeof(uint32_t));
}


void Bashkiria2mRenderer::operate()
{
    if (m_line<256) {
        int addr = (m_page==0 ? 0x1000 : 0x9000) + ((m_line+m_scrollAct)&0xFF);
        int offsetX = m_showBorder?21:0, offsetY = m_showBorder?10:0;
        uint32_t* nColor = m_palette[m_colorMode ? 1 : 0];
        for (int col = 0; col < 48; col++,addr+=256) {
            uint16_t b1 = m_screenMemory[addr];
            uint16_t b2 = m_screenMemory[addr+0x4000]<<1;
            for (int pt = 0; pt < 8; pt++, b1 >>= 1, b2 >>= 1)
                m_frameBuf[(m_line + offsetY) * m_sizeX + col * 8 + pt + offsetX] = nColor[(b1&1)|(b2&2)];
        }
        m_curClock += m_kDiv; ++m_line;
    } else if(m_line<312) {
        m_platform->getCore()->vrtc(true);
        m_curClock += m_kDiv*56; m_line += 56;
    } else {
        renderFrame();
        m_scrollAct = m_scroll;
        m_platform->getCore()->vrtc(false); m_line = 0;
    }
}


void Bashkiria2mRenderer::toggleColorMode()
{
    m_colorMode = !m_colorMode;
}


void Bashkiria2mRenderer::toggleCropping()
{
    m_showBorder = !m_showBorder;
}


void Bashkiria2mRenderer::setPalette(int addr, uint8_t value)
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


bool Bashkiria2mRenderer::setProperty(const string& propertyName, const EmuValuesList& values)
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


string Bashkiria2mRenderer::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "visibleArea") {
        return m_showBorder ? "yes" : "no";
    } else if (propertyName == "crtMode") {
        return u8"384\u00D7256@50.08Hz" ;
    } else if (propertyName == "colorMode") {
        return m_colorMode ? "color" : "mono";
    }

    return "";
}


void Bashkiria2mPalette::writeByte(int addr, uint8_t value)
{
    if (m_renderer) m_renderer->setPalette(addr, value);
}


bool Bashkiria2mPalette::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        m_renderer = static_cast<Bashkiria2mRenderer*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


bool Bashkiria2mPpi8255Circuit1::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "renderer") {
        m_renderer = static_cast<Bashkiria2mRenderer*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "mapper") {
        m_addrSpaceMapper = static_cast<AddrSpaceMapper*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


void Bashkiria2mPpi8255Circuit1::setPortA(uint8_t value)
{
    m_printerData = value;
}


void Bashkiria2mPpi8255Circuit1::setPortB(uint8_t value)
{
    m_renderer->setScroll(value);
}


void Bashkiria2mPpi8255Circuit1::setPortC(uint8_t value)
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


uint8_t Bashkiria2mPpi8255Circuit2::getPortA()
{
    return 0xFF;
}


void Bashkiria2mPpi8255Circuit2::setPortB(uint8_t)//value)
{
}


void Bashkiria2mPpi8255Circuit2::setPortC(uint8_t value)
{
    if(m_fdc) {
        m_fdc->setHead(value&2?0:1);
        m_fdc->setDrive(value&4?1:0);
    }
}


bool Bashkiria2mPpi8255Circuit2::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "fdc") {
        m_fdc = static_cast<Fdc1793*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


Bashkiria2mKeyboard::Bashkiria2mKeyboard()
{
    Bashkiria2mKeyboard::resetKeys();
}


void Bashkiria2mKeyboard::resetKeys()
{
    for (int i = 0; i < 11; i++)
        m_keys[i] = 0;
}


void Bashkiria2mKeyboard::processKey(EmuKey key, bool isPressed)
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


uint8_t Bashkiria2mKeyboard::getMatrixData(int mask)
{
    uint8_t val = 0;
    for (int i = 0; i < 11; i++) {
        if (mask & 1)
            val |= m_keys[i];
        mask >>= 1;
    }

    return val;
}


EmuKey Bashkiria2mKbdLayout::translateKey(PalKeyCode keyCode)
{
    if (m_mode == KLM_JCUKEN)
        return EK_NONE;

    switch (keyCode) {
    case PK_INS:
        return EK_INS;
    case PK_DEL:
        return EK_DEL;
    case PK_PGUP:
        return EK_LANG;
    case PK_KP_0:
        return EK_PHOME;
    case PK_UP:
        return m_upAsNumpad5 ? EK_MENU : EK_UP;

    case PK_EQU:
        return EK_EQU;
    case PK_LBRACKET:
        return EK_LBRACE;
    case PK_RBRACKET:
        return EK_RBRACE;
    case PK_APOSTROPHE:
        return EK_COLON;
    case PK_TILDE:
        return EK_YO;

    case PK_F6:
        return EK_GRAVE;
    case PK_F7:
        return EK_TILDE;
    case PK_F8:
        return EK_LBRACKET;
    case PK_F9:
        return EK_RBRACKET;

    default:
        break;
    }

    EmuKey key = translateCommonKeys(keyCode);
    if (key != EK_NONE)
        return key;

    switch (keyCode) {
    case PK_KP_1:
        return EK_HOME;
    case PK_LCTRL:
    case PK_RCTRL:
        return EK_CTRL;
    case PK_F6:
        return EK_UNDSCR;
    case PK_F10:
        return EK_GRAPH;
    case PK_KP_PLUS:
    case PK_MENU:
        return EK_FIX;
    case PK_F12:
        return EK_STOP;
    case PK_F11:
        return EK_SEL;
    case PK_KP_MUL:
        return EK_INS;
    case PK_KP_DIV:
        return EK_DEL;
    case PK_KP_7:
        return EK_SHOME; //7
    case PK_KP_9:
        return EK_SEND;  // 9
    case PK_KP_3:
        return EK_END;   // 3
    case PK_KP_PERIOD:
        return EK_PEND;  // .
    case PK_KP_0:
        return EK_PEND;  // 0
    case PK_KP_5:
        return EK_MENU;  // 5

    case PK_KP_MINUS:
        return EK_CLEAR;  // - ОЧЭК
    default:
        return EK_NONE;
    }
}


EmuKey Bashkiria2mKbdLayout::translateUnicodeKey(unsigned unicodeKey, PalKeyCode keyCode, bool& shift, bool& lang)
{
    lang = false;
    shift = false;

    if (keyCode == PK_KP_MUL || keyCode == PK_KP_DIV || keyCode == PK_KP_MINUS || keyCode == PK_KP_PLUS)
        return EK_NONE;

    if (unicodeKey >= L'A' && unicodeKey <= L'Z')
        unicodeKey += 0x20; // uppercase latin to lowercase
    else if (unicodeKey >= L'a' && unicodeKey <= L'z')
        unicodeKey -= 0x20; // lowercase latin to uppercase

    switch (unicodeKey) {
    case L'@':
        shift = false;
        return EK_2;
    case L'%':
        shift = false;
        return EK_5;
    case L'^':
        shift = false;
        return EK_6;
    case L'&':
        shift = false;
        return EK_7;
    case L'*':
        shift = false;
        return EK_8;
    case L'(':
        shift = false;
        return EK_9;
    case L')':
        shift = false;
        return EK_0;

    case L'-':
        shift = true;
        return EK_MINUS;
    case L'_':
        shift = false;
        return EK_MINUS;
    case L'=':
        shift = true;
        return EK_EQU;
    case L'+':
        shift = false;
        return EK_EQU;

    case L'[':
        shift = true;
        return EK_LBRACKET;
    case L']':
        shift = true;
        return EK_RBRACKET;
    case L'{':
        shift = true;
        return EK_LBRACE;
    case L'}':
        shift = true;
        return EK_RBRACE;

    case L'\'':
        shift = false;
        return EK_SEMICOLON;
    case L';':
        shift = true;
        return EK_SEMICOLON;

    case L'\"':
        shift = false;
        return EK_COLON;
    case L':':
        shift = true;
        return EK_COLON;

    case L'~':
        shift = false;
        return EK_TILDE;
    case L'`':
        shift = false;
        return EK_GRAVE;
    }

    bool rus = false;
    if ((unicodeKey >= L'А' && unicodeKey <= L'Я') || unicodeKey == L'Ё')
        rus = true;
    else if ((unicodeKey >= L'а' && unicodeKey <= L'я')  || unicodeKey == L'ё') {
        rus = true;
        if (unicodeKey != L'ё')
            unicodeKey -= 0x20;
        else
            unicodeKey -= 0x50;
        //shift = true; // Baskiria does not allow simultaneously shift and lang :(
    }

    if (!rus) {
        EmuKey key = translateCommonUnicodeKeys(unicodeKey, shift, lang);

        if (key >= EK_0 && key <= EK_9)
            shift = !shift;

        if (key == EK_SLASH || key == EK_COMMA || key == EK_PERIOD)
            shift = !shift;

        return key;
    } else {
        lang = true;
        //shift = true;
        switch (unicodeKey) {
        case L'А':
            return EK_F;
        case L'Б':
            return EK_GRAVE;
        case L'В':
            return EK_D;
        case L'Г':
            return EK_U;
        case L'Д':
            return EK_L;
        case L'Е':
            return EK_T;
        case L'Ё':
            return EK_YO;
        case L'Ж':
            return EK_LBRACKET;
        case L'З':
            return EK_P;
        case L'И':
            return EK_B;
        case L'Й':
            return EK_Q;
        case L'К':
            return EK_R;
        case L'Л':
            return EK_K;
        case L'М':
            return EK_V;
        case L'Н':
            return EK_Y;
        case L'О':
            return EK_J;
        case L'П':
            return EK_G;
        case L'Р':
            return EK_H;
        case L'С':
            return EK_C;
        case L'Т':
            return EK_N;
        case L'У':
            return EK_E;
        case L'Ф':
            return EK_A;
        case L'Х':
            return EK_LBRACE;
        case L'Ц':
            return EK_W;
        case L'Ч':
            return EK_X;
        case L'Ш':
            return EK_I;
        case L'Щ':
            return EK_O;
        case L'Ъ':
            return EK_RBRACE;
        case L'Ы':
            return EK_S;
        case L'Ь':
            return EK_M;
        case L'Э':
            return EK_RBRACKET;
        case L'Ю':
            return EK_TILDE;
        case L'Я':
            return EK_Z;
        default:
            return EK_NONE; // normally this should not occur
        }
    }
}


bool Bashkiria2mKbdLayout::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (KbdLayout::setProperty(propertyName, values))
        return true;

    if (propertyName == "upAsNumpad5") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_upAsNumpad5 = values[0].asString() == "yes";
            return true;
        }
    }

    return false;
}


string Bashkiria2mKbdLayout::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = KbdLayout::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "upAsNumpad5")
        return m_upAsNumpad5 ? "yes" : "no";

    return "";
}


void Bashkiria2mKbdMem::writeByte(int, uint8_t)
{
}


uint8_t Bashkiria2mKbdMem::readByte(int addr)
{
    return m_kbd ? m_kbd->getMatrixData(addr&0x100 ? (addr&7)<<8 : addr&0xFF) : 0xFF;
}


bool Bashkiria2mKbdMem::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "kbd") {
        m_kbd = static_cast<Bashkiria2mKeyboard*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


int Bashkiria2mPit8253SoundSource::calcValue()
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


void Bashkiria2mPit8253SoundSource::setGate(bool gate)
{
    if (m_pit) m_pit->getCounter(1)->setGate(gate);
}


void Bashkiria2mPit8253SoundSource::tuneupPit()
{
    m_pit->getCounter(0)->setExtClockMode(true);
}


void Bashkiria2mSpi8251::writeByte(int addr, uint8_t value)
{
    if (m_snd && (addr&1)==1) m_snd->setGate((value&0x20)==0);
}


bool Bashkiria2mSpi8251::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "soundSource") {
        m_snd = static_cast<Bashkiria2mPit8253SoundSource*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}

