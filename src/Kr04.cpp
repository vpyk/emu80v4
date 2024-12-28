/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2021-2024
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
#include "AddrSpace.h"
#include "Kr04.h"
#include "Emulation.h"
#include "Platform.h"
#include "PlatformCore.h"
#include "EmuWindow.h"
#include "SoundMixer.h"
#include "WavReader.h"
#include "RkKeyboard.h"
#include "Pit8253.h"
#include "Cpu.h"
#include "Crt8275.h"

using namespace std;


void Kr04Core::vrtc(bool isActive)
{
    if (isActive) {
        m_crtRenderer->renderFrame();
    }

    if (isActive)
        g_emulation->screenUpdateReq();

}


void Kr04Core::hrtc(bool isActive, int)
{
    if (isActive) {
        //Pit8253Counter* cnt0 = m_pit->getCounter(0);
        Pit8253Counter* cnt1 = m_pit->getCounter(1);
        Pit8253Counter* cnt2 = m_pit->getCounter(2);

        /*bool prev0 = cnt0->getOut();
        cnt0->operateForTicks(1);
        if (!prev0 && cnt0->getOut())
            m_cpu->intRst(7);*/

        cnt1->operateForTicks(1);

        cnt2->setGate(!cnt1->getOut());
    }
}


void Kr04Core::inte(bool isActive)
{
    if (isActive && m_intReq && m_cpu->getInte()) {
        m_intReq = false; // comment out this when cpu interrupt code will be ready
        m_cpu->intRst(7);
    }
}


void Kr04Core::timer(int /*id*/, bool isActive)
{
    m_intReq = isActive;
    if (isActive) {
        if (m_cpu->getInte()) {
            //m_intReq = false;
            m_cpu->intRst(7);
        }
    }
}


void Kr04Core::draw()
{
    m_window->drawFrame(m_crtRenderer->getPixelData());
    m_window->endDraw();
}


void Kr04Core::attachCrtRenderer(Crt8275Renderer* crtRenderer)
{
    m_crtRenderer = crtRenderer;
}


bool Kr04Core::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (PlatformCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<Crt8275Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "pit") {
        m_pit = static_cast<Pit8253*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "kr04PitSoundSource") {
        m_kr04PitSoundSource = static_cast<Kr04Pit8253SoundSource*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "cpu") {
        m_cpu = static_cast<Cpu8080Compatible*>(g_emulation->findObject(values[0].asString()));
        return true;
    }
    return false;
}


Kr04Renderer::Kr04Renderer()
{
    m_useAltFont = false;

    m_fntCharWidth = 8;
    m_visibleOffsetX = -4;

    m_customDraw = true;
    m_ltenOffset = false;
}


void Kr04Renderer::setColorMode(Kr04ColorMode colorMode)
{
    m_colorMode = colorMode;
}


void Kr04Renderer::toggleColorMode()
{
    if (m_colorMode == KCM_MONO)
        setColorMode(KCM_COLOR);
    else if (m_colorMode == KCM_COLOR)
        setColorMode(KCM_COLORMODULE);
    else if (m_colorMode == KCM_COLORMODULE)
        setColorMode(KCM_MONO);
}


void Kr04Renderer::customDrawSymbolLine(uint32_t* linePtr, uint8_t symbol, int line, bool lten, bool vsp, bool rvv, bool gpa0, bool gpa1, bool hglt)
{
    int offset = 0x8000 | (rvv ? 0x4000 : 0) | (hglt ? 0x2000 : 0) | (gpa1 ? 0x1000 : 0) | (gpa0 ? 0x0800 : 0) | ((symbol & 0x40) << 4) | (line << 6) | (symbol & 0x3F);
    uint8_t bt = vsp ? 0 : m_memory->readByte(offset);

    const bool p2 = true; // jumper

    for (int pt = 0; pt < (m_hiRes ? 8 : 4); pt++) {
        int idx = (bt & 1) | (bt >> 3 & 2) | (lten ? 4 : 0) | (!m_hiRes ? 8 : 0) | (p2 ? 0x10 : 0);
        int vidOut = c_d46[idx];
        uint32_t color;
        switch (m_colorMode) {
        case KCM_COLOR:
            color = (vidOut & 1 ? 0xff0000 : 0) | (vidOut & 2 ? 0x00ff00 : 0) | (vidOut & 4 ? 0x0000ff : 0);
            break;
        case KCM_COLORMODULE:
            color = m_colorCircuit->translateColor(vidOut & 7);
            break;
        default:
        case KCM_MONO:
            uint8_t amp = c_bwPalette[vidOut >> 4 & 7];
            color = amp | (amp << 8) | (amp << 16);
            break;
        }
        *linePtr++ = color;
        if (!m_hiRes)
            *linePtr++ = color;
        bt >>= 1;
    }
}


wchar_t Kr04Renderer::getUnicodeSymbol(uint8_t chr, bool, bool, bool)
{
    ;
    return m_crt->getFrame()->nLines <= 10 ? c_rkSymbols[chr] : 0;
}


bool Kr04Renderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (Crt8275Renderer::setProperty(propertyName, values))
        return true;

    if (propertyName == "addrSpace") {
        m_memory = static_cast<AddrSpace*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else  if (propertyName == "colorCircuit") {
        m_colorCircuit = static_cast<Kr04PpiColor8255Circuit*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "colorMode") {
        if (values[0].asString() == "mono")
            setColorMode(KCM_MONO);
        else if (values[0].asString() == "color")
            setColorMode(KCM_COLOR);
        else if (values[0].asString() == "colorModule")
            setColorMode(KCM_COLORMODULE);
        else
            return false;

        return true;
    }

    return false;
}


