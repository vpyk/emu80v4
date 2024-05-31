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

// Crt8275.cpp

// Реализация контроллера CRT КР580ВГ75

#include <sstream>
#include <stdint.h>
#include <stdlib.h>

#include "Globals.h"
#include "Crt8275.h"
#include "Cpu.h"
#include "Dma8257.h"
#include "Platform.h"
#include "PlatformCore.h"
#include "Emulation.h"

using namespace std;

////////////////////////////
// Crt8275 Implementation //
////////////////////////////


// Common Methods Implementation

Crt8275::Crt8275()
{
    m_raster = new Crt8275Raster;
    m_raster->m_crt = this;
}


Crt8275::~Crt8275()
{
    delete m_raster;
}


void Crt8275::setFrequency(int64_t freq)
{
    EmuObject::setFrequency(freq);
    m_raster->setFrequency(freq);
}


void Crt8275::attachCore(PlatformCore* core)
{
    m_core = core;
    m_raster->m_core = core;
}


void Crt8275::init()
{
    m_cpuKDiv = m_platform->getCpu()->getKDiv();
}


void Crt8275::reset()
{
    m_isPaused = true;
    m_curClock = -1;
    m_crtCmd = CC_LOADCURSOR;
    m_nRows = 1;
    m_nLines = 1;
    m_isSpacedRows = false;
    m_nCharsPerRow = 1;
    m_undLine = 0;
    m_isOffsetLine = false;
    m_isTransparentAttr = false;
    //m_cursorFormat = CF_BLINKINGBLOCK;
    m_cursorBlinking = true;
    m_cursorUnderline = false;
    m_nVrRows = 1;
    m_nHrChars = 2;
    m_burstSpaceCount = 0;
    m_burstCount = 1;
    m_cursorPos = 0;
    m_cursorRow = 0;
    m_isIntsEnabled = false;
    m_statusReg = 0;
    m_cmdReg = 0;
    m_isCompleteCommand = true;
    m_wasDmaUnderrun = false;
    m_isRasterStarted = false;
    m_wasVsync = false;
    for (int i=0; i<4; i++)
        m_resetParam[i] = 0;

    m_raster->pause();
}



void Crt8275::attachDMA(Dma8257* dma, int channel)
{
    m_dma = dma;
    m_dmaChannel = channel;
}



// Buffer Display Related Methods

void Crt8275::prepareFrame()
{
    m_curUnderline = false;
    m_curReverse = false;
    m_curBlink = false;
    m_curHighlight = false;
    m_curGpa1 = false;
    m_curGpa0 = false;
    m_isBlankedToTheEndOfScreen = false;

    m_frameCount++;

    // alt renderer fields
    m_frame.cursorRow = m_cursorRow;
    m_frame.cursorPos = m_cursorPos;
    m_frame.frameCount = m_frameCount;
    m_frame.cursorBlinking = m_cursorBlinking;
    m_frame.cursorUnderline = m_cursorUnderline;
}


