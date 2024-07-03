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

// Crt8275Renderer.cpp

#include <sstream>
#include <cstring>

#include "Globals.h"
#include "Crt8275Renderer.h"
#include "Emulation.h"
#include "Crt8275.h"

using namespace std;


Crt8275Renderer::Crt8275Renderer()
{
    m_useAltFont = true;
}


void Crt8275Renderer::attachCrt(Crt8275* crt)
{
    m_crt = crt;
}


bool Crt8275Renderer::isRasterPresent()
{
    return m_crt->getRasterPresent();
}


void Crt8275Renderer::setCropping(bool cropping)
{
    m_cropping = cropping;
    if (m_secondaryRenderer)
        m_secondaryRenderer->setCropping(cropping);
}

void Crt8275Renderer::toggleCropping()
{
    setCropping(!m_cropping);
}

void Crt8275Renderer::calcAspectRatio(int charWidth)
{
    m_frameRate = m_crt->getFrameRate();
    m_freqMHz = g_emulation->getFrequency() / 1000000.0 / m_crt->getKDiv();

    int scanLines = m_crt->getNLines() * (m_crt->getNRows() + m_crt->getVrRows());

    if (m_frameRate == 0.0)
        m_aspectRatio = 1.0;
    else if (scanLines >= 475) {
        // VGA (480p)
        m_aspectRatio = 25.175 / m_fntCharWidth / m_freqMHz; // 480.0 * 25.175 * 4 / 640 / m_fntCharWidth / m_freqMHz / 3;
    } else if (scanLines >= 360) {
        // VGA400
        m_aspectRatio = 25.175 * 5 / 6 / m_fntCharWidth / m_freqMHz; // 400.0 * 25.175 * 4 / 640 / m_fntCharWidth / m_freqMHz / 3;
    } else if (m_frameRate < 55.0) {
        // 625i (PAL)
        m_aspectRatio = 576.0 * 9 / 704 / charWidth / m_freqMHz; // 576 * 13.5 * 4 / 704 / m_fntCharWidth / freqMHz / 3 / 2
    } else if (scanLines < 360) {
        // 480i (NTSC)
        m_aspectRatio = 480.0 * 9 / 704 / charWidth / m_freqMHz; // 480 * 13.5 * 4 / 704 / m_fntCharWidth / freqMHz / 3 / 2
    }
}


