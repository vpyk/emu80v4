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

#include "Apogey.h"
#include "Emulation.h"
#include "Globals.h"
#include "EmuWindow.h"


using namespace std;

ApogeyCore::ApogeyCore()
{
    //
}



ApogeyCore::~ApogeyCore()
{
   //
}



void ApogeyCore::vrtc(bool isActive)
{
    if (isActive) {
        m_crtRenderer->renderFrame();
    }

    if (isActive)
        g_emulation->screenUpdateReq();
}


void ApogeyCore::draw()
{
    m_window->drawFrame(m_crtRenderer->getPixelData());
    m_window->endDraw();
}


void ApogeyCore::inte(bool isActive)
{
    m_crtRenderer->setFontSetNum(isActive ? 1 : 0);
}


void ApogeyCore::attachCrtRenderer(Crt8275Renderer* crtRenderer)
{
    m_crtRenderer = crtRenderer;
}


bool ApogeyCore::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (PlatformCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<Crt8275Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }
    return false;
}


ApogeyRenderer::ApogeyRenderer()
{
    m_fntCharWidth = 6;
    m_fntCharHeight = 8;
    m_fntLcMask = 0x7;

    m_ltenOffset = false;
    m_rvvOffset  = false;
    m_hgltOffset = false;
    m_gpaOffset  = false;

    m_useRvv     = true;

    m_customDraw = false;
}


uint32_t ApogeyRenderer::getCurFgColor(bool gpa0, bool gpa1, bool hglt)
{
    static const uint32_t c_bwPalette[8] = {0x000000, 0x828282, 0xC5C5C5, 0xEEEEEE, 0x585858, 0xAEAEAE, 0xDFDFDF, 0xFFFFFF};

    switch (m_colorMode) {
    case ColorMode::Mono:
        return hglt ? 0xFFFFFF : 0xC0C0C0;
    case ColorMode::Color:
        return (gpa0 ? 0 : 0x0000FF) | (gpa1 ? 0 : 0x00FF00) | (hglt ? 0 : 0xFF0000);
    case ColorMode::Grayscale:
        return c_bwPalette[(gpa0 ? 0 : 4) + (gpa1 ? 0 : 2) + (hglt ? 0 : 1)];
    }
}


uint32_t ApogeyRenderer::getCurBgColor(bool, bool, bool)
{
    return 0x000000;
}


const uint8_t* ApogeyRenderer::getCurFontPtr(bool, bool, bool)
{
    return m_font + (m_fontNumber ? 1024 : 0);
}


const uint8_t* ApogeyRenderer::getAltFontPtr(bool, bool, bool)
{
    return m_altFont + (m_fontNumber ? (8+12+16)*128 : 0);
}


wchar_t ApogeyRenderer::getUnicodeSymbol(uint8_t chr, bool, bool, bool)
{
    if (m_fontNumber == 0)
        return c_apogeySymbols[chr];
    else
        return chr ? L'·' : L' ';
}


void ApogeyRenderer::setColorMode(ColorMode colorMode)
{
    m_colorMode = colorMode;
    switch (colorMode) {
    case ColorMode::Color:
    case ColorMode::Grayscale:
        m_rvvOffset  = false;
        m_hgltOffset = false;
        m_gpaOffset  = false;
        break;
    case ColorMode::Mono:
        m_rvvOffset  = true;
        m_hgltOffset = true;
        m_gpaOffset  = true;
    }
}


void ApogeyRenderer::toggleColorMode()
{
    switch (m_colorMode) {
    case ColorMode::Mono:
        setColorMode(ColorMode::Color);
        break;
    case ColorMode::Color:
        setColorMode(ColorMode::Grayscale);
        break;
    case ColorMode::Grayscale:
        setColorMode(ColorMode::Mono);
    }
}


bool ApogeyRenderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (Crt8275Renderer::setProperty(propertyName, values))
        return true;

    if (propertyName == "colorMode") {
        if (values[0].asString() == "mono")
            setColorMode(ColorMode::Mono);
        else if (values[0].asString() == "color")
            setColorMode(ColorMode::Color);
        else if (values[0].asString() == "grayscale")
            setColorMode(ColorMode::Grayscale);
        else
            return false;
        return true;
    }

    return false;
}


string ApogeyRenderer::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = Crt8275Renderer::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "colorMode") {
        switch (m_colorMode) {
        case ColorMode::Mono:
            return "mono";
        case ColorMode::Color:
            return "color";
        case ColorMode::Grayscale:
            return "grayscale";
        }
    }
    return "";
}


void ApogeyRomDisk::setPortC(uint8_t value)
{
    bool newA15 = value & 0x80;
    value &= 0x7f;
    m_curAddr = (m_curAddr & ~0x7f00) | (value << 8);
    if (newA15 && !m_oldA15) // перед переключением банка нужно сбросить бит 7 порта B
        m_curAddr = (m_curAddr & 0x7fff) | ((m_curAddr & m_mask) << 15);
    m_oldA15 = newA15;
}


bool ApogeyRomDisk::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (RkRomDisk::setProperty(propertyName, values))
        return true;

    if (propertyName == "extBits") {
        int bits = values[0].asInt();
        m_mask = (1 << bits) - 1;
        return true;
    } else if (propertyName == "sizeMB") {
        int mb = values[0].asInt();
        if (mb < 0)
            mb = 0;
        else if (mb > 2 && mb <= 4)
            mb = 4;
        else if (mb > 4)
            mb = 8;
        switch(mb)
        {
           case 0: m_mask = 0x0f; break; // 512KB
           case 1: m_mask = 0x1f; break; // 1MB
           case 2: m_mask = 0x3f; break; // 2MB
           case 4: m_mask = 0x7f; break; // 4MB
           case 8: m_mask = 0xff; break; // 8MB
        }
        return true;
    }

    return false;
}