void Crt8275::displayBuffer()
{
    m_frame.nRows = m_nRows;
    m_frame.nLines = m_nLines;
    m_frame.nCharsPerRow = m_nCharsPerRow;
    m_frame.isOffsetLineMode = m_isOffsetLine;

    // ffame format check fields
    m_frame.nHrChars = m_nHrChars;
    m_frame.nVrRows = m_nVrRows;

    bool isBlankedToTheEndOfRow = false;

    int fifoPos = 0;

    for (int i = 0; i < m_nCharsPerRow; i++) {
        uint8_t chr = m_rowBuf[i % 80];

        if (m_wasDmaUnderrun || isBlankedToTheEndOfRow || m_isBlankedToTheEndOfScreen || !m_isDisplayStarted)
            chr = 0;

        if (m_isTransparentAttr && ((chr & 0xC0) == 0x80)) {
            // Transparent Field Attribute Code
            m_curUnderline = chr & 0x20;
            m_curReverse = chr & 0x10;
            m_curBlink = chr & 0x02;
            m_curHighlight = chr & 0x01;
            m_curGpa0 = chr & 0x04;
            m_curGpa1 = chr & 0x08;

            chr = m_fifo[fifoPos++];
            fifoPos &= 0x0f;
        }

        m_frame.symbols[m_curRow][i].chr = chr & 0x7F;

//         if (m_isDmaStoppedForRow || m_isDmaStoppedForFrame) {
//             m_frame.symbols[m_curRow][i].chr = 0x41;
//             for (int j = 0; j < m_nLines; j++) {
//                 m_frame.symbols[m_curRow][i].symbolLineAttributes[j].vsp  = false;
//                 m_frame.symbols[m_curRow][i].symbolLineAttributes[j].lten = true;
//             }
//         } else

        if (!m_isDisplayStarted || isBlankedToTheEndOfRow || m_isBlankedToTheEndOfScreen || m_wasDmaUnderrun) {
            m_frame.symbols[m_curRow][i].symbolAttributes.rvv  = false;
            m_frame.symbols[m_curRow][i].symbolAttributes.hglt = false; // ?
            m_frame.symbols[m_curRow][i].symbolAttributes.gpa0 = false; // ?
            m_frame.symbols[m_curRow][i].symbolAttributes.gpa1 = false; // ?
            for (int j = 0; j < m_nLines; j++) {
                m_frame.symbols[m_curRow][i].symbolLineAttributes[j].vsp  = true;
                m_frame.symbols[m_curRow][i].symbolLineAttributes[j].lten = false;
            }
        } else if (chr < 0x80) {
            // Ordinary symbol
            m_frame.symbols[m_curRow][i].symbolAttributes.rvv  = m_curReverse;
            m_frame.symbols[m_curRow][i].symbolAttributes.hglt = m_curHighlight;
            m_frame.symbols[m_curRow][i].symbolAttributes.gpa0 = m_curGpa0;
            m_frame.symbols[m_curRow][i].symbolAttributes.gpa1 = m_curGpa1;
            for (int j = 0; j < m_nLines; j++) {
                m_frame.symbols[m_curRow][i].symbolLineAttributes[j].vsp  = m_curBlink && (m_frameCount & 0x10);
                m_frame.symbols[m_curRow][i].symbolLineAttributes[j].lten = false;

                if ((m_undLine > 7) && ((j == 0) || (j == m_nLines - 1)))
                    m_frame.symbols[m_curRow][i].symbolLineAttributes[j].vsp = true;
            }
            if (m_curUnderline) {
                m_frame.symbols[m_curRow][i].symbolLineAttributes[m_undLine].lten = true;
                if (m_curBlink)
                    m_frame.symbols[m_curRow][i].symbolLineAttributes[m_undLine].lten = !(m_frameCount & 0x10);
                //_frame.symbols[m_curRow][i].symbolLineAttributes[m_undLine].vsp = false;
            }
        } else if ((chr & 0xC0) == 0x80) {
            // Field Attribute Code
            m_curUnderline = chr & 0x20;
            m_curReverse = chr & 0x10;
            m_curBlink = chr & 0x02;
            m_curHighlight = chr & 0x01;
            m_curGpa0 = chr & 0x04;
            m_curGpa1 = chr & 0x08;

            for (int j = 0; j < m_nLines; j++) {
                m_frame.symbols[m_curRow][i].symbolLineAttributes[j].vsp  = true;
                m_frame.symbols[m_curRow][i].symbolLineAttributes[j].lten = false;
            }
            m_frame.symbols[m_curRow][i].symbolAttributes.rvv  = false;
            m_frame.symbols[m_curRow][i].symbolAttributes.hglt = m_curHighlight; // ?
            m_frame.symbols[m_curRow][i].symbolAttributes.gpa0 = m_curGpa0; //?? уточнить!!!
            m_frame.symbols[m_curRow][i].symbolAttributes.gpa1 = m_curGpa1; //?? уточнить!!!

        } else if ((chr & 0xC0) == 0xC0 && (chr & 0x30) != 0x30) {
            // Character Attribute
            int cccc = (chr & 0x3C) >> 2;

            for (int j = 0; j < m_nLines; j++) {
                if (j < m_undLine) {
                    m_frame.symbols[m_curRow][i].symbolLineAttributes[j].vsp = m_cCharAttrVsp[cccc][0] || ((chr & 0x02) && (m_frameCount & 0x10));
                    m_frame.symbols[m_curRow][i].symbolLineAttributes[j].lten = 0;//cCharAttr[cccc][1][0];
                } else if (j > m_undLine) {
                    m_frame.symbols[m_curRow][i].symbolLineAttributes[j].vsp = m_cCharAttrVsp[cccc][1] || ((chr & 0x02) && (m_frameCount & 0x10));
                    m_frame.symbols[m_curRow][i].symbolLineAttributes[j].lten = 0;//cCharAttr[cccc][1][2];
                } else {// j == _undLine
                    m_frame.symbols[m_curRow][i].symbolLineAttributes[j].vsp = (chr & 0x02) && (m_frameCount & 0x10);
                    m_frame.symbols[m_curRow][i].symbolLineAttributes[j].lten = m_cCharAttrLten[cccc] && !((chr & 0x02) && (m_frameCount & 0x10));
                }
            }

            m_frame.symbols[m_curRow][i].symbolAttributes.hglt = chr & 0x01;
//            _frame.symbols[m_curRow][i].symbolAttributes.gpa0 = chr & 0x04;
//            _frame.symbols[m_curRow][i].symbolAttributes.gpa1 = chr & 0x08;

            m_frame.symbols[m_curRow][i].symbolAttributes.rvv  = m_curReverse;
            m_frame.symbols[m_curRow][i].symbolAttributes.gpa0 = m_curGpa0;
            m_frame.symbols[m_curRow][i].symbolAttributes.gpa1 = m_curGpa1;

        } else {
            // Special Control Characters
            if (chr & 0x02) {
                // End of Screen (stop or not stop DMA)
                m_isBlankedToTheEndOfScreen = true;
            } else {
                // End of Row (stop or not stop DMA)
                isBlankedToTheEndOfRow = true;
            }
            m_frame.symbols[m_curRow][i].symbolAttributes.rvv  = m_curReverse;
            m_frame.symbols[m_curRow][i].symbolAttributes.hglt = m_curHighlight;
            m_frame.symbols[m_curRow][i].symbolAttributes.gpa0 = m_curGpa0;
            m_frame.symbols[m_curRow][i].symbolAttributes.gpa1 = m_curGpa1;
            for (int j = 0; j < m_nLines; j++) {
                m_frame.symbols[m_curRow][i].symbolLineAttributes[j].vsp  = true;
                m_frame.symbols[m_curRow][i].symbolLineAttributes[j].lten = false;
            }
            if (m_curUnderline)
                m_frame.symbols[m_curRow][i].symbolLineAttributes[m_undLine].lten = true;
        }
    }
    if (m_isDisplayStarted && (m_curRow == m_cursorRow) && (m_cursorPos < 80)) {
        if (m_cursorUnderline) {
            m_frame.symbols[m_cursorRow][m_cursorPos].symbolLineAttributes[m_undLine].lten = !m_cursorBlinking || (m_frameCount & 0x08);
        } else {
            if (!m_cursorBlinking || (m_frameCount & 0x08))
                m_frame.symbols[m_cursorRow][m_cursorPos].symbolAttributes.rvv = !(m_frame.symbols[m_cursorRow][m_cursorPos].symbolAttributes.rvv);
        }
    }
}