string Kr04Renderer::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = Crt8275Renderer::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "colorMode")
        switch (m_colorMode) {
        case KCM_MONO:
            return "mono";
        case KCM_COLOR:
            return "color";
        case KCM_COLORMODULE:
            return "colorModule";
        }

    return "";
}


uint8_t Kr04Ppi8255Circuit::getPortB()
{
    uint16_t bits = m_kbd->getMatrixData();
    return (bits & 0x7F) | ((bits & 0x400) >> 3);
}



uint8_t Kr04Ppi8255Circuit::getPortC()
{
    uint16_t bits = m_kbd->getMatrixData();
    return ((bits & 0x300) >> 3) | ((bits & 0x80) >> 3) | (g_emulation->getWavReader()->getCurValue() ? 0x80 : 0x00);
}



void Kr04Ppi8255Circuit::setPortA(uint8_t value)
{
    m_kbd->setMatrixMask(value);
}


void Kr04Ppi8255Circuit::setPortC(uint8_t value)
{
    if (m_portCLoInputMode)
        value |= 0x0F;

    if (m_tapeSoundSource)
        m_tapeSoundSource->setValue(value & 8);
    m_platform->getCore()->tapeOut(value & 8);

    if (m_mapper)
        m_mapper->setCurPage(value & 0x3);

    if (m_renderer)
        m_renderer->setHiRes(value & 4);
}


void Kr04Ppi8255Circuit::setPortCLoMode(bool isInput)
{
    m_portCLoInputMode = isInput;
    if (isInput)
        setPortC(getPortC() | 0x03);
}



void Kr04Ppi8255Circuit::attachKeyboard(KrKeyboard* kbd)
{
    m_kbd = kbd;
}


bool Kr04Ppi8255Circuit::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "keyboard") {
        attachKeyboard(static_cast<KrKeyboard*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "tapeSoundSource") {
        m_tapeSoundSource = static_cast<GeneralSoundSource*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "mapper") {
        m_mapper = static_cast<AddrSpaceMapper*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "crtRenderer") {
        m_renderer = static_cast<Kr04Renderer*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


void Kr04PpiColor8255Circuit::setPortA(uint8_t value)
{
    m_portA = value;
}


void Kr04PpiColor8255Circuit::setPortB(uint8_t value)
{
    m_portB = value;
}


void Kr04PpiColor8255Circuit::setPortC(uint8_t value)
{
    m_portC = value;
}


uint32_t Kr04PpiColor8255Circuit::translateColor(int rgb)
{
    int color2 = (((rgb << 1) | (rgb >> 1)) & 2) | (((rgb << 1) | (rgb << 2)) & 4);
    int b = (m_portA >> color2) & 0x03;
    int g = (m_portB >> color2) & 0x03;
    int r = (m_portC >> color2) & 0x03;
    return (c_colors[r]) << 16 | (c_colors[g] << 8) | c_colors[b];
}


void Kr04Pit8253SoundSource::tuneupPit()
{
    //m_pit->getCounter(0)->setExtClockMode(true);
    m_pit->getCounter(1)->setExtClockMode(true);
}


int Kr04Pit8253SoundSource::calcValue()
{
    int res = 0;

    if (m_pit) {
        Pit8253Counter* cnt2 = m_pit->getCounter(2);
        cnt2->updateState();
        res += MAX_SND_AMP - (cnt2->getAvgOut());
        cnt2->resetStats();

        //m_pit->getCounter(0)->resetStats();
        m_pit->getCounter(2)->resetStats();
    }

    return res * m_ampFactor;
}


Kr04FileLoader::Kr04FileLoader()
{
    m_skipTicks = 1200000;
}


void Kr04FileLoader::afterReset()
{
    Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_platform->getCpu());

    // Send Spc/Spc/Cr
    Keyboard* kbd = m_platform->getKeyboard();
    for (int i = 0; i < 3; i++) {
        // Mode # 3, press space 3 times
        g_emulation->exec((int64_t)cpu->getKDiv() * 50000, true);
        kbd->processKey(EK_SPACE, true);
        g_emulation->exec((int64_t)cpu->getKDiv() * 50000, true);
        kbd->processKey(EK_SPACE, false);
    }
    // Press CR
    g_emulation->exec((int64_t)cpu->getKDiv() * 50000, true);
    kbd->processKey(EK_CR, true);
    g_emulation->exec((int64_t)cpu->getKDiv() * 50000, true);
    kbd->processKey(EK_CR, false);
    g_emulation->exec((int64_t)cpu->getKDiv() * 500000, true);

    AddressableDevice* io = cpu->getIoAddrSpace();
    io->writeByte(0xC2, (io->readByte(0xC2) & 0xFC) | 0x01);
}


int Kr04CpuWaits::getCpuWaitStates(int memTag, int, int normalClocks)
{
    static const int waits[19] = {0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 2, 3, 0, 3, 0, 0, 4, 5, 4};
    int w = waits[normalClocks];
    if (memTag != 1 && normalClocks > 4)
        w--;
    return w;
}
