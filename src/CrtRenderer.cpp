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

#include <locale>
#include <codecvt>

#include "Pal.h"
#include "Globals.h"
#include "Emulation.h"
#include "CrtRenderer.h"

using namespace std;

CrtRenderer::~CrtRenderer()
{
    if (m_pixelData)
        delete[] m_pixelData;
    if (m_prevPixelData)
        delete[] m_prevPixelData;
}


void CrtRenderer::attachSecondaryRenderer(CrtRenderer* renderer)
{
    m_secondaryRenderer = renderer;
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
    // don't update if in debug or paused mode
    if (!reqForSwapBuffers && (g_emulation->getPausedState() || g_emulation->isDebuggerActive()))
        return;
    reqForSwapBuffers = false;

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
    m_prevAspectRatio = m_aspectRatio;

    m_sizeX = w;
    m_sizeY = h;
    m_pixelData = buf;
    m_bufSize = bs;

    ++m_frameNo;
}


void CrtRenderer::updateScreenOnce()
{
    if (!g_emulation->getPausedState() && !g_emulation->isDebuggerActive())
        return;

       ++m_frameNo;
}


void CrtRenderer::prepareDebugScreen()
{
    if (m_defaultDebugRendering) {
        enableSwapBuffersOnce();
        renderFrame();
    }
}


const char* CrtRenderer::generateTextScreen(wchar_t* wTextArray, int w, int h)
{
    // calculate row lengths for every row without trailing spaces
    int* rowLengths = new int[h];
    for (int y = 0; y < h; y++) {
        int x;
        for (x = w - 1; x >= 0; x--)
            if (wTextArray[y * w + x] != u' ')
                break;
        rowLengths[y] = x + 1;
    }

    // Calculate first and last non-empty rows
    int y;
    for (y = 0; y < h; y++)
        if (rowLengths[y] > 0)
            break;
    int firstRow = y;
    for (y = h - 1; y >= 0; y--)
        if (rowLengths[y] > 0)
            break;
    int lastRow = y;
    if(firstRow > lastRow)
        firstRow = lastRow = 0;


    // Calculate left offset
    int firstPos = w;
    for (int y = firstRow; y <= lastRow; y++)
        for (int x = 0; x < rowLengths[y]; x++)
            if (wTextArray[y * w + x] != u' ') {
                if (firstPos > x)
                    firstPos = x;
                break;
            }
    if (firstPos >= w)
        firstPos = 0;

    wstring wTextScreen;
    for (int y = firstRow; y <= lastRow; y++) {
        for (int x = firstPos; x < rowLengths[y]; x++) {
            wchar_t wchr = wTextArray[y * w + x];
            wTextScreen.append(1, wchr);
        }
        wTextScreen.append(L"\n");
    }

    delete[] wTextArray;
    delete[] rowLengths;

    std::wstring_convert<std::codecvt_utf8<wchar_t>> conversion;
    m_textScreen = conversion.to_bytes(wTextScreen);

    return m_textScreen.c_str();
}


bool CrtRenderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "secondaryRenderer") {
        attachSecondaryRenderer(static_cast<CrtRenderer*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
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
    if (m_useAltFont)
        m_isAltRender = isAltRender;
}


void TextCrtRenderer::toggleRenderingMethod()
{
    if (m_useAltFont)
        setAltRender(!m_isAltRender);
}


void TextCrtRenderer::renderFrame()
{
    swapBuffers();

    if (m_useAltFont && m_isAltRender && m_altFont)
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