void Crt8275Renderer::trimImage(int charWidth, int charHeight)
{
    if (!m_cropping)
        return;

    if (m_sizeX < 100 || m_sizeY < 100)
        return;

    int visibleX, visibleY, visibleWidth, visibleHeight;

    //double effectiveFreq = m_freqMHz * m_freqMHz * 64 / (m_crt->getNCharsPerRow() + m_crt->getHrChars());

    int scanLines = m_crt->getNLines() * (m_crt->getNRows() + m_crt->getVrRows());

    if (m_frameRate == 0.0)
        return;
    else if (scanLines >= 475) {
        // 480p (VGA)
        // don't PLL horiz. freq for VGA
        //double effectiveFreq = (m_crt->getNCharsPerRow() + m_crt->getHrChars()) / (800 / 25.175);
        double effectiveFreq = g_emulation->getFrequency() / 1000000.0 / m_crt->getKDiv();
        visibleX = (144 * effectiveFreq / 25.175 - m_crt->getHrChars() - 1) * charWidth + m_visibleOffsetX;
        //visibleY = 19 - charHeight * m_crt->getVrRows();
        visibleY = (35 * charHeight / m_crt->getNLines()) - charHeight * m_crt->getVrRows();
        visibleWidth = (640 * effectiveFreq * charWidth) / 25.175 + 0.5;
        visibleHeight = 480 * charHeight / m_crt->getNLines() + 0.5;
    } else if (scanLines >= 360) {
        // VGA400
        // don't PLL horiz. freq for VGA
        double effectiveFreq = g_emulation->getFrequency() / 1000000.0 / m_crt->getKDiv();
        visibleX = (144 * effectiveFreq / 25.175 - m_crt->getHrChars() - 1) * charWidth + m_visibleOffsetX;
        visibleY = (33 * charHeight / m_crt->getNLines()) - charHeight * m_crt->getVrRows();
        visibleWidth = (640 * effectiveFreq * charWidth) / 25.175 + 0.5;
        visibleHeight = 400 * charHeight / m_crt->getNLines() + 0.5;
    } else if (m_frameRate < 55.0) {
        // 576i (PAL)
        double effectiveFreq = (m_crt->getNCharsPerRow() + m_crt->getHrChars()) / 64.;
        visibleX = (140 * effectiveFreq / 13.5 - m_crt->getHrChars() - 1) * charWidth + m_visibleOffsetX; //31 //48;
        //visibleY = 23 - charHeight * m_crt->getVrRows(); //30;
        visibleY = (23 * charHeight / m_crt->getNLines()) - charHeight * m_crt->getVrRows();
        visibleWidth = (704 * effectiveFreq * charWidth) / 13.5; // 384;
        visibleHeight = 288 * charHeight / m_crt->getNLines(); //288; //250;
    } else {
        //480i (NTSC)
        double effectiveFreq = (m_crt->getNCharsPerRow() + m_crt->getHrChars()) / 63.556; //(572. / 9. /*63.(5)*/);
        visibleX = (130 * effectiveFreq / 13.5 - m_crt->getHrChars() - 1) * charWidth + m_visibleOffsetX;
        //visibleY = 19 - charHeight * m_crt->getVrRows();
        visibleY = (19 * charHeight / m_crt->getNLines()) - charHeight * m_crt->getVrRows();
        visibleWidth = (704 * effectiveFreq * charWidth) / 13.5;
        visibleHeight = 240 * charHeight / m_crt->getNLines();
    }

    int visibleDataSize = visibleWidth * visibleHeight;
    uint32_t* visibleData = new uint32_t[visibleDataSize];

    for (int i = 0; i < visibleDataSize; i++)
        visibleData[i] = 0;

    int dstX = 0;
    int dstY = 0;
    int copyWidth = visibleWidth;
    int copyHeight = visibleHeight;

    if (visibleX < 0) {
        dstX = -visibleX;
        copyWidth += visibleX;
        visibleX = 0;
    }

    if (visibleX + copyWidth > m_sizeX)
        copyWidth = m_sizeX - visibleX;

    if (visibleY < 0) {
        dstY = -visibleY;
        copyHeight += visibleY;
        visibleY = 0;
    }

    if (visibleY + copyHeight > m_sizeY)
        copyHeight = m_sizeY - visibleY;

    for (int i = 0; i < copyHeight; i++)
        memcpy(visibleData + (i + dstY) * visibleWidth + dstX, m_pixelData + (visibleY + i) * m_sizeX + visibleX, copyWidth * sizeof(uint32_t));

    delete[] m_pixelData;
    m_pixelData = visibleData;
    m_sizeX = visibleWidth;
    m_sizeY = visibleHeight;
    m_dataSize = visibleWidth * visibleHeight;
    m_bufSize = m_dataSize;
    m_aspectRatio = double(m_sizeY) * 4 / 3 / m_sizeX;

    m_cropX = visibleX;
    m_cropY = visibleY;
}


void Crt8275Renderer::mouseDrag(int x, int y)
{
    if (m_cropping) {
        x += m_cropX;
        y += m_cropY;
    }

    x /= m_fntCharWidth;
    y /= m_crt->getFrame()->nLines;

    m_crt->setLpenPosition(x, y);
}


