/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2022-2024
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
#include "Palmira.h"
#include "Emulation.h"
#include "EmuWindow.h"
#include "Memory.h"
#include "AddrSpace.h"

using namespace std;


void PalmiraCore::vrtc(bool isActive)
{
    if (isActive) {
        m_crtRenderer->renderFrame();
    }

    if (isActive)
        g_emulation->screenUpdateReq();
}


void PalmiraCore::draw()
{
    m_window->drawFrame(m_crtRenderer->getPixelData());
    m_window->endDraw();
}


void PalmiraCore::attachCrtRenderer(Crt8275Renderer* crtRenderer)
{
    m_crtRenderer = crtRenderer;
}


bool PalmiraCore::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (PlatformCore::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        attachCrtRenderer(static_cast<Crt8275Renderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }
    return false;
}


PalmiraRenderer::PalmiraRenderer()
{
    m_useAltFont = false;

    m_fntCharWidth = 8;
    m_fntCharHeight = 16;
    m_fntLcMask = 0xF;
}


const uint8_t* PalmiraRenderer::getCurFontPtr(bool, bool, bool)
{
    return m_useExtFont ? m_extFontPtr : (m_font + (m_fontNumber * 2048));
}


void PalmiraRenderer::attachFontRam(Ram* fontRam)
{
    m_extFontPtr = fontRam->getDataPtr();
}


void PalmiraRenderer::setColorMode(Rk86ColorMode)
{
    m_colorMode = RCM_COLOR1;
    m_dashedLten = true;
}


bool PalmiraRenderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (Rk86Renderer::setProperty(propertyName, values))
        return true;

    if (propertyName == "fontRam") {
        attachFontRam(static_cast<Ram*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


string PalmiraRenderer::getPropertyStringValue(const string& propertyName)
{
    if (propertyName == "altRenderer") {
        return "";
    }

    return Rk86Renderer::getPropertyStringValue(propertyName);
}


bool PalmiraConfigRegister::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "crtRenderer") {
        m_renderer = static_cast<PalmiraRenderer*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "lowerMemMapper") {
        m_lowerMemMapper = static_cast<AddrSpaceMapper*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "upperMemMapper") {
        m_upperMemMapper = static_cast<AddrSpaceMapper*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "fontMemMapper") {
        m_fontMemMapper = static_cast<AddrSpaceMapper*>(g_emulation->findObject(values[0].asString()));
        return true;
    }
    return false;
}


void PalmiraConfigRegister::writeByte(int, uint8_t value)
{
    m_renderer->setExtFontMode(value & 0x40);
    m_fontMemMapper->setCurPage(value & 0x40 ? 1 : 0);
    m_renderer->setFontSetNum((value & 0x0E) >> 1);

    m_lowerMemMapper->setCurPage(value & 1);
    m_upperMemMapper->setCurPage(value & 0x80 ? 0 : 1);
}
