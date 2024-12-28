/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2024
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

#include "Globals.h"
#include "Mikrosha.h"
#include "Emulation.h"
#include "Platform.h"
#include "EmuWindow.h"
#include "Crt8275Renderer.h"
#include "Ppi8255Circuit.h"
#include "RkKeyboard.h"
#include "SoundMixer.h"
#include "RkPpi8255Circuit.h"
#include "Pit8253.h"
#include "Pit8253Sound.h"

using namespace std;

void MikroshaCore::vrtc(bool isActive)
{
    if (isActive) {
        m_crtRenderer->renderFrame();
    }

    if (isActive)
        g_emulation->screenUpdateReq();

}


void MikroshaCore::draw()
{
    m_window->drawFrame(m_crtRenderer->getPixelData());
    m_window->endDraw();
}


void MikroshaCore::attachCrtRenderer(Crt8275Renderer* crtRenderer)
{
    m_crtRenderer = crtRenderer;
}


bool MikroshaCore::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (PlatformCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<Crt8275Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }
    return false;
}



uint8_t MikroshaPpi8255Circuit::getPortA()
{
    return m_kbd->getMatrixData();
}



uint8_t MikroshaPpi8255Circuit::getPortB()
{
    return 0xFF;
}


/*
uint8_t MikroshaPpi8255Circuit::getPortC()
{
    return m_kbd->getCtrlKeys() & 0xE0; //!!! + ввод с магнитофона!
}*/



void MikroshaPpi8255Circuit::setPortB(uint8_t value)
{
    m_kbd->setMatrixMask(value);
}


void MikroshaPpi8255Circuit::attachPit(Pit8253* pit)
{
    m_pit = pit;
}


void MikroshaPpi8255Circuit::setPortC(uint8_t value)
{
    m_tapeSoundSource->setValue(value & 1);
    m_platform->getCore()->tapeOut(value & 1);
    m_pit->getCounter(2)->setGate(value & 4);
    m_pitSoundSource->setGate(value & 2);
}


bool MikroshaPpi8255Circuit::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (RkPpi8255Circuit::setProperty(propertyName, values))
        return true;

    if (propertyName == "pit") {
        attachPit(static_cast<Pit8253*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "pitSoundSource") {
        m_pitSoundSource = static_cast<MikroshaPit8253SoundSource*>(g_emulation->findObject(values[0].asString()));
        return true;
    }
    return false;
}



void MikroshaPpi2Circuit::attachCrtRenderer(Crt8275Renderer* crtRenderer)
{
    m_crtRenderer = crtRenderer;
}


void MikroshaPpi2Circuit::setPortB(uint8_t value)
{
    m_crtRenderer->setFontSetNum(value & 0x80 ? 1 : 0);
}


bool MikroshaPpi2Circuit::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (Ppi8255Circuit::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<Crt8275Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }
    return false;
}



MikroshaRenderer::MikroshaRenderer()
{
    m_fntCharWidth = 6;
    m_fntCharHeight = 8;
    m_fntLcMask = 0x7;

    m_ltenOffset = false;
    m_rvvOffset  = true;
    m_hgltOffset = true;
    m_gpaOffset  = true;

    m_useRvv     = false;
    m_dashedLten = true;

    m_customDraw = false;
}


uint32_t MikroshaRenderer::getCurFgColor(bool, bool, bool)
{
    return 0xC0C0C0;
}


uint32_t MikroshaRenderer::getCurBgColor(bool, bool, bool)
{
    return 0x000000;
}


const uint8_t* MikroshaRenderer::getCurFontPtr(bool, bool, bool)
{
    return m_font + (m_fontNumber ? 1024 : 0);
}


const uint8_t* MikroshaRenderer::getAltFontPtr(bool, bool, bool)
{
    return m_altFont + (m_fontNumber ? (8+12+16)*128 : 0);
}


wchar_t MikroshaRenderer::getUnicodeSymbol(uint8_t chr, bool, bool, bool)
{
    return c_mikroshaSymbols[m_fontNumber * 128 + chr];
}


void MikroshaPit8253SoundSource::updateStats()
{
    if (m_pit) {
        m_pit->updateState();
        if (m_gate)
            m_sumValue += m_pit->getCounter(2)->getAvgOut();
        }
}


int MikroshaPit8253SoundSource::calcValue()
{
    updateStats();
    int res = m_sumValue;
    m_sumValue = 0;

    for (int i = 0; i < 3; i++)
        m_pit->getCounter(i)->resetStats();

    return res * m_ampFactor;
}


void MikroshaPit8253SoundSource::setGate(bool gate)
{
    updateStats();
    m_gate = gate;
}


EmuKey KristaKbdLayout::translateKey(PalKeyCode keyCode)
{
    switch (keyCode) {
    case PK_F1:
        return EK_NONE;
    case PK_F2:
        return EK_F1;
    case PK_F3:
        return EK_F2;
    case PK_F4:
        return EK_F3;
    case PK_F5:
        return EK_F4;
    case PK_F6:
        return EK_F5;
    default:
        break;
    }

    EmuKey key = RkKbdLayout::translateKey(keyCode);
    return key;
}
