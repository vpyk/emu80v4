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

#include "Rk86.h"
#include "Emulation.h"
#include "Globals.h"
#include "EmuWindow.h"
#include "SoundMixer.h"

using namespace std;


void Rk86Core::vrtc(bool isActive)
{
    if (isActive) {
        m_crtRenderer->renderFrame();
    }

    if (isActive)
        g_emulation->screenUpdateReq();
}


void Rk86Core::draw()
{
    m_window->drawFrame(m_crtRenderer->getPixelData());
    m_window->endDraw();
}


void Rk86Core::inte(bool isActive)
{
    m_beepSoundSource->setValue(isActive ? 1 : 0);
}


void Rk86Core::attachCrtRenderer(Crt8275Renderer* crtRenderer)
{
    m_crtRenderer = crtRenderer;
}


bool Rk86Core::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (PlatformCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<Crt8275Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "beepSoundSource") {
        m_beepSoundSource = static_cast<GeneralSoundSource*>(g_emulation->findObject(values[0].asString()));
        return true;
    }
    return false;
}


Rk86Renderer::Rk86Renderer()
{
    m_fntCharWidth = 6;
    m_fntCharHeight = 8;
    m_fntLcMask = 0x7;

    m_ltenOffset = false;
    m_rvvOffset  = false;
    m_hgltOffset = true;
    m_gpaOffset  = true;

    m_useRvv     = true;

    m_customDraw = false;
}


uint32_t Rk86Renderer::getCurFgColor(bool gpa0, bool gpa1, bool hglt)
{
    switch (m_colorMode) {
        //case RCM_MONO_SIMPLE:
        case RCM_COLOR1:
             {
                 uint32_t res = (gpa1 ? 0x0000FF : 0) | (gpa0 ? 0x00FF00 : 0) | (hglt ? 0xFF0000 : 0);
                if (res == 0)
                    res = 0xC0C0C0;
                return res;
             }
        case RCM_COLOR2:
            return (gpa0 ? 0 : 0xFF0000) | (gpa1 ? 0: 0x00FF00) | (hglt ? 0: 0x0000FF);
        //case RCM_MONO:
        default:
            return 0xC0C0C0;
    }
}


uint32_t Rk86Renderer::getCurBgColor(bool, bool, bool)
{
    return 0x000000;
}


const uint8_t* Rk86Renderer::getCurFontPtr(bool, bool, bool)
{
    return m_font;
}


const uint8_t* Rk86Renderer::getAltFontPtr(bool, bool, bool)
{
    return m_altFont;
}


wchar_t Rk86Renderer::getUnicodeSymbol(uint8_t chr, bool, bool, bool)
{
    return c_rkSymbols[chr];
}


void Rk86Renderer::setColorMode(Rk86ColorMode mode) {
    m_colorMode = mode;
    m_dashedLten = mode == RCM_MONO_ORIG;
    m_useRvv = mode != RCM_MONO_ORIG;
}


void Rk86Renderer::toggleColorMode()
{
    if (m_colorMode == RCM_MONO_ORIG)
        setColorMode(RCM_MONO);
    else if (m_colorMode == RCM_MONO)
        setColorMode(RCM_COLOR1);
    else if (m_colorMode == RCM_COLOR1)
        setColorMode(RCM_COLOR2);
    else if (m_colorMode == RCM_COLOR2)
        setColorMode(RCM_MONO_ORIG);
}


bool Rk86Renderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (Crt8275Renderer::setProperty(propertyName, values))
        return true;

    if (propertyName == "colorMode") {
        if (values[0].asString() == "original")
            setColorMode(RCM_MONO_ORIG);
        else if (values[0].asString() == "mono")
            setColorMode(RCM_MONO);
        else if (values[0].asString() == "color1")
            setColorMode(RCM_COLOR1);
        else if (values[0].asString() == "color2")
            setColorMode(RCM_COLOR2);
        else
            return false;
        return true;
    }

    return false;
}


string Rk86Renderer::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = Crt8275Renderer::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "colorMode") {
        switch (m_colorMode) {
            case RCM_MONO_ORIG:
                return "original";
            case RCM_MONO:
                return "mono";
            case RCM_COLOR1:
                return "color1";
            case RCM_COLOR2:
                return "color2";
        }
    }

    return "";
}


RkPixeltronRenderer::RkPixeltronRenderer()
{
    m_fntCharWidth = 6;

    m_customDraw = true;

    m_ltenOffset = false;
    m_gpaOffset  = false;
}


void RkPixeltronRenderer::customDrawSymbolLine(uint32_t* linePtr, uint8_t symbol, int line, bool lten, bool vsp, bool /*rvv*/, bool gpa0, bool /*gpa1*/, bool /*hglt*/)
{
    int offset = (gpa0 ? 0x400 : 0) + symbol * 8 + (line & 7);
    uint8_t bt = ~m_font[offset];
    if (lten)
        bt = 0x3f;
    else if (vsp)
        bt = 0x00;

    uint32_t fgColor = (bt & 0x40) ? 0xC0C0C0 : 0xFFFFFF;
    uint32_t bgColor = (bt & 0x80) ? 0x404040 : 0x000000;

    for (int i = 0; i < 6; i++) {
        *linePtr++ = (bt & 0x20) ? fgColor : bgColor;
        bt <<= 1;
    }
}