void Crt8275Renderer::primaryRenderFrame()
{
    calcAspectRatio(m_fntCharWidth);

    const Frame* frame = m_crt->getFrame();

    int nRows = frame->nRows;
    int nLines = frame->nLines;
    int nChars = frame->nCharsPerRow;

    m_sizeX = nChars * m_fntCharWidth;
    m_sizeY = nRows * nLines;

    m_dataSize = nRows * nLines * nChars * m_fntCharWidth;
    if (m_dataSize > m_bufSize) {
        if (m_pixelData)
            delete[] m_pixelData;
        m_pixelData = new uint32_t [m_dataSize];
        m_bufSize = m_dataSize;
    }

    memset(m_pixelData, 0, m_dataSize * sizeof(uint32_t));
    uint32_t* rowPtr = m_pixelData;

    for (int row = 0; row < nRows; row++) {
        uint32_t* chrPtr = rowPtr;
        bool curLten[16];
        memset(curLten, 0, sizeof(curLten));
        for (int chr = 0; chr < nChars; chr++) {
            Symbol symbol = frame->symbols[row][chr];
            uint32_t* linePtr = chrPtr;

            bool hglt;
            if (!m_hgltOffset || (chr == nChars - 1))
                hglt = symbol.symbolAttributes.hglt;
            else
                hglt = frame->symbols[row][chr+1].symbolAttributes.hglt;

            bool gpa0, gpa1;
            if (!m_gpaOffset || (chr == nChars - 1)) {
                gpa0 = symbol.symbolAttributes.gpa0;
                gpa1 = symbol.symbolAttributes.gpa1;
            }
            else {
                gpa0 = frame->symbols[row][chr+1].symbolAttributes.gpa0;
                gpa1 = frame->symbols[row][chr+1].symbolAttributes.gpa1;
            }

            bool rvv;
            if (!m_rvvOffset || (chr == nChars - 1))
                rvv = symbol.symbolAttributes.rvv;
            else
                rvv = frame->symbols[row][chr+1].symbolAttributes.rvv;

            const uint8_t* fntPtr = getCurFontPtr(gpa0, gpa1, hglt);
            uint32_t fgColor = getCurFgColor(gpa0, gpa1, hglt);
            uint32_t bgColor = getCurBgColor(gpa0, gpa1, hglt);

            for (int ln = 0; ln < nLines; ln++) {
                int lc;
                if (!frame->isOffsetLineMode)
                    lc = ln;
                else
                    lc = ln != 0 ? ln - 1 : nLines - 1;

                bool vsp = symbol.symbolLineAttributes[ln].vsp;

                bool lten;
                if (!m_ltenOffset || (chr == nChars - 1))
                    lten = symbol.symbolLineAttributes[ln].lten;
                else
                    lten = frame->symbols[row][chr+1].symbolLineAttributes[ln].lten;

                curLten[ln] = m_dashedLten ? lten && !curLten[ln] : lten;

                if (!m_customDraw) {
                    uint16_t fntLine = fntPtr[symbol.chr * m_fntCharHeight + (lc & m_fntLcMask)] << (8 - m_fntCharWidth);

                    for (int pt = 0; pt < m_fntCharWidth; pt++) {
                        bool v = curLten[ln] || !(vsp || (fntLine & 0x80));
                        if (rvv && m_useRvv)
                            v = !v;
                        linePtr[pt] = v ? fgColor : bgColor;
                        fntLine <<= 1;
                    }
                    if (m_dashedLten && (curLten[ln] || !(vsp || (fntLine & 0x400))))
                        curLten[ln] = true;
                } else
                    customDrawSymbolLine(linePtr, symbol.chr, lc, lten, vsp, rvv, gpa0, gpa1, hglt);
                linePtr += nChars * m_fntCharWidth;
            }
            chrPtr += m_fntCharWidth;
        }
    rowPtr += nLines * nChars * m_fntCharWidth;
    }

    trimImage(m_fntCharWidth, nLines);
}


void Crt8275Renderer::altRenderFrame()
{
    calcAspectRatio(8);

    const Frame* frame = m_crt->getFrame();

    int nRows = frame->nRows;
    int nChars = frame->nCharsPerRow;

    int nLines;
    if (nRows <= 32)
        nLines = 16;
    else if (nRows <= 42)
        nLines = 12;
    else
        nLines = 8;

    m_aspectRatio = m_aspectRatio * nLines / frame->nLines;

    m_sizeX = nChars * 8;
    m_sizeY = nRows * nLines;

    m_dataSize = nRows * nLines * nChars * 8;
    if (m_dataSize > m_bufSize) {
        if (m_pixelData)
            delete[] m_pixelData;
        m_pixelData = new uint32_t [m_dataSize];
        m_bufSize = m_dataSize;
    }

    memset(m_pixelData, 0, m_dataSize * sizeof(uint32_t));
    uint32_t* rowPtr = m_pixelData;

    for (int row = 0; row < nRows; row++) {
        uint32_t* chrPtr = rowPtr;
        for (int chr = 0; chr < nChars; chr++) {
            Symbol symbol = frame->symbols[row][chr];
            uint32_t* linePtr = chrPtr;

            bool hglt;
            if (!m_hgltOffset || (chr == nChars - 1))
                hglt = symbol.symbolAttributes.hglt;
            else
                hglt = frame->symbols[row][chr+1].symbolAttributes.hglt;

            bool gpa0, gpa1;
            if (!m_gpaOffset || (chr == nChars - 1)) {
                gpa0 = symbol.symbolAttributes.gpa0;
                gpa1 = symbol.symbolAttributes.gpa1;
            }
            else {
                gpa0 = frame->symbols[row][chr+1].symbolAttributes.gpa0;
                gpa1 = frame->symbols[row][chr+1].symbolAttributes.gpa1;
            }

            bool rvv;
            if (!m_rvvOffset || (chr == nChars - 1))
                rvv = symbol.symbolAttributes.rvv;
            else
                rvv = frame->symbols[row][chr+1].symbolAttributes.rvv;

            bool vsp = symbol.symbolLineAttributes[1].vsp; // !!!

            const uint8_t* fntPtr = getAltFontPtr(gpa0, gpa1, hglt);
            if (nLines == 12)
                fntPtr += 8;
            else if (nLines == 16)
                fntPtr += (8+12);
            uint32_t fgColor = getCurFgColor(gpa0, gpa1, hglt);
            uint32_t bgColor = getCurBgColor(gpa0, gpa1, hglt);


            for (int ln = 0; ln < nLines; ln++) {
                uint8_t fntLine = fntPtr[symbol.chr * (8+12+16) + ln];


                bool lten = false;
                //bool rvv2 = rvv;

                if (frame->cursorUnderline && frame->cursorRow == row && frame->cursorPos + (m_ltenOffset ? -1 : 0) == chr) {
                    if (ln >= nLines - 2 && (!frame->cursorBlinking || frame->frameCount % 20 < 12 ))
                        lten = true;
                } //else if (!frame->cursorUnderline && frame->cursorRow == row && frame->cursorPos + (m_rvvOffset ? -1 : 0) == chr) {
                    //if (!frame->cursorBlinking || (frame->frameCount & 0x10) )
                        //rvv2 = !rvv2;
                //}

                for (int pt = 0; pt < 8; pt++) {
                    bool v = lten || (!vsp && (fntLine & 0x80));
                    if (rvv && m_useRvv)
                        v = !v;
                    linePtr[pt] = v ? fgColor : bgColor;
                    fntLine <<= 1;
                }
                linePtr += nChars * 8;
            }
            chrPtr += 8;
        }
    rowPtr += nLines * nChars * 8;
    }

    trimImage(8, nLines);
}


