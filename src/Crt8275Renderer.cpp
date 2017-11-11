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

// Crt8275Renderer.cpp

#include <string.h>

#include "Crt8275Renderer.h"
#include "Emulation.h"
#include "Crt8275.h"

using namespace std;


void Crt8275Renderer::attachCrt(Crt8275* crt)
{
    m_crt = crt;
}


bool Crt8275Renderer::isRasterPresent()
{
    return m_crt->getRasterPresent();
}


void Crt8275Renderer::primaryRenderFrame()
{
    double frameRate = m_crt->getFrameRate();
    double freqMHz = g_emulation->getFrequency() / m_crt->getKDiv() / 1000000.0;
    if (frameRate == 0.0)
        m_aspectRatio = 1.0;
    else if (frameRate < 55.0) {
        // PAL
        m_aspectRatio = 288.0 * 4 / 52 / 3 / m_fntCharWidth / freqMHz;
    } else {
        // NTSC
        m_aspectRatio = 240.0 * 4 / 52 / 3 / m_fntCharWidth / freqMHz;
    }

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

                if (!m_customDraw) {
                    uint8_t fntLine = fntPtr[symbol.chr * m_fntCharHeight + (lc & m_fntLcMask)] << (8 - m_fntCharWidth);

                    for (int pt = 0; pt < m_fntCharWidth; pt++) {
                        //bool v = lten || !vsp && (fntLine & 0x80);
                        bool v = lten || !(vsp || (fntLine & 0x80));
                        if (rvv && m_useRvv)
                            v = !v;
                        linePtr[pt] = v ? fgColor : bgColor;
                        fntLine <<= 1;
                    }
                } else
                    customDrawSymbolLine(linePtr, symbol.chr, lc, lten, vsp, rvv, gpa0, gpa1, hglt);
                linePtr += nChars * m_fntCharWidth;
            }
            chrPtr += m_fntCharWidth;
        }
    rowPtr += nLines * nChars * m_fntCharWidth;
    }

}


void Crt8275Renderer::altRenderFrame()
{
    m_aspectRatio = 1.0;

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
}


bool Crt8275Renderer::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (TextCrtRenderer::setProperty(propertyName, values))
        return true;

    if (propertyName == "crt") {
        attachCrt(static_cast<Crt8275*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}
