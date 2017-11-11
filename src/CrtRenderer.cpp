/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2017
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

#include "Crt8275Renderer.h"

using namespace std;

CrtRenderer::~CrtRenderer()
{
    if (m_pixelData)
        delete[] m_pixelData;
    if (m_prevPixelData)
        delete[] m_prevPixelData;
}


EmuPixelData CrtRenderer::getPixelData()
{
    EmuPixelData pd;

    // Проверка "синего экрана"
    if (!isRasterPresent()) {
        pd.pixelData = nullptr;
        pd.prevPixelData = nullptr;
        return pd;
    }


    pd.width = m_sizeX;
    pd.height = m_sizeY;
    pd.pixelData = m_pixelData;
    pd.aspectRatio = m_aspectRatio;

    pd.prevWidth = m_prevSizeX;
    pd.prevHeight = m_prevSizeY;
    pd.prevPixelData = m_prevPixelData;
    pd.prevAspectRatio = m_prevAspectRatio;

    pd.frameNo = m_frameNo;

    return pd;
}


void CrtRenderer::swapBuffers()
{
    int w,h, bs;
    uint32_t* buf;

    w = m_prevSizeX;
    h = m_prevSizeY;
    buf = m_prevPixelData;
    bs = m_prevBufSize;

    m_prevSizeX = m_sizeX;
    m_prevSizeY = m_sizeY;
    m_prevPixelData = m_pixelData;
    m_prevBufSize = m_bufSize;

    m_sizeX = w;
    m_sizeY = h;
    m_pixelData = buf;
    m_bufSize = bs;

    ++m_frameNo;
}



TextCrtRenderer::~TextCrtRenderer()
{
    if (m_font)
        delete[] m_font;
    if (m_altFont)
        delete[] m_altFont;
}


void TextCrtRenderer::setFontFile(string fontFileName)
{
    m_font = palReadFile(fontFileName, m_fontSize);
}


void TextCrtRenderer::setAltFontFile(string fontFileName)
{
    m_altFont = palReadFile(fontFileName, m_altFontSize);
}


void TextCrtRenderer::setAltRender(bool isAltRender)
{
    m_isAltRender = isAltRender;
}


void TextCrtRenderer::toggleRenderingMethod()
{
    setAltRender(!m_isAltRender);
}


void TextCrtRenderer::renderFrame()
{
    swapBuffers();

    if (m_isAltRender)
        altRenderFrame();
    else
        primaryRenderFrame();
}


bool TextCrtRenderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (CrtRenderer::setProperty(propertyName, values))
        return true;

    if (propertyName == "font") {
        setFontFile(values[0].asString());
        return true;
    } else if (propertyName == "altFont") {
        setAltFontFile(values[0].asString());
        return true;
    } else if (propertyName == "altRenderer") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            setAltRender(values[0].asString() == "yes");
            return true;
        }
    }

    return false;
}


string TextCrtRenderer::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "altRenderer") {
        return m_isAltRender ? "yes" : "no";
    }

    return "";
}
