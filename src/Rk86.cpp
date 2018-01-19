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

#include "Rk86.h"
#include "Emulation.h"
#include "Globals.h"
#include "EmuWindow.h"
#include "SoundMixer.h"

using namespace std;


Rk86Core::Rk86Core()
{
    m_beepSoundSource = new GeneralSoundSource;
}



Rk86Core::~Rk86Core()
{
    delete m_beepSoundSource;
}


void Rk86Core::vrtc(bool isActive)
{
    if (isActive) {
        m_crtRenderer->renderFrame();
    }
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


void Rk86Renderer::toggleColorMode()
{
    /*if (m_colorMode == RCM_MONO_SIMPLE)
        m_colorMode = RCM_MONO;
    else */if (m_colorMode == RCM_MONO)
        m_colorMode = RCM_COLOR1;
    else if (m_colorMode == RCM_COLOR1)
        m_colorMode = RCM_COLOR2;
    else if (m_colorMode == RCM_COLOR2)
        m_colorMode = RCM_MONO/*_SIMPLE*/;
}


bool Rk86Renderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (Crt8275Renderer::setProperty(propertyName, values))
        return true;

    if (propertyName == "colorMode") {
        /*if (values[0].asString() == "original")
            m_colorMode = RCM_MONO_SIMPLE;
        else */if (values[0].asString() == "mono")
            m_colorMode = RCM_MONO;
        else if (values[0].asString() == "color1")
            m_colorMode = RCM_COLOR1;
        else if (values[0].asString() == "color2")
            m_colorMode = RCM_COLOR2;
        else
            return false;
        return true;
    }

    return false;
}


string Rk86Renderer::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "colorMode") {
        switch (m_colorMode) {
            /*case RCM_MONO_SIMPLE:
                return "original";*/
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