// AddressableDevice Methods Implemantation

void Crt8275::writeByte(int addr, uint8_t value)
{
    addr &= 1;
    switch (addr) {
        case 0:
            // Writing Parameter Register
            switch (m_crtCmd) {
                case CC_RESET:
                    switch (m_parameterNum++) {
                        case 0:
                            // SHHHHHHH
                            m_isSpacedRows = (value & 0x80) != 0;
                            m_nCharsPerRow = (value & 0x7f) + 1;
                            m_isCompleteCommand = false;
                            m_statusReg |= 0x08; // reset IC flag
                            break;
                        case 1:
                            // VVRRRRRR
                            m_nVrRows = ((value & 0xc0) >> 6) + 1;
                            m_nRows = (value & 0x3f) + 1;
                            break;
                        case 2:
                            // UUUULLLL
                            m_undLine = ((value & 0xf0) >> 4);
                            m_nLines = (value & 0xf) + 1;
                            break;
                        case 3:
                            // MFCCZZZZ
                            m_isOffsetLine = (value & 0x80) != 0;
                            m_isTransparentAttr = (value & 0x40) == 0;
                            m_cursorBlinking = !(value & 0x20);
                            m_cursorUnderline = value & 0x10;
                             //m_cursorBlinking = !m_cursorBlinking;
                             //m_cursorUnderline = !m_cursorUnderline;
                            //m_cursorFormat = static_cast<CursorFormat>((value & 0x30) >> 4);
                            m_nHrChars = ((value & 0x0f) + 1) * 2;
                            m_isCompleteCommand = true;
                            m_statusReg &= ~0x08; // reset IC flag
                            m_parameterNum = 0;
                            break;
                        default:
                        ;
                        //! change status
                    }
                    break;
                case CC_LOADCURSOR:
                    switch (m_parameterNum) {
                        case 0:
                            // Char number
                            m_cursorPos = value & 0x7f;
                            m_parameterNum = 1;
                            m_isCompleteCommand = false;
                            m_statusReg |= 0x08; // set IC flag
                            break;
                        case 1:
                            // Row number
                            m_cursorRow = value & 0x3f;
                            m_parameterNum = 0;
                            m_isCompleteCommand = true;
                            m_statusReg &= ~0x08; // reset IC flag
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
        case 1:
            // Writing Command Register
            m_cmdReg = value;
            //m_statusReg |= m_isCompleteCommand ? 0 : 0x08;
            m_isCompleteCommand = true;
            m_statusReg &= ~0x08; // reset IC flag
            switch ((value & 0xE0) >> 5) {
                case 0:
                    // Reset
                    m_crtCmd = CC_RESET;
                    m_parameterNum = 0;
                    m_isCompleteCommand = false;
                    m_isDisplayStarted = false;
                    m_statusReg &= 0xBB; // reset IE and VE flags
                    m_statusReg |= 0x08; // set IC flag
                    startRasterIfNotStarted();
                    break;
                case 1:
                    // Start Display
                    m_burstSpaceCount = ((value & 0x1C) >> 2) * 8;
                    if (m_burstSpaceCount != 0)
                        m_burstSpaceCount--;
                    m_burstCount = 1 << (value & 0x03);
                    m_statusReg |= 0x44; // VE & IE
                    m_isIntsEnabled = true;
                    m_isDisplayStarted = true;
                    startRasterIfNotStarted();
                    break;
                case 2:
                    // Stop Display
                    m_statusReg &= 0xFb;
                    //m_raster->stopRaster();
                    m_isDisplayStarted = false;
                    startRasterIfNotStarted();
                    break;
                case 3:
                    // Read Light Pen
                    m_crtCmd = CC_READLPEN;
                    m_parameterNum = 0;
                    startRasterIfNotStarted();
                    break;
                case 4:
                    // Load Cursor Position
                    m_crtCmd = CC_LOADCURSOR;
                    m_parameterNum = 0;
                    m_isCompleteCommand = false;
                    m_statusReg |= 0x08; // set IC flag
                    startRasterIfNotStarted();
                    break;
                case 5:
                    // Enable Interrupts
                    m_isIntsEnabled = true;
                    m_statusReg |= 0x40; // IE
                    startRasterIfNotStarted();
                    break;
                case 6:
                    // Disable Interrupts
                    m_isIntsEnabled = false;
                    m_statusReg &= ~0x40;
                    startRasterIfNotStarted();
                    break;
                case 7:
                    // Preset Counetrs
                    presetCounters();
                    m_isRasterStarted = false;
                    m_raster->stopRaster();
                    break;
                default:
                    break;
            }
            break;
        default:
            // Normally this never occurs
            break;
    }
}

uint8_t Crt8275::readByte(int nAddr)
{
    uint8_t value;
    nAddr &= 1;
    switch (nAddr) {
        case 0:
            // Reading Parameter Register
            switch (m_crtCmd) {
                case CC_RESET:
                    value = m_resetParam[m_parameterNum++];
                    if (m_parameterNum == 4) {
                            m_isCompleteCommand = false;
                            m_statusReg |= 0x08; // set IC flag
                            m_parameterNum = 0;
                    }
                    break;
                case CC_LOADCURSOR:
                    switch (m_parameterNum) {
                        case 0:
                            // Char number
                            value = m_cursorPos;
                            m_parameterNum = 1;
                            break;
                        default: //case 1:
                            // Row number
                            value = m_cursorRow;
                            m_parameterNum = 0;
                            m_isCompleteCommand = false;
                            m_statusReg |= 0x08; // set IC flag
                            break;
                    }
                    break;
                case CC_READLPEN:
                    switch (m_parameterNum) {
                        case 0:
                            // Char number
                            value = m_lpenX;
                            m_parameterNum = 1;
                            break;
                        default: //case 1:
                            // Row number
                            value = m_lpenY;
                            m_parameterNum = 0;
                            m_isCompleteCommand = false;
                            break;
                    }
                    break;
                default:
                    value = m_cmdReg & 0x7f; // umdocumented;
            }
            break;
        default: //case 1:
            // Reading Status Register
            value = m_statusReg;
            m_statusReg &= 0xc4;
            break;
    }
    return value;
}



// ActiveDevice Related Methods Implemantation

void Crt8275::operate()
{
     //if (m_isDmaStoppedForRow || m_isDmaStoppedForFrame) {
     //    return;
     //}

    uint8_t byte;

    if (m_isBurstSpace) {
        m_isBurstSpace = false;
        m_curClock += (m_burstSpaceCount * m_kDiv);
        return;
    }

    m_isBurst = m_curBurstPos == 0;

    if (m_dma->dmaRequest(m_dmaChannel, byte, m_isBurst ? m_curClock : 0)) {
        putCharToBuffer(byte);
        if (!m_isPaused) { // !!!
            m_curClock += (m_isBurst ? 8 : 4) * m_cpuKDiv;
            if ((m_curBurstPos == m_burstCount - 1) && (m_curClock % m_kDiv != 0))
                m_curClock = m_curClock + m_kDiv - m_curClock % m_kDiv;
        }
        m_curBurstPos = (m_curBurstPos + 1) % m_burstCount;
        if (m_curBurstPos == 0 && m_burstSpaceCount != 0)
            m_isBurstSpace = true;
    }
    else
        dmaUnderrun();
  //if (m_isPaused) m_curClock = -1;
}


void Crt8275::putCharToBuffer(uint8_t byte)
{
    if (m_needExtraByte) {
        m_needExtraByte = false;
        pause();
        return;
    }

    if (m_isNextCharToFifo) {
        m_fifo[m_curFifoPos++] = byte & 0x7f;
        m_curFifoPos %= 16;
        if (m_curFifoPos == 0)
            m_statusReg |= 0x01; // FIFO Overrun
        m_isNextCharToFifo = false;
        if (m_curBufPos == m_nCharsPerRow) {
            // end of row
            pause();
             //m_isDmaStoppedForRow = false;
            //m_displayBuffer();
        }
    } else {
        if (m_curBufPos < m_nCharsPerRow)
            m_rowBuf[m_curBufPos++ % 80] = byte;
        else {
            pause();
            return;
        }

        if ((byte & 0xf1) == 0xF1) {
            // stop DMA
            if (byte & 0x2)
                // end of screen - stop DMA
                m_isDmaStoppedForFrame = true;
            else
                // end of row - stop DMA
                m_isDmaStoppedForRow = true;
            // extra DMA request after stop DMA at last symbol at row or DMA burst
            if ((m_curBufPos == m_nCharsPerRow /*- 1*/) || (m_curBurstPos == m_burstCount - 1))
                pause();
            else
                m_needExtraByte = true;
        } else if (m_isTransparentAttr && (byte & 0xc0) == 0x80)
            // field attribute code and transparent attribute is active
            m_isNextCharToFifo = true;
        else if (m_curBufPos == m_nCharsPerRow) {
            // end of row
//            m_displayBuffer();
            pause();
        }
        //m_curBufPos++;
    }
}


void Crt8275::dmaUnderrun()
{
    m_wasDmaUnderrun = true;
    m_statusReg |= 0x02;
    pause();
}


void Crt8275::nextRow()
{
    if (!m_isPaused)
        dmaUnderrun();

    displayBuffer();

    m_curBufPos = 0;
    m_curFifoPos = 0;
    m_curBurstPos = 0;
    m_isNextCharToFifo = false;
    m_isDmaStoppedForRow = false;
    m_needExtraByte = false;

    m_curRow = m_raster->m_curScanRow;
    if ((m_curRow >= m_nRows) || m_isDmaStoppedForFrame || m_wasDmaUnderrun)
        pause();
    else {
        syncronize();
        resume();
    }
}


void Crt8275::nextFrame()
{
    // Начало новго фрейма
    prepareFrame();
    presetCounters();
}



void Crt8275::presetCounters()
{
    m_curRow = 0;
    m_isDmaStoppedForFrame = false;
    m_wasDmaUnderrun = false;

    m_curBufPos = 0;
    m_curFifoPos = 0;
    m_curBurstPos = 0;
    m_isNextCharToFifo = false;
    m_isDmaStoppedForRow = false;
}



void Crt8275::startRasterIfNotStarted()
{
    if (!m_isRasterStarted) {
        m_raster->startRaster();
        m_isRasterStarted = true;
    }
}


//void Crt8275::startDisplay()
//{
//    m_isDisplayStarted = true;
//    resume();
//}



void Crt8275::stopDisplay()
{
    m_isDisplayStarted = false;
    pause();
}



bool Crt8275::getVsyncOccured()
{
    bool res = m_wasVsync;
    m_wasVsync = false;
    return res;
}



double Crt8275::getFrameRate()
{
    int chars = (m_nRows + m_nVrRows) * m_nLines * (m_nCharsPerRow + m_nHrChars);
    if (chars == 0)
        return 0.0;
    return double(g_emulation->getFrequency() / m_kDiv) / chars;
}


void Crt8275::setLpenPosition(int x, int y)
{
    m_lpenX = x + m_lpenCorrection;
    m_lpenY = y;
    m_statusReg |= 0x10;
}


//////////////////////////////////
// Crt8275Raster Implementation //
//////////////////////////////////

Crt8275Raster::Crt8275Raster()
{
    pause();
}



void Crt8275Raster::stopRaster()
{
    //m_curScanRow = 0;
    //m_curScanLine = 0;
    m_isHrtcActive = false;
    m_isVrtcActive = false;
    //m_crt->m_stopDisplay();
    pause();
}



void Crt8275Raster::startRaster()
{
     m_curScanRow = 0;
     m_curScanLine = 0;

    syncronize();
    resume();

    m_crt->nextFrame();
    m_crt->syncronize();
    m_crt->resume();
}



void Crt8275Raster::operate()
{
    if (!m_isHrtcActive) {
        // normal display
        m_curClock += m_crt->m_nCharsPerRow * m_kDiv;
        m_isHrtcActive = true;
        m_core->hrtc(true, m_curScanLine);
    } else {
        // HRTC active
        m_curClock += m_crt->m_nHrChars * m_kDiv;
        //m_crt->syncronize(m_curClock);
        m_isHrtcActive = false;
        //_crt->_wasVsync = true;
        m_core->hrtc(false, m_curScanLine);
        ++m_curScanLine;
        m_curScanLine %= m_crt->m_nLines;
        if (m_curScanLine == 0) {
            // next row
            m_curScanRow++;
            m_crt->syncronize(m_curClock);
            //if (m_curScanRow < m_crt->m_nRows)
            if (m_curScanRow <= m_crt->m_nRows) {
                m_crt->syncronize(m_curClock);
                m_crt->nextRow();
            }
            //if (m_crt->m_isIntsEnabled && (m_curScanRow == m_crt->m_nRows - 1))
            //    m_crt->m_statusReg |= 0x20;
            if (m_curScanRow == m_crt->m_nRows) {
                // next row is VRTC
                if (m_crt->m_isIntsEnabled)
                    m_crt->m_statusReg |= 0x20; // actually should be at the beginning of the last display row
                m_isVrtcActive = true;
                m_crt->m_wasVsync = true;
                m_core->vrtc(true);
            } else if (m_curScanRow >= m_crt->m_nRows + m_crt->m_nVrRows) {
                // frame complete
                m_isVrtcActive = false;
                m_core->vrtc(false);
                m_crt->syncronize(m_curClock);
                m_crt->nextFrame();
                m_crt->resume();
                //! m_renderer->drawFrame();
                m_curScanRow = 0;
                m_curScanLine = 0;
            }
        }
    }
}



bool Crt8275::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "dma") {
        if (values[1].isInt()) {
            attachDMA(static_cast<Dma8257*>(g_emulation->findObject(values[0].asString())), values[1].asInt());
            return true;
        }
    } else if (propertyName == "core") {
        attachCore(static_cast<PlatformCore*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "lpenCorrection") {
        m_lpenCorrection = values[0].asInt();
        return true;
    }

    return false;
}


string Crt8275::getDebugInfo()
{
    stringstream ss;
    ss << "CRT i8275:" << "\n";
    ss << "Row:" << m_nRows << " ";
    ss << "Ch:" << m_nCharsPerRow << "\n";
    ss << "Lc:" << m_nLines << " ";
    ss << "Ul:" << m_undLine << "\n";
    ss << "Vrc:" << m_nVrRows << " ";
    ss << "Hrc:" << m_nHrChars << "\n";
    ss << "Dcb:" << m_burstCount << " ";
    ss << "Bsc:" << m_burstSpaceCount << "\n";
    ss << "CX:" << m_cursorPos << " ";
    ss << "CY:" << m_cursorRow << "\n";
    ss << "Trp" << (m_isTransparentAttr ? "+" : "-") << " ";
    ss << "OfsL" << (m_isOffsetLine ? "+" : "-") << "\n";
    ss << "Cur:" << (m_cursorUnderline ? "Ul" : "Bl");
    ss << (m_cursorBlinking ? "Bln" : "") << " ";
    ss << "R" << (m_isRasterStarted ? "+" : "-") << "\n";
    ss << "CurRow: ";
    if (m_curRow < m_nRows)
          ss << m_curRow;
    else
          ss << "VRTC";
    ss << "\n";
    return ss.str();
}