void Crt8275Renderer::prepareDebugScreen()
{
    // check if frame geometry was changed
    if (m_crt->getFrame()->nCharsPerRow != m_crt->getNCharsPerRow() ||
        m_crt->getFrame()->nRows != m_crt->getNRows() ||
        m_crt->getFrame()->nLines != m_crt->getNLines() ||
        m_crt->getFrame()->nHrChars != m_crt->getHrChars() ||
        m_crt->getFrame()->nVrRows != m_crt->getVrRows())
        return;

    enableSwapBuffersOnce();
    renderFrame();
}


wchar_t Crt8275Renderer::getUnicodeSymbol(uint8_t, bool, bool, bool)
{
    return 0;
}


const char* Crt8275Renderer::getTextScreen()
{
    const Frame* frame = m_crt->getFrame();

    int h = frame->nRows;
    int w = frame->nCharsPerRow;

    wchar_t* wTextArray = new wchar_t[h * w];

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Symbol symbol = frame->symbols[y][x];
            uint8_t chr = symbol.chr;
            bool gpa0 = symbol.symbolAttributes.gpa0;
            bool gpa1 = symbol.symbolAttributes.gpa1;
            bool hglt = symbol.symbolAttributes.hglt;
            bool vsp = symbol.symbolLineAttributes[1].vsp;

            wchar_t wchr = getUnicodeSymbol(chr, gpa0, gpa1, hglt);

            wTextArray[y * w + x] = wchr && !vsp ? wchr : u' ';
        }
    }

    return generateTextScreen(wTextArray, w, h);
}


bool Crt8275Renderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (TextCrtRenderer::setProperty(propertyName, values))
        return true;

    if (propertyName == "crt") {
        attachCrt(static_cast<Crt8275*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "visibleArea") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            setCropping(values[0].asString() == "yes");
            return true;
        }
    } else if (propertyName == "visibleOffsetX") {
        m_visibleOffsetX = values[0].asInt();
        return true;
    }

    return false;
}


string Crt8275Renderer::getCrtMode()
{
    const Frame* frame = m_crt->getFrame();
    stringstream ss;
    if (m_crt->getRasterPresent()) {
        ss << frame->nCharsPerRow << u8"\u00D7" << frame->nRows << u8"\u00D7" << frame->nLines << "@";
        ss.precision(2);
        ss << fixed << m_crt->getFrameRate() << "Hz";
    } else
        ss << "No sync";

    return ss.str();
}


string Crt8275Renderer::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = TextCrtRenderer::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "crtMode") {
        return getCrtMode();
    } else if (propertyName == "visibleArea") {
        return m_cropping ? "yes" : "no";
    }

    return "";
}
