/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2019
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

#include <sstream>
#include <iomanip>
#include <algorithm>

#include <string.h>

#include "Debugger.h"
#include "Emulation.h"
#include "Platform.h"
#include "Pal.h"

#include "CpuZ80.h"
#include "Cpu8080dasm.h"
#include "CpuZ80dasm.h"

using namespace std;

DebugWindow::DebugWindow(Platform* platform)
{
    m_windowType = EWT_DEBUG;

    m_compactLayout.cols = 80;
    m_compactLayout.rows = 40;
    m_compactLayout.code.left = 1;
    m_compactLayout.code.top = 1;
    m_compactLayout.code.width = 34;
    m_compactLayout.code.height = 21;
    m_compactLayout.regs.left = 36;
    m_compactLayout.regs.top = 1;
    m_compactLayout.regs.width = 11;
    m_compactLayout.regs.height = 6;
    m_compactLayout.flags.left = 48;
    m_compactLayout.flags.top = 1;
    m_compactLayout.flags.width = 7;
    m_compactLayout.flags.height = 6;
    m_compactLayout.stack.left = 56;
    m_compactLayout.stack.top = 1;
    m_compactLayout.stack.width = 15;
    m_compactLayout.stack.height = 6;
    m_compactLayout.bpts.left = 72;
    m_compactLayout.bpts.top = 1;
    m_compactLayout.bpts.width = 7;
    m_compactLayout.bpts.height = 6;
    m_compactLayout.regMemHex.left = 36;
    m_compactLayout.regMemHex.top = 8;
    m_compactLayout.regMemHex.width = 43;
    m_compactLayout.regMemHex.height = 7;
    m_compactLayout.regMemSmb.left = 36;
    m_compactLayout.regMemSmb.top = 16;
    m_compactLayout.regMemSmb.width = 43;
    m_compactLayout.regMemSmb.height = 6;
    m_compactLayout.dump.left = 1;
    m_compactLayout.dump.top = 23;
    m_compactLayout.dump.width = 78;
    m_compactLayout.dump.height = 15;
    m_compactLayout.menu.left = 0;
    m_compactLayout.menu.top = 39;
    m_compactLayout.menu.width = 80;
    m_compactLayout.menu.height = 1;
    m_compactLayout.aux.left = 0;
    m_compactLayout.aux.top = 0;
    m_compactLayout.aux.width = 0;
    m_compactLayout.aux.height = 0;

    m_stdLayout.cols = 96;
    m_stdLayout.rows = 48;
    m_stdLayout.code.left = 1;
    m_stdLayout.code.top = 1;
    m_stdLayout.code.width = 37;
    m_stdLayout.code.height = 28;
    m_stdLayout.regs.left = 39;
    m_stdLayout.regs.top = 1;
    m_stdLayout.regs.width = 23;
    m_stdLayout.regs.height = 8;
    m_stdLayout.flags.left = 63;
    m_stdLayout.flags.top = 1;
    m_stdLayout.flags.width = 8;
    m_stdLayout.flags.height = 8;
    m_stdLayout.stack.left = 72;
    m_stdLayout.stack.top = 1;
    m_stdLayout.stack.width = 15;
    m_stdLayout.stack.height = 8;
    m_stdLayout.bpts.left = 88;
    m_stdLayout.bpts.top = 1;
    m_stdLayout.bpts.width = 7;
    m_stdLayout.bpts.height = 8;
    m_stdLayout.regMemHex.left = 39;
    m_stdLayout.regMemHex.top = 10;
    m_stdLayout.regMemHex.width = 57;
    m_stdLayout.regMemHex.height = 9;
    m_stdLayout.regMemSmb.left = 39;
    m_stdLayout.regMemSmb.top = 20;
    m_stdLayout.regMemSmb.width = 42;
    m_stdLayout.regMemSmb.height = 9;
    m_stdLayout.dump.left = 1;
    m_stdLayout.dump.top = 30;
    m_stdLayout.dump.width = 80;
    m_stdLayout.dump.height = 16;
    m_stdLayout.menu.left = 0;
    m_stdLayout.menu.top = 47;
    m_stdLayout.menu.width = 96;
    m_stdLayout.menu.height = 1;
    m_stdLayout.aux.left = 82;
    m_stdLayout.aux.top = 20;
    m_stdLayout.aux.width = 13;
    m_stdLayout.aux.height = 26;

    m_platform = platform;
    m_cpu = static_cast<Cpu8080Compatible*>(m_platform->getCpu());
    m_z80cpu = dynamic_cast<CpuZ80*>(m_cpu);
    m_as = m_cpu->getAddrSpace();

    m_z80Mode = m_z80cpu != nullptr;

    memset(&m_states, 0, sizeof(m_states));

    int bytesToRead = (256 + 128) * 12;
    m_font = palReadFile("dbgfont.bin", bytesToRead);

    m_isRunning = true;

    for (int i = 0; i < 96; i++)
        for (int j = 0; j < 48; j++) {
            m_screen[i][j].chr = 0;
            m_screen[i][j].fgColor = 14;
            m_screen[i][j].bgColor = 1;
    }

    setCaption("Emu80 Debugger");

    m_pixelData.prevPixelData = nullptr;
    m_pixelData.aspectRatio = 1.0;
    m_pixelData.prevAspectRatio = 1.0;

    setLayout(false);

    m_cursorVisible = false;

    fillCpuStatus();

    m_mode = AM_CODE;

    codeInit();
    dumpInit();
    regsInit();
    flagsInit();
    inputInit();

    setWindowStyle(WS_AUTOSIZE);
    setFrameScale(FS_1X);
}


DebugWindow::~DebugWindow()
{
    delete[] m_pixels;
    delete[] m_font;
}


void DebugWindow::setLayout(bool isCompact)
{
    m_compactMode = isCompact;
    m_curLayout = m_compactMode ? &m_compactLayout : &m_stdLayout;

    if (m_pixels)
        delete[] m_pixels;

    m_pixelData.width = m_curLayout->cols * m_chrW;
    m_pixelData.height = m_curLayout->rows * m_chrH;
    m_pixels = new uint32_t[m_curLayout->cols * m_chrW * m_curLayout->rows * m_chrH];
    m_pixelData.pixelData = m_pixels;

    setDefaultWindowSize(m_curLayout->cols * m_chrW, m_curLayout->rows * m_chrH);
}


void DebugWindow::closeRequest()
{
    run();
}


void DebugWindow::startDebug()
{
    if (!m_isRunning)
        return;

    if (m_resetCpuClockFlag || (m_tempBp && m_tempBp->getHookAddr() != m_cpu->getPC()))
        resetCpuClock();

    m_isRunning = false;

    if (m_tempBp) {
        delete m_tempBp;
        m_tempBp = nullptr;
    }

    m_stateNum = 1 - m_stateNum;
    fillCpuStatus();
    codeGotoPc();
    show();
    draw();
}


void DebugWindow::putString(int x, int y, string s, int fgColor, int bgColor)
{
    m_curX = x;
    m_curY = y;
    m_curFgColor = fgColor;
    m_curBgColor = bgColor;
    putString(s);
}


void DebugWindow::putString(int x, int y, string s)
{
    m_curX = x;
    m_curY = y;
    putString(s);
}


void DebugWindow::putString(string s, int fgColor, int bgColor)
{
    m_curFgColor = fgColor;
    m_curBgColor = bgColor;
    putString(s);
}

void DebugWindow::putString(string s)
{
    for (unsigned i = 0; i < s.size(); i++) {
        if (m_curX >= m_curLayout->cols) {
            m_curX = 0;
            if (++m_curY >= m_curLayout->rows)
                break;
        }

        m_screen[m_curX][m_curY].chr = s[i];
        m_screen[m_curX][m_curY].fgColor = m_curFgColor;
        m_screen[m_curX][m_curY].bgColor = m_curBgColor;
        ++m_curX;
    }
}

void DebugWindow::draw()
{
    const DebuggerOptions& debOpt = g_emulation->getDebuggerOptions();
    m_mnemo8080UpperCase = debOpt.mnemo8080UpperCase;
    m_mnemoZ80UpperCase = debOpt.mnemoZ80UpperCase;
    m_swapF5F9 = debOpt.swapF5F9;

    checkForInput();

    drawDbgFrame();
    displayCpuStatus();
    displayObjectDbgInfo();

    drawHintBar();

    for (int col = 0; col < m_curLayout->cols; col++)
        for (int row = 0; row < m_curLayout->rows; row++) {
            unsigned chr = m_screen[col][row].chr;
            unsigned fgColor = m_screen[col][row].fgColor;
            unsigned bgColor = m_screen[col][row].bgColor;
            uint8_t* fnt = m_font + chr * m_chrH;
            for (int line = 0; line < m_chrH; line++) {
                uint8_t bt = *fnt++;
                for (int point = 0; point < m_chrW; point++) {
                    m_pixels[point + m_chrW * (col + m_curLayout->cols * (line + row * m_chrH))] = m_palette[bt & 0x80 ? fgColor : bgColor];
                    bt <<= 1;
                }
            }
        }

    if (m_cursorVisible && m_cursorCounter < 15) {
        int scrX = m_cursorXPos * m_chrW;
        int scrY = m_cursorYPos * m_chrH + m_chrH - 3;
        int pos = scrY * m_curLayout->cols * m_chrW + scrX;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < m_chrW; j++)
                m_pixels[pos + j] ^= 0xFFFFFF;
            pos += m_curLayout->cols * m_chrW;
        }
    }
    m_cursorCounter = (m_cursorCounter + 1) % 30;

    drawFrame(m_pixelData);
    endDraw();
}


static string int2Hex(unsigned n, int width)
{
    ostringstream oss;
    oss << setw(width);
    oss << setfill('0');
    oss << uppercase << hex  << n;
    return oss.str();
}


static unsigned hex2Int(string s)
{
    int res;
    if (s == "")
        s = "0";
    istringstream iss(s);
    iss >> hex >> res;
    return res;
}


static string int2Str(int n)
{
    ostringstream oss;
    oss << n;
    return oss.str();
}

void DebugWindow::putChars(unsigned chr, int num, bool isVertical)
{
    for (int i = 0; i < num; i++) {
        if (m_curX >= m_curLayout->cols || m_curY >= m_curLayout->rows)
            break;

        m_screen[m_curX][m_curY].chr = chr;
        m_screen[m_curX][m_curY].fgColor = m_curFgColor;
        m_screen[m_curX][m_curY].bgColor = m_curBgColor;

        if (isVertical)
            ++m_curY;
        else
            ++m_curX;
    }
}


void DebugWindow::putChars(int x, int y, unsigned chr, int num, bool isVertical)
{
    m_curX = x;
    m_curY = y;
    putChars(chr, num, isVertical);
}


void DebugWindow::highlight(int x, int y, int color, int num, bool isVertical)
{
    for (int i = 0; i < num; i++) {
        if (x >= m_curLayout->cols || y >= m_curLayout->rows)
            break;

        m_screen[x][y].bgColor = color;

        if (isVertical)
            ++y;
        else
            ++x;
    }
}


void DebugWindow::clearBlock(int x1, int y1, int x2, int y2, int bgColor)
{
    for (int x = x1; x <= x2 && x < m_curLayout->cols; x++)
        for (int y = y1; y <= y2 && y < m_curLayout->rows; y++) {
            m_screen[x][y].chr = 0;
            m_screen[x][y].bgColor = bgColor;
        }
}


void DebugWindow::drawDbgFrame()
{
    clearBlock(0, 0, 95, 47, 1);
    putChars(1, 0, 0xcd, m_curLayout->cols - 2);
    putChars(1, m_curLayout->rows - 2, 0xcd, m_curLayout->cols - 2);
    putChars(0, 1, 0xba, m_curLayout->rows - 2, true);
    putChars(m_curLayout->cols - 1, 1, 0xba, m_curLayout->rows - 2, true);
    putChars(m_curLayout->regs.left - 1, 1, 0xb3, m_curLayout->regs.height + m_curLayout->regMemHex.height + m_curLayout->regMemSmb.height + 2, true);
    putChars(m_curLayout->flags.left - 1, 1, 0xb3, m_curLayout->flags.height, true);
    putChars(m_curLayout->stack.left - 1, 1, 0xb3, m_curLayout->stack.height, true);
    putChars(m_curLayout->bpts.left - 1, 1, 0xb3, m_curLayout->bpts.height, true);
    putChars(1, m_curLayout->dump.top - 1, 0xc4, m_curLayout->dump.width);
    putChars(m_curLayout->regMemHex.left, m_curLayout->regMemHex.top - 1, 0xc4, m_curLayout->regMemHex.width);
    if (m_curLayout->aux.width == 0)
        putChars(m_curLayout->regMemSmb.left, m_curLayout->regMemSmb.top - 1, 0xc4, m_curLayout->regMemSmb.width);
    else
        putChars(m_curLayout->regMemSmb.left, m_curLayout->regMemSmb.top - 1, 0xc4, m_curLayout->regMemSmb.width + m_curLayout->aux.width + 1);

    putChars(0, 0, 0xc9, 1);
    putChars(m_curLayout->cols - 1, 0, 0xbb, 1);
    putChars(0, m_curLayout->rows - 2, 0xc8, 1);
    putChars(m_curLayout->cols - 1, m_curLayout->rows - 2, 0xbc, 1);
    putChars(m_curLayout->regs.left - 1, 0, 0xd1, 1);
    putChars(m_curLayout->flags.left - 1, 0, 0xd1, 1);
    putChars(m_curLayout->stack.left - 1, 0, 0xd1, 1);
    putChars(m_curLayout->bpts.left - 1, 0, 0xd1, 1);
    putChars(0, m_curLayout->dump.top - 1, 0xc7, 1);
    putChars(m_curLayout->regs.left - 1, m_curLayout->regMemHex.top - 1, 0xc3, 1);
    putChars(m_curLayout->regs.left - 1, m_curLayout->regMemSmb.top - 1, 0xc3, 1);
    putChars(m_curLayout->cols - 1, m_curLayout->regMemHex.top - 1, 0xb6, 1);
    putChars(m_curLayout->cols - 1, m_curLayout->regMemSmb.top - 1, 0xb6, 1);
    if (m_curLayout->aux.width == 0)
        putChars(m_curLayout->cols - 1, m_curLayout->dump.top - 1, 0xb6, 1);
    putChars(m_curLayout->flags.left - 1, m_curLayout->regMemHex.top - 1, 0xc1, 1);
    putChars(m_curLayout->stack.left - 1, m_curLayout->regMemHex.top - 1, 0xc1, 1);
    putChars(m_curLayout->bpts.left - 1, m_curLayout->regMemHex.top - 1, 0xc1, 1);
    putChars(m_curLayout->regs.left - 1, m_curLayout->dump.top - 1, 0xc1, 1);

    if (m_curLayout->aux.width != 0) {
        putChars(m_curLayout->aux.left - 1, m_curLayout->aux.top, 0xb3, m_curLayout->aux.height, true);
        putChars(m_curLayout->aux.left - 1, m_curLayout->aux.top - 1, 0xc2, 1);
        putChars(m_curLayout->aux.left - 1, m_curLayout->rows - 2,  0xcf, 1);
        putChars(m_curLayout->aux.left - 1, m_curLayout->dump.top - 1,  0xb4, 1);
    }

    setColors(11, 1);
    putString(m_curLayout->regs.left + 1, m_curLayout->regs.top, "AF = ");
    putString(m_curLayout->regs.left + 1, m_curLayout->regs.top + 1, "BC = ");
    putString(m_curLayout->regs.left + 1, m_curLayout->regs.top + 2, "DE = ");
    putString(m_curLayout->regs.left + 1, m_curLayout->regs.top + 3, "HL = ");
    putString(m_curLayout->regs.left + 1, m_curLayout->regs.top + 4, "SP = ");
    putString(m_curLayout->regs.left + 1, m_curLayout->regs.top + 5, "PC = ");
    if (m_z80Mode) {
        putString(m_curLayout->regs.left + 12, m_curLayout->regs.top, "AF' = ");
        putString(m_curLayout->regs.left + 12, m_curLayout->regs.top + 1, "BC' = ");
        putString(m_curLayout->regs.left + 12, m_curLayout->regs.top + 2, "DE' = ");
        putString(m_curLayout->regs.left + 12, m_curLayout->regs.top + 3, "HL' = ");
        putString(m_curLayout->regs.left + 12, m_curLayout->regs.top + 4, "IX  = ");
        putString(m_curLayout->regs.left + 12, m_curLayout->regs.top + 5, "IY  = ");

        putString(m_curLayout->regs.left + 1, m_curLayout->regs.top + 7, "R  = ");
        putString(m_curLayout->regs.left + 12, m_curLayout->regs.top + 7, "IM  = ");
    }
    // else if (!m_compactMode)
        //clearBlock(m_curLayout->regs.left + 12, m_curLayout->regs.top, m_curLayout->regs.left + 12 + 9, m_curLayout->regs.top + 7, 1);

    //clearBlock(m_curLayout->flags.left, m_curLayout->flags.top, m_curLayout->flags.left + m_curLayout->flags.width - 1, m_curLayout->flags.top + m_curLayout->flags.height - 1, 1);
    if (m_z80Mode) {
        putString(m_curLayout->flags.left + 2, m_curLayout->flags.top, "C =");
        putString(m_curLayout->flags.left + 2, m_curLayout->flags.top + 1, "Z =");
        putString(m_curLayout->flags.left + 1, m_curLayout->flags.top + 2, "PV =");
        putString(m_curLayout->flags.left + 2, m_curLayout->flags.top + 3, "S =");
        putString(m_curLayout->flags.left + 2, m_curLayout->flags.top + 4, "H =");
        putString(m_curLayout->flags.left + 2, m_curLayout->flags.top + 5, "N =");
    } else {
        int offset = m_compactMode ? 1 : 2;
        putString(m_curLayout->flags.left + offset, m_curLayout->flags.top, "C =");
        putString(m_curLayout->flags.left + offset, m_curLayout->flags.top + 1, "Z =");
        putString(m_curLayout->flags.left + offset, m_curLayout->flags.top + 2, "P =");
        putString(m_curLayout->flags.left + offset, m_curLayout->flags.top + 3, "S =");
        if (m_compactMode)
            putString(m_curLayout->flags.left + 1, m_curLayout->flags.top + 4, "AC=");
        else
            putString(m_curLayout->flags.left + 1, m_curLayout->flags.top + 4, "AC =");
    }

    if (!m_compactMode)
        putString(m_curLayout->flags.left + 1, m_curLayout->flags.top + (m_z80Mode ? 7 : 6), "IFF=");

    putString(m_curLayout->stack.left + 1, m_curLayout->stack.top, "SP     : ");
    putString(m_curLayout->stack.left + 1, m_curLayout->stack.top + 1, "SP+0002: ");
    putString(m_curLayout->stack.left + 1, m_curLayout->stack.top + 2, "SP+0004: ");
    putString(m_curLayout->stack.left + 1, m_curLayout->stack.top + 3, "SP+0006: ");
    putString(m_curLayout->stack.left + 1, m_curLayout->stack.top + 4, "SP+0008: ");
    putString(m_curLayout->stack.left + 1, m_curLayout->stack.top + 5, "SP+000A: ");
    if (!m_compactMode) {
        putString(m_curLayout->stack.left + 1, m_curLayout->stack.top + 6, "SP+000C: ");
        putString(m_curLayout->stack.left + 1, m_curLayout->stack.top + 7, "SP+000E: ");
    }

    putString(m_curLayout->regMemHex.left + 1, m_curLayout->regMemHex.top + 1, "  HL=    :");
    putString(m_curLayout->regMemHex.left + 1, m_curLayout->regMemHex.top + 2, "  BC=    :");
    putString(m_curLayout->regMemHex.left + 1, m_curLayout->regMemHex.top + 3, "  DE=    :");
    putString(m_curLayout->regMemHex.left + 1, m_curLayout->regMemHex.top + 4, "  SP=    :");
    putString(m_curLayout->regMemHex.left + 1, m_curLayout->regMemHex.top + 5, "[HL]=    :");
    putString(m_curLayout->regMemHex.left + 1, m_curLayout->regMemHex.top + 6, "[SP]=    :");
    if (m_z80Mode) {
        putString(m_curLayout->regMemHex.left + 1, m_curLayout->regMemHex.top + 7, "  IX=    :");
        putString(m_curLayout->regMemHex.left + 1, m_curLayout->regMemHex.top + 8, "  IY=    :");
    } else if (!m_compactMode) {
        putChars(m_curLayout->regMemHex.left + 1, m_curLayout->regMemHex.top + 7, 0, 52);
        putChars(m_curLayout->regMemHex.left + 1, m_curLayout->regMemHex.top + 8, 0, 52);
    }
    putString(m_curLayout->regMemSmb.left + 1, m_curLayout->regMemSmb.top + 1, "  HL=    :");
    putString(m_curLayout->regMemSmb.left + 1, m_curLayout->regMemSmb.top + 2, "  BC=    :");
    putString(m_curLayout->regMemSmb.left + 1, m_curLayout->regMemSmb.top + 3, "  DE=    :");
    putString(m_curLayout->regMemSmb.left + 1, m_curLayout->regMemSmb.top + 4, "  SP=    :");
    putString(m_curLayout->regMemSmb.left + 1, m_curLayout->regMemSmb.top + 5, "[HL]=    :");
    if (!m_compactMode)
        putString(m_curLayout->regMemSmb.left + 1, m_curLayout->regMemSmb.top + 6, "[SP]=    :");
    if (m_z80Mode) {
        putString(m_curLayout->regMemSmb.left + 1, m_curLayout->regMemSmb.top + 7, "  IX=    :");
        putString(m_curLayout->regMemSmb.left + 1, m_curLayout->regMemSmb.top + 8, "  IY=    :");
    } else if (!m_compactMode) {
        putChars(m_curLayout->regMemSmb.left + 1, m_curLayout->regMemSmb.top + 7, 0, 39);
        putChars(m_curLayout->regMemSmb.left + 1, m_curLayout->regMemSmb.top + 8, 0, 39);
    }

    setColors(7, 1);
    putString(m_curLayout->dump.left + 9, m_curLayout->dump.top, ".0 .1 .2 .3 .4 .5 .6 .7 .8 .9 .A .B .C .D .E .F   0123456789ABCDEF");
    if (m_compactMode)
        putString(m_curLayout->regMemHex.left + 12, m_curLayout->regMemHex.top, "-4 -3 -2 -1  0 +1 +2 +3 +4 +5");
    else
        putString(m_curLayout->regMemHex.left + 12, m_curLayout->regMemHex.top, "-6 -5 -4 -3 -2 -1  0 +1 +2 +3 +4 +5 +6 +7");

    putString(m_curLayout->regMemSmb.left + 11, m_curLayout->regMemSmb.top, "-DCBA9876543210123456789ABCDEF+");
}


void DebugWindow::resetCpuClock()
{
    m_cpuClock = m_cpu->getClock() / m_cpu->getKDiv();
}


void DebugWindow::displayObjectDbgInfo()
{
    if (m_compactMode)
        return;

    int clocks = int(m_cpu->getClock() / m_cpu->getKDiv() - m_cpuClock);
    ostringstream ss;
    ss << clocks;

    //const char* s = m_platform->getAllDebugInfo().c_str(); // !! figure out why does this not work?
    string info = "CPU:\n" + ss.str() + "\n\n" + m_platform->getAllDebugInfo();
    const char* s = info.c_str();

    int x = 0;
    int y = 0;
    int i = 0;

    while (s[i]) {
        if (y > m_curLayout->aux.height)
            break;
        if (x >= m_curLayout->aux.width) {
            y++;
            x = 0;
        }
        if (s[i] == '\n') {
            y++;
            x = 0;
        } else {
            m_screen[m_curLayout->aux.left + x][m_curLayout->aux.top + y].chr = s[i];
            m_screen[m_curLayout->aux.left + x++][m_curLayout->aux.top + y].fgColor = 11;
        }
        i++;
    }
}


void DebugWindow::fillCpuStatus()
{
    m_states[m_stateNum].af = m_cpu->getAF();
    m_states[m_stateNum].bc = m_cpu->getBC();
    m_states[m_stateNum].de = m_cpu->getDE();
    m_states[m_stateNum].hl = m_cpu->getHL();
    m_states[m_stateNum].sp = m_cpu->getSP();
    m_states[m_stateNum].pc = m_cpu->getPC();
    m_states[m_stateNum].iff = m_cpu->getInte();
    m_states[m_stateNum].a = (m_states[m_stateNum].af & 0xff00) >> 8;
    m_states[m_stateNum].f = m_states[m_stateNum].af & 0xff;
    m_states[m_stateNum].b = (m_states[m_stateNum].bc & 0xff00) >> 8;
    m_states[m_stateNum].c = m_states[m_stateNum].bc & 0xff;
    m_states[m_stateNum].d = (m_states[m_stateNum].de & 0xff00) >> 8;
    m_states[m_stateNum].e = m_states[m_stateNum].de & 0xff;
    m_states[m_stateNum].h = (m_states[m_stateNum].hl & 0xff00) >> 8;
    m_states[m_stateNum].l = m_states[m_stateNum].hl & 0xff;
    m_states[m_stateNum].fl_c = m_states[m_stateNum].af & 0b00000001 ? 1 : 0;
    m_states[m_stateNum].fl_z = m_states[m_stateNum].af & 0b01000000 ? 1 : 0;
    m_states[m_stateNum].fl_p = m_states[m_stateNum].af & 0b00000100 ? 1 : 0;
    m_states[m_stateNum].fl_m = m_states[m_stateNum].af & 0b10000000 ? 1 : 0;
    m_states[m_stateNum].fl_ac = m_states[m_stateNum].af & 0b00010000 ? 1 : 0;
    m_states[m_stateNum].stack0 = memWord(m_states[m_stateNum].sp);
    m_states[m_stateNum].stack2 = memWord(m_states[m_stateNum].sp + 2);
    m_states[m_stateNum].stack4 = memWord(m_states[m_stateNum].sp + 4);
    m_states[m_stateNum].stack6 = memWord(m_states[m_stateNum].sp + 6);
    m_states[m_stateNum].stack8 = memWord(m_states[m_stateNum].sp + 8);
    m_states[m_stateNum].stackA = memWord(m_states[m_stateNum].sp + 0xA);
    m_states[m_stateNum].stackC = memWord(m_states[m_stateNum].sp + 0xC);
    m_states[m_stateNum].stackE = memWord(m_states[m_stateNum].sp + 0xE);
    m_states[m_stateNum].word_mem_hl = memWord(m_states[m_stateNum].hl);
    m_states[m_stateNum].word_mem_sp = memWord(m_states[m_stateNum].sp);
    for (uint16_t i = 0; i < 29; i++) {
        m_states[m_stateNum].mem_hl[i] = memByte(m_states[m_stateNum].hl - 13 + i);
        m_states[m_stateNum].mem_bc[i] = memByte(m_states[m_stateNum].bc - 13 + i);
        m_states[m_stateNum].mem_de[i] = memByte(m_states[m_stateNum].de - 13 + i);
        m_states[m_stateNum].mem_sp[i] = memByte(m_states[m_stateNum].sp - 13 + i);
        m_states[m_stateNum].mem_mem_hl[i] = memByte(m_states[m_stateNum].word_mem_hl - 13 + i);
        m_states[m_stateNum].mem_mem_sp[i] = memByte(m_states[m_stateNum].word_mem_sp - 13 + i);
        if (m_z80cpu) {
            m_states[m_stateNum].mem_ix[i] = memByte(m_states[m_stateNum].de - 13 + i);
            m_states[m_stateNum].mem_iy[i] = memByte(m_states[m_stateNum].sp - 13 + i);
        }
    }

    if (m_z80cpu) {
        m_states[m_stateNum].af2 = m_z80cpu->getAF2();
        m_states[m_stateNum].bc2 = m_z80cpu->getBC2();
        m_states[m_stateNum].de2 = m_z80cpu->getDE2();
        m_states[m_stateNum].hl2 = m_z80cpu->getHL2();
        m_states[m_stateNum].ix = m_z80cpu->getIX();
        m_states[m_stateNum].iy = m_z80cpu->getIY();
        m_states[m_stateNum].a2 = (m_states[m_stateNum].af2 & 0xff00) >> 8;
        m_states[m_stateNum].f2 = m_states[m_stateNum].af2 & 0xff;
        m_states[m_stateNum].b2 = (m_states[m_stateNum].bc2 & 0xff00) >> 8;
        m_states[m_stateNum].c2 = m_states[m_stateNum].bc2 & 0xff;
        m_states[m_stateNum].d2 = (m_states[m_stateNum].de2 & 0xff00) >> 8;
        m_states[m_stateNum].e2 = m_states[m_stateNum].de2 & 0xff;
        m_states[m_stateNum].h2 = (m_states[m_stateNum].hl2 & 0xff00) >> 8;
        m_states[m_stateNum].l2 = m_states[m_stateNum].hl2 & 0xff;
        m_states[m_stateNum].fl_n = m_states[m_stateNum].af & 0b00000010 ? 1 : 0;
        m_states[m_stateNum].r = m_z80cpu->getR();
        m_states[m_stateNum].im = m_z80cpu->getIM();
    }
}


string DebugWindow::getInstructionMnemonic(uint16_t addr)
{
    string mnemo;

    uint8_t buf[4];
    buf[0] = memByte(addr);
    buf[1] = memByte(addr + 1);
    buf[2] = memByte(addr + 2);
    buf[3] = memByte(addr + 3); // Z80

    if (!m_z80Mode && !m_z80Mnemonics) {
        mnemo = i8080GetInstructionMnemonic(buf);
        if (!m_mnemo8080UpperCase)
            transform(mnemo.begin(), mnemo.end(), mnemo.begin(), ::tolower);
        return mnemo;
    } else {
        unsigned length;
        STEP_FLAG flag;
        mnemo = cpu_disassemble_z80(addr, buf, length, flag);
        if (m_mnemoZ80UpperCase)
            transform(mnemo.begin(), mnemo.end(), mnemo.begin(), ::toupper);
    }

    return mnemo;
}


int DebugWindow::getInstructionLength(uint16_t addr)
{
    uint8_t buf[4];
    buf[0] = memByte(addr);
    buf[1] = memByte(addr + 1); // Z80
    buf[2] = memByte(addr + 2); // Z80
    buf[3] = memByte(addr + 3); // Z80

    if (!m_z80Mode && !m_z80Mnemonics) {
        return i8080GetInstructionLength(buf);
    }

    unsigned length;
    STEP_FLAG flag;
    cpu_disassemble_z80(addr, buf, length, flag);
    return length;
}


bool DebugWindow::getInstructionOverFlag(uint16_t addr)
{
    if (!m_z80Mode && !m_z80Mnemonics) {
        uint8_t op = memByte(addr);
        return (op & 0xcf) == 0xcd /* CALL */ || (op & 0xc7) == 0xc7 /* RST */ || (op & 0xc7) == 0xc4 /* Cx */;
    }

    uint8_t buf[4];
    buf[0] = memByte(addr);
    buf[1] = memByte(addr + 1); // Z80
    buf[2] = memByte(addr + 2); // Z80
    buf[3] = memByte(addr + 3); // Z80

    unsigned length;
    STEP_FLAG flag;
    cpu_disassemble_z80(addr, buf, length, flag);
    return (flag == SF_OVER);
}


void DebugWindow::drawHexByte(int x, int y, uint8_t val, bool changed)
{
    setColors(changed ? 14 : 15, 1);
    putString(x, y, int2Hex(val, 2));
}


void DebugWindow::drawHexWord(int x, int y, uint16_t val, bool changed)
{
    setColors(changed ? 14 : 15, 1);
    putString(x, y, int2Hex(val, 4));
}


void DebugWindow::drawInt(int x, int y, int val, bool changed)
{
    setColors(changed ? 14 : 15, 1);
    putString(x, y, int2Str(val));
}


void DebugWindow::drawSymSeq(int x, int y, uint8_t* seq, uint8_t* old_seq, int len)
{
    setPos(x, y);
    for (int i = 0; i< len; i++) {
        setColors(seq[i] != old_seq[i] ? 14 : 15, 1);
        putChars(seq[i] < 128 ? seq[i] + 256 : 0, 1);
    }
}


void DebugWindow::drawHexSeq(int x, int y, uint8_t* seq, uint8_t* old_seq, int len)
{
    for (int i = 0; i< len; i++)
        drawHexByte(x + i * 3, y, seq[i], seq[i] != old_seq[i]);
}


void DebugWindow::displayCpuStatus()
{
    int baseY = m_curLayout->stack.top;
    int baseX = m_curLayout->stack.left + 10;
    drawHexWord(baseX, baseY++, m_states[m_stateNum].stack0, m_states[m_stateNum].stack0 != m_states[1-m_stateNum].stack0);
    drawHexWord(baseX, baseY++, m_states[m_stateNum].stack2, m_states[m_stateNum].stack2 != m_states[1-m_stateNum].stack2);
    drawHexWord(baseX, baseY++, m_states[m_stateNum].stack4, m_states[m_stateNum].stack4 != m_states[1-m_stateNum].stack4);
    drawHexWord(baseX, baseY++, m_states[m_stateNum].stack6, m_states[m_stateNum].stack6 != m_states[1-m_stateNum].stack6);
    drawHexWord(baseX, baseY++, m_states[m_stateNum].stack8, m_states[m_stateNum].stack8 != m_states[1-m_stateNum].stack8);
    drawHexWord(baseX, baseY++, m_states[m_stateNum].stackA, m_states[m_stateNum].stackA != m_states[1-m_stateNum].stackA);
    if (!m_compactMode) {
        drawHexWord(baseX, baseY++, m_states[m_stateNum].stackC, m_states[m_stateNum].stackC != m_states[1-m_stateNum].stackC);
        drawHexWord(baseX, baseY, m_states[m_stateNum].stackE, m_states[m_stateNum].stackE != m_states[1-m_stateNum].stackE);
        }

    baseY = m_curLayout->regMemHex.top + 1;
    baseX = m_curLayout->regMemHex.left + 7;
    drawHexWord(baseX, baseY++, m_states[m_stateNum].hl, m_states[m_stateNum].hl != m_states[1-m_stateNum].hl);
    drawHexWord(baseX, baseY++, m_states[m_stateNum].bc, m_states[m_stateNum].bc != m_states[1-m_stateNum].bc);
    drawHexWord(baseX, baseY++, m_states[m_stateNum].de, m_states[m_stateNum].de != m_states[1-m_stateNum].de);
    drawHexWord(baseX, baseY++, m_states[m_stateNum].sp, m_states[m_stateNum].sp != m_states[1-m_stateNum].sp);
    drawHexWord(baseX, baseY++, m_states[m_stateNum].word_mem_hl, m_states[m_stateNum].word_mem_hl != m_states[1-m_stateNum].word_mem_hl);
    drawHexWord(baseX, baseY++, m_states[m_stateNum].word_mem_sp, m_states[m_stateNum].word_mem_sp != m_states[1-m_stateNum].word_mem_sp);
    if (m_z80Mode) {
        drawHexWord(baseX, baseY++, m_states[m_stateNum].ix, m_states[m_stateNum].ix != m_states[1-m_stateNum].ix);
        drawHexWord(baseX, baseY, m_states[m_stateNum].iy, m_states[m_stateNum].iy != m_states[1-m_stateNum].iy);
    }

    baseY = m_curLayout->regMemSmb.top + 1;
    baseX = m_curLayout->regMemSmb.left + 7;
    drawHexWord(baseX, baseY++, m_states[m_stateNum].hl, m_states[m_stateNum].hl != m_states[1-m_stateNum].hl);
    drawHexWord(baseX, baseY++, m_states[m_stateNum].bc, m_states[m_stateNum].bc != m_states[1-m_stateNum].bc);
    drawHexWord(baseX, baseY++, m_states[m_stateNum].de, m_states[m_stateNum].de != m_states[1-m_stateNum].de);
    drawHexWord(baseX, baseY++, m_states[m_stateNum].sp, m_states[m_stateNum].sp != m_states[1-m_stateNum].sp);
    drawHexWord(baseX, baseY++, m_states[m_stateNum].word_mem_hl, m_states[m_stateNum].word_mem_hl != m_states[1-m_stateNum].word_mem_hl);
    if (!m_compactMode)
        drawHexWord(baseX, baseY++, m_states[m_stateNum].word_mem_sp, m_states[m_stateNum].word_mem_sp != m_states[1-m_stateNum].word_mem_sp);
    if (m_z80Mode) {
        drawHexWord(baseX, baseY++, m_states[m_stateNum].ix, m_states[m_stateNum].ix != m_states[1-m_stateNum].ix);
        drawHexWord(baseX, baseY, m_states[m_stateNum].iy, m_states[m_stateNum].iy != m_states[1-m_stateNum].iy);
    }

    baseY = m_curLayout->regMemHex.top + 1;
    baseX = m_curLayout->regMemHex.left + 12;
    int first = m_compactMode ? 9 : 7;
    int len = m_compactMode ? 10 : 14;
    drawHexSeq(baseX,  baseY++, m_states[m_stateNum].mem_hl + first, m_states[1-m_stateNum].mem_hl + first, len);
    drawHexSeq(baseX, baseY++, m_states[m_stateNum].mem_bc + first, m_states[1-m_stateNum].mem_bc + first, len);
    drawHexSeq(baseX, baseY++, m_states[m_stateNum].mem_de + first, m_states[1-m_stateNum].mem_de + first, len);
    drawHexSeq(baseX, baseY++, m_states[m_stateNum].mem_sp + first, m_states[1-m_stateNum].mem_sp + first, len);
    drawHexSeq(baseX, baseY++, m_states[m_stateNum].mem_mem_hl + first, m_states[1-m_stateNum].mem_mem_hl + first, len);
    drawHexSeq(baseX, baseY++, m_states[m_stateNum].mem_mem_sp + first, m_states[1-m_stateNum].mem_mem_sp + first, len);
    if (m_z80Mode) {
        drawHexSeq(baseX, baseY++, m_states[m_stateNum].mem_ix + first, m_states[1-m_stateNum].mem_ix + first, len);
        drawHexSeq(baseX, baseY, m_states[m_stateNum].mem_iy + first, m_states[1-m_stateNum].mem_iy + first, len);
    }
    baseY = m_curLayout->regMemHex.top + 1;
    baseX += m_compactMode ? 12 : 18;
    highlight(baseX, baseY, 3, m_z80Mode ? 8 : 6, true);
    highlight(baseX + 1, baseY, 3, m_z80Mode ? 8 : 6, true);

    baseY = m_curLayout->regMemSmb.top + 1;
    baseX = m_curLayout->regMemSmb.left + 12;
    drawSymSeq(baseX, baseY++, m_states[m_stateNum].mem_hl, m_states[1-m_stateNum].mem_hl, 29);
    drawSymSeq(baseX, baseY++, m_states[m_stateNum].mem_bc, m_states[1-m_stateNum].mem_bc, 29);
    drawSymSeq(baseX, baseY++, m_states[m_stateNum].mem_de, m_states[1-m_stateNum].mem_de, 29);
    drawSymSeq(baseX, baseY++, m_states[m_stateNum].mem_sp, m_states[1-m_stateNum].mem_sp, 29);
    drawSymSeq(baseX, baseY++, m_states[m_stateNum].mem_mem_hl, m_states[1-m_stateNum].mem_mem_hl, 29);
    if (!m_compactMode)
        drawSymSeq(baseX, baseY++, m_states[m_stateNum].mem_mem_sp, m_states[1-m_stateNum].mem_mem_sp, 29);
    if (m_z80Mode) {
        drawSymSeq(baseX, baseY++, m_states[m_stateNum].mem_ix, m_states[1-m_stateNum].mem_ix, 29);
        drawSymSeq(baseX, baseY++, m_states[m_stateNum].mem_iy, m_states[1-m_stateNum].mem_iy, 29);
    }
    baseY = m_curLayout->regMemHex.top + 1;
    baseX += m_compactMode ? 12 : 18;
    len = m_z80Mode ? 8 : (m_compactMode ? 5 : 6);
    highlight(m_curLayout->regMemSmb.left + 25, m_curLayout->regMemSmb.top + 1, 3, len, true);

    regsDraw();
    flagsDraw();
    dumpDraw();
    codeDraw();
    bpointsDraw();
    inputDraw();
}


void DebugWindow::drawHintBar()
{
    //clearBlock(0, m_curLayout->rows - 1, m_curLayout->cols - 1, m_curLayout->rows - 1, 1);

    string s;

    switch (m_mode) {
        case AM_CODE:
            if (!m_swapF5F9)
                s = "C/D/R/F/B,Tab,Esc-Section A-Addr F4-Here F5-B/p F7-Step F8-Over F9-Run";
            else
                s = "C/D/R/F/B,Tab,Esc-Section A-Addr F4-Here F5-Run F7-Step F8-Over F9-B/p";
            if (!m_compactMode)
                s += " U-Skip";
            if (!m_z80Mode) {
                s += " Z-Mnemo";
                if (!m_compactMode)
                    s += " M-Mini";
            }
            break;
        case AM_DUMP:
            //s = "C/D/R/B,Tab,Esc-Section A-Addr F2-Edit";
            s = "C/D/R/F/B,Tab,Esc-Section A-Addr Enter/F2-Edit";
            break;
        case AM_REGS:
            //s = "C/D/R/B,Tab,Esc-Section F2-Edit";
            s = "C/D/R/F/B,Tab,Esc-Section Enter/F2-Edit";
            break;
        case AM_FLAGS:
            //s = "C/D/R/B,Tab,Esc-Section F2-Edit";
            s = "C/D/R/F/B,Tab,Esc-Section 0/1/Space-Set";
            break;
        case AM_BPOINTS:
            //s = "C/D/R/B,Tab,Esc-Section F2-Edit Enter-Goto";
            s = "C/D/R/F/B,Tab,Esc-Section Enter-Goto";
            break;
        case AM_INPUT:
            s = "Enter-Enter Esc-Cancel";
            break;
        default:
            break;
    }
    int pos =  (m_curLayout->cols - s.size()) / 2;
    setPos(pos, m_curLayout->menu.top);
    setColors(7, 5);
    for (unsigned i = 0; i < s.size(); i++) {
        string symb = s.substr(i, 1);
        if (symb == "-")
            setColors(7, 1);
        putString(symb);
        if (symb == " ")
            setColors(7, 5);
    }
}


void DebugWindow::processKey(PalKeyCode keyCode, bool isPressed)
{
    if (isPressed) {
        if (m_mode == AM_INPUT)
            inputKbdProc(keyCode);
        else
            switch (keyCode) {
                case PK_C:
                case PK_ESC:
                    m_mode = AM_CODE;
                    break;
                case PK_D:
                    m_mode = AM_DUMP;
                    break;
                case PK_R:
                    m_mode = AM_REGS;
                    break;
                case PK_F:
                    m_mode = AM_FLAGS;
                    break;
                case PK_B:
                    if (m_bpList.size() > 0)
                        m_mode = AM_BPOINTS;
                    break;
                case PK_TAB:
                    if (m_mode == AM_CODE)
                        m_mode = AM_DUMP;
                    else if (m_mode == AM_DUMP)
                        m_mode = AM_REGS;
                    else if (m_mode == AM_REGS)
                        m_mode = AM_FLAGS;
                    else // if (m_mode == AM_FLAGS)
                        m_mode = AM_CODE;
                    break;
                case PK_M:
                    if (!m_z80Mode)
                        setLayout(!m_compactMode);
                    break;
                default:
                    switch (m_mode) {
                        case AM_CODE:
                            codeKbdProc(keyCode);
                            break;
                        case AM_DUMP:
                            dumpKbdProc(keyCode);
                            break;
                        case AM_REGS:
                            regsKbdProc(keyCode);
                            break;
                        case AM_FLAGS:
                            flagsKbdProc(keyCode);
                            break;
                        case AM_BPOINTS:
                            bpointsKbdProc(keyCode);
                            break;
                        default:
                            break;
                    }
            }
    }
}


bool DebugWindow::checkRegion(const DbgRegion& rgn, int x, int y)
{
    return ((x >= rgn.left) && (x < rgn.left + rgn.width) && (y >= rgn.top) && (y < rgn.top + rgn.height));
}

void DebugWindow::mouseClick(int x, int y, PalMouseKey key)
{
    if (!translateCoords(x, y))
        return;

    x /= m_chrW;
    y /= m_chrH;

    if (m_mode == AM_INPUT)
        return;

    if (key == PM_WHEEL_UP || key == PM_WHEEL_DOWN) {
        switch (m_mode) {
        case AM_DUMP:
            dumpClick(x, y, key);
            break;
        case AM_CODE:
            codeClick(x, y, key);
            break;
        case AM_REGS:
            regsClick(x, y, key);
            break;
        case AM_FLAGS:
            flagsClick(x, y, key);
            break;
        case AM_BPOINTS:
            bpointsClick(x, y, key);
            break;
        default:
            break;
        }
        return;
    }

    if (checkRegion(m_curLayout->code, x, y)) {
        m_mode = AM_CODE;
        codeClick(x - m_curLayout->code.left, y - m_curLayout->code.top, key);
    } else if (checkRegion(m_curLayout->dump, x, y)) {
        m_mode = AM_DUMP;
        dumpClick(x - m_curLayout->dump.left, y - m_curLayout->dump.top, key);
    }
    else if (checkRegion(m_curLayout->regs, x, y)) {
        m_mode = AM_REGS;
        regsClick(x - m_curLayout->regs.left, y - m_curLayout->regs.top, key);
    } else if (checkRegion(m_curLayout->flags, x, y)) {
        m_mode = AM_FLAGS;
        flagsClick(x - m_curLayout->flags.left, y - m_curLayout->flags.top, key);
    } else if (checkRegion(m_curLayout->bpts, x, y) && (m_bpList.size() > 0)) {
        m_mode = AM_BPOINTS;
        bpointsClick(x - m_curLayout->bpts.left, y - m_curLayout->bpts.top, key);
    }
}


void DebugWindow::checkForInput()
{
    if (m_mode != AM_INPUT) {
        switch (m_inputFromMode) {
            case AM_CODE:
                codeProcessInput();
                break;
            case AM_DUMP:
                dumpProcessInput();
                return; // m_inputFromMode set in dumpProcessInput()
                //break;
            case AM_REGS:
                regsProcessInput();
                break;
            default:
                break;
        }
    m_inputFromMode = AM_NONE;
    }
}


void DebugWindow::step()
{
    /*unsigned pc = m_states[m_stateNum].pc;
    unsigned len = getInstructionLength(pc);
    m_states[m_stateNum].pc += len;
    codeGotoPc();*/
    m_cpu->debugStepRequest();
    m_isRunning = true;
    checkForCurBreakpoint();
    m_resetCpuClockFlag = false;
    g_emulation->debugRun();
}


void DebugWindow::run()
{
    m_isRunning = true;
    checkForCurBreakpoint();
    m_resetCpuClockFlag = true;
    g_emulation->debugRun();
    hide();
}


void DebugWindow::over()
{
    unsigned pc = m_states[m_stateNum].pc;
    unsigned len = getInstructionLength(pc);
    bool over = getInstructionOverFlag(pc);

    //uint8_t op = memByte(pc);
    //over = (op & 0xcf) == 0xcd /* CALL */ || (op & 0xc7) == 0xc7 /* RST */ || (op & 0xc7) == 0xc4 /* Cx */;

    if (over) {
        // если пользователь нажал F8, пока не успела выполниться предудущая F8 или F4, ничего не делаем
        if (m_tempBp)
            return;
        m_tempBp = new CodeBreakpoint(pc + len);
        m_cpu->addHook(m_tempBp);

        m_isRunning = true;
        checkForCurBreakpoint();
        m_resetCpuClockFlag = false;
        g_emulation->debugRun();
        hide();
    } else
        step();
}


void DebugWindow::skip()
{
    unsigned pc = m_states[m_stateNum].pc;
    unsigned len = getInstructionLength(pc);
    m_cpu->setPC(pc + len);
    m_isRunning = true;
    startDebug();
}


void DebugWindow::here()
{
    unsigned pc = m_codeLayout[m_codeHighlightedLine];

    // если пользователь нажал F4, пока не успела выполниться предудущая F4 или F8, ничего не делаем
    if (m_tempBp)
        return;
    m_tempBp = new CodeBreakpoint(pc);
    m_cpu->addHook(m_tempBp);

    m_isRunning = true;
    checkForCurBreakpoint();
    m_resetCpuClockFlag = false;
    g_emulation->debugRun();
    hide();
}


void DebugWindow::breakpoint()
{
    unsigned pc = m_codeLayout[m_codeHighlightedLine];

    // ищем точку останова
    bool found = false;
    BreakpointInfo bpInfo;
    list<BreakpointInfo>::iterator it;
    for (it = m_bpList.begin(); it != m_bpList.end(); it++)
        if ((*it).addr == pc) {
            found = true;
            bpInfo = *it;
            break;
        }

    if (found) {
        // нашли, удаляем
        delete bpInfo.codeBp;
        m_bpList.erase(it);
    } else {
        // не нашли
        if (m_bpList.size() >=6) // ограничение на количество точек останова
            return;
        // ставим
        CodeBreakpoint* cbp = new CodeBreakpoint(pc);
        m_cpu->addHook(cbp);
        bpInfo.addr = pc;
        bpInfo.type = BT_EXEC;
        bpInfo.codeBp = cbp;
        m_bpList.push_back(bpInfo);
    }
}


// проверяет перед выполнением, не установлена ли точка останова на текущий PC
void DebugWindow::checkForCurBreakpoint()
{
    for (auto it = m_bpList.begin(); it != m_bpList.end(); it++)
        if ((*it).type == BT_EXEC && (*it).addr == m_states[m_stateNum].pc)
            (*it).codeBp->setSkipCount(1);
}

// ######## INPUT mode methods ########

void DebugWindow::inputInit()
{
    m_inputFromMode = AM_NONE;
}

void DebugWindow::inputStart(ActiveMode fromMode, int x, int y, int nDigits, bool useInitialNumber, unsigned initialNumber)
{
    m_mode = AM_INPUT;
    m_inputFromMode = fromMode;
    m_inputXPos = x;
    m_inputYPos = y;
    m_inputNDigits = nDigits;
    if (useInitialNumber) {
        //m_inputCurPos = nDigits - 1;
        m_inputCurValue = int2Hex(initialNumber, nDigits);
    } else {
        m_inputCurPos = 0;
        m_inputCurValue = "";
    }
    m_inputCurPos = 0;
}


void DebugWindow::inputDraw()
{
    if (m_mode == AM_INPUT) {
        clearBlock(m_inputXPos, m_inputYPos, m_inputXPos + m_inputNDigits - 1, m_inputYPos, 0);
        putString(m_inputXPos, m_inputYPos, m_inputCurValue, 10, 0);
        m_cursorVisible = true;
        m_cursorXPos = m_inputXPos + m_inputCurPos;
        m_cursorYPos = m_inputYPos;
    }
}


void DebugWindow::inputKbdProc(PalKeyCode keyCode)
{
    switch (keyCode) {
        case PK_0:
            inputDigit('0');
            break;
        case PK_1:
            inputDigit('1');
            break;
        case PK_2:
            inputDigit('2');
            break;
        case PK_3:
            inputDigit('3');
            break;
        case PK_4:
            inputDigit('4');
            break;
        case PK_5:
            inputDigit('5');
            break;
        case PK_6:
            inputDigit('6');
            break;
        case PK_7:
            inputDigit('7');
            break;
        case PK_8:
            inputDigit('8');
            break;
        case PK_9:
            inputDigit('9');
            break;
        case PK_A:
            inputDigit('A');
            break;
        case PK_B:
            inputDigit('B');
            break;
        case PK_C:
            inputDigit('C');
            break;
        case PK_D:
            inputDigit('D');
            break;
        case PK_E:
            inputDigit('E');
            break;
        case PK_F:
            inputDigit('F');
            break;
        case PK_LEFT:
            if (m_inputCurPos != 0)
                --m_inputCurPos;
            break;
        case PK_RIGHT:
            if (m_inputCurPos < m_inputCurValue.size() && m_inputCurPos < m_inputNDigits - 1)
                ++m_inputCurPos;
            break;
        case PK_DEL:
            if (m_inputCurValue.size() > 0 && m_inputCurPos != m_inputCurValue.size())
                m_inputCurValue.erase(m_inputCurPos, 1);
            break;
        case PK_BSP:
            if (m_inputCurPos > 0)
                m_inputCurValue.erase(--m_inputCurPos, 1);
            break;
        case PK_ESC:
            m_cursorVisible = false;
            m_mode = m_inputFromMode;
            m_inputFromMode = AM_NONE; // чтобы игнорировалось влзвращаемое значение
            break;
        case PK_ENTER:
            m_cursorVisible = false;
            m_mode = m_inputFromMode;
            m_inputReturnValue = hex2Int(m_inputCurValue);
            break;
        default:
            break;
    }
}


void DebugWindow::inputDigit(char digit)
{
    if (m_inputCurValue.size() <= m_inputCurPos)
        m_inputCurValue += digit;
    else
        m_inputCurValue[m_inputCurPos] = digit;
    if (m_inputCurPos < m_inputNDigits - 1)
        ++m_inputCurPos;
}


// ######## CODE section methods ########

// первичная инициализация секции
void DebugWindow::codeInit()
{
    m_codeCurPcLine = m_codeDefPcLine;
    m_codeHighlightedLine = m_codeCurPcLine;
    m_codeLayout[m_codeDefPcLine] = m_states[m_stateNum].pc;
    codeFillLayout(m_codeDefPcLine);
}

// переход на текущий PC в листинге
// вызывается после каждого очередного шага в отладчике
void DebugWindow::codeGotoPc()
{
    unsigned pc = m_states[m_stateNum].pc;
    if (m_codeCurPcLine >=0 && m_codeCurPcLine < m_codeMaxPcLine && m_codeLayout[m_codeCurPcLine + 1] == pc)
        ++m_codeCurPcLine;
    else
        m_codeCurPcLine = m_codeDefPcLine;
    m_codeLayout[m_codeCurPcLine] = pc;
    m_codeHighlightedLine = m_codeCurPcLine;
    codeFillLayout(m_codeCurPcLine);
}


// заполнение адресов строк относительно строки lineNum, у которой уже корректно выставлен адрес
void DebugWindow::codeFillLayout(int lineNum)
{
    unsigned len;
    uint16_t addr = m_codeLayout[lineNum];

    for (int i = lineNum; i < m_curLayout->code.height; i++) {
        m_codeLayout[i] = addr;
        len = getInstructionLength(addr);
        addr += len;
    }


    addr = m_codeLayout[lineNum] - lineNum * 4 - 5;
    int skip = 0;
    while (int16_t(m_codeLayout[lineNum] - addr) > 0) {
        addr += getInstructionLength(addr);
        ++skip;
    }
    skip -= lineNum;
    addr = m_codeLayout[lineNum] - lineNum * 4 - 5;
    int n = 0;
    while (int16_t(m_codeLayout[lineNum] - addr) > 0 ) {
        if (skip != 0)
            --skip;
        else {
            m_codeLayout[n++] = addr;
        }
        addr += getInstructionLength(addr);
    }
}


// перерисовка секции кода
void DebugWindow::codeDraw()
{
    //clearBlock(m_curLayout->code.left, m_curLayout->code.top, m_curLayout->code.left + m_curLayout->code.width - 1, m_curLayout->code.top + m_curLayout->code.height - 1, 1);

    unsigned pc = m_states[m_stateNum].pc;
    int pcLine = -1;
    for (int i = 0; i < m_curLayout->code.height; i++) {
        if (m_codeLayout[i] == pc)
            pcLine = i;
        int len = getInstructionLength(m_codeLayout[i]);
        string cmd = getInstructionMnemonic(m_codeLayout[i]);
        if (i != m_curLayout->code.height - 1 && (m_codeLayout[i] + len) != m_codeLayout[i + 1]) {
            cmd = "DB " + int2Hex(memByte(m_codeLayout[i]), 2) + "h";
            len = 1;
        }
        string s = int2Hex(m_codeLayout[i], 4) + ": ";
        for (int j = 0; j < len; j++)
            s = s + int2Hex(memByte(m_codeLayout[i] + j), 2) + " ";
        while (s.size() < 17)
            s += " ";
        s += cmd;

        setColors(11, 1);
        putString(m_curLayout->code.left + 3, m_curLayout->code.top + i, s.substr(0, 5));
        setColors(15, 1);
        putString(m_curLayout->code.left + 8, m_curLayout->code.top + i, s.substr(5));
    }

    // указатель PC
    if (pcLine >= 0)
        putChars(m_curLayout->code.left + 2, m_curLayout->code.top + pcLine, 0x10, 1);

    setColors(12, 1);
    for (auto it = m_bpList.begin(); it != m_bpList.end(); it++) {
        for(int i = 0; i < m_curLayout->code.height; i++)
            if (m_codeLayout[i] == (*it).addr && (*it).type == BT_EXEC)
                putChars(m_curLayout->code.left + 1, m_curLayout->code.top + i, 0x0f, 1);
    }

    // подсветка текущей строки
    if (m_mode == AM_CODE)
        highlight(m_curLayout->code.left, m_curLayout->code.top + m_codeHighlightedLine, 3, m_compactMode ? 34 : 37);
    //else
        //highlight(m_curLayout->code.left, m_curLayout->code.top + m_codeHighlightedLine, 0, 34);
}


// клавиатурный обработчик секции кода
void DebugWindow::codeKbdProc(PalKeyCode keyCode)
{
    int i;

    switch (keyCode) {
        case PK_UP:
            if (m_codeHighlightedLine > 0)
                // просто перемещаем подствеку вверх
                m_codeHighlightedLine--;
            else if (m_codeCurPcLine < m_curLayout->code.height - 1 && m_codeCurPcLine >= 0) {
                // PC виден в листинге и не на последней строке, сдвигаем листинг вверх
                m_codeCurPcLine++;
                m_codeLayout[m_codeCurPcLine] = m_codeLayout[m_codeCurPcLine - 1];
                codeFillLayout(m_codeCurPcLine);
            } else if (m_codeCurPcLine == m_curLayout->code.height - 1) {
                // PC на последней строке и исчезает в результате сдвига
                m_codeCurPcLine = -1;
                m_codeLayout[m_codeDefPcLine] = m_codeLayout[m_codeDefPcLine - 1];
                codeFillLayout(m_codeDefPcLine);
            } else {
                // PC отсутствует в листинге, может появиться на первой строке в результате сдвига
                m_codeLayout[m_codeDefPcLine] = m_codeLayout[m_codeDefPcLine - 1];
                codeFillLayout(m_codeDefPcLine);
                if (m_codeLayout[0] <= m_states[m_stateNum].pc && m_codeLayout[1] > m_states[m_stateNum].pc) {
                    m_codeCurPcLine = 0;
                    codeFillLayout(0);
                }
            }
            //codeDraw();
            break;
        case PK_DOWN:
            if (m_codeHighlightedLine < m_curLayout->code.height - 1)
                // просто перемещаем подствеку вниз
                m_codeHighlightedLine++;
            else if (m_codeCurPcLine > 0) {
                // PC виден в листинге и не на первой строке, сдвигаем листинг вниз
                m_codeCurPcLine--;
                m_codeLayout[m_codeCurPcLine] = m_codeLayout[m_codeCurPcLine + 1];
                codeFillLayout(m_codeCurPcLine);
            } else  if (m_codeCurPcLine == 0) {
                // PC на первой строке и исчезает в результате сдвига
                m_codeCurPcLine = -1;
                m_codeLayout[0] = m_codeLayout[1];
                codeFillLayout(0);
            } else {
                // PC отсутствует в листинге, может появиться на последней строке в результате сдвига
                m_codeLayout[0] = m_codeLayout[1];
                codeFillLayout(0);
                if (m_codeLayout[m_curLayout->code.height - 2] <= m_states[m_stateNum].pc && m_codeLayout[m_curLayout->code.height - 1] > m_states[m_stateNum].pc) {
                    m_codeCurPcLine = m_curLayout->code.height - 1;
                    codeFillLayout(m_curLayout->code.height - 1);
                }
            }
            //codeDraw();
            break;
        case PK_PGUP:
            for (i = 0; i< 16; i++)
                codeKbdProc(PK_UP);
            break;
        case PK_PGDN:
            for (i = 0; i< 16; i++)
                codeKbdProc(PK_DOWN);
            break;
        case PK_F4:
            here();
            break;
        case PK_F5:
            m_swapF5F9 ? run() : breakpoint();
            break;
        case PK_F7:
            step();
            break;
        case PK_F8:
            over();
            break;
        case PK_F9:
            m_swapF5F9 ? breakpoint() : run();
            break;
        case PK_U:
            skip();
            break;
        case PK_A:
            inputStart(m_mode, m_curLayout->code.left + 3, 1 + m_codeHighlightedLine, 4, true, m_codeLayout[m_codeHighlightedLine]);
            //inputStart(m_mode, 4, 24, 4, false);
            break;
        case PK_Z:
            m_z80Mnemonics = !m_z80Mnemonics;
            break;
        default:
            break;
    }
}


// обработчик событий мыши секции кода
void DebugWindow::codeClick(int x, int y, PalMouseKey key)
{
    switch (key) {
    case PM_LEFT_CLICK:
        m_codeHighlightedLine = y;
        break;
    case PM_LEFT_DBLCLICK:
        if (x < 3)
            breakpoint();
        break;
    case PM_WHEEL_UP:
        codeKbdProc(PK_UP);
        break;
    case PM_WHEEL_DOWN:
        codeKbdProc(PK_DOWN);
        break;
    }
}


void DebugWindow::codeGotoAddr(uint16_t addr)
{
    m_codeLayout[m_codeHighlightedLine] = addr;
    codeFillLayout(m_codeHighlightedLine);
}


void DebugWindow::codeProcessInput()
{
    codeGotoAddr(m_inputReturnValue);
}



// ######## DUMP section methods ########

void DebugWindow::dumpInit()
{
    m_dumpCurAddr = 0x0000;
    m_dumpCurStartAddr = m_dumpCurAddr;
}


// отрисовка дампа
void DebugWindow::dumpDraw()
{
    //clearBlock(m_curLayout->dump.left + 3, m_curLayout->dump.top + 1, m_curLayout->dump.left + m_curLayout->dump.width - 5, m_curLayout->dump.top + m_curLayout->dump.height - 1, 1);

    uint8_t buf[16];

    for (int i = 0; i < m_curLayout->dump.height - 1; i++) {
        setColors(11, 1);
        putString(m_curLayout->dump.left + 3, m_curLayout->dump.top + 1 + i, int2Hex((m_dumpCurStartAddr + i * 16) & 0xFFFF, 4));
        putString(":");
        for (int j = 0; j < 16; j++)
            buf[j] = memByte(m_dumpCurStartAddr + i * 16 + j);
        drawHexSeq(m_curLayout->dump.left + 9, m_curLayout->dump.top + 1 + i, buf, buf, 16);
        drawSymSeq(m_curLayout->dump.left + 59, m_curLayout->dump.top + 1 + i, buf, buf, 16);
    }
    if (m_mode == AM_DUMP) {
        int ofs = (m_dumpCurAddr - m_dumpCurStartAddr) & 0xffff;
        int line = ofs / 16;
        int bt = ofs % 16;
        highlight(m_curLayout->dump.left + 8 + bt * 3, m_curLayout->dump.top + 1 + line, 3, 4);
    }
}


// клавиатурный обработчик секции дампа
void DebugWindow::dumpKbdProc(PalKeyCode keyCode)
{
    switch (keyCode) {
        case PK_UP:
            m_dumpCurAddr -= 16;
            break;
        case PK_DOWN:
            m_dumpCurAddr += 16;
            break;
        case PK_LEFT:
            m_dumpCurAddr--;
            break;
        case PK_RIGHT:
            m_dumpCurAddr++;
            break;
        case PK_PGUP:
            m_dumpCurAddr -= 0x80;
            m_dumpCurStartAddr -= 0x80;
            break;
        case PK_PGDN:
            m_dumpCurAddr += 0x80;
            m_dumpCurStartAddr += 0x80;
            break;
        case PK_A:
            m_dumpInputAddr = true;
            inputStart(m_mode, m_curLayout->dump.left + 3, m_curLayout->dump.top + 1 + uint16_t(m_dumpCurAddr - m_dumpCurStartAddr) / 16, 4, true, m_dumpCurAddr & 0xFFF0);
            break;
        case PK_ENTER:
        case PK_F2: {
            m_dumpInputAddr = false;
            int ofs = (m_dumpCurAddr - m_dumpCurStartAddr) & 0xffff;
            int line = ofs / 16;
            int bt = ofs % 16;
            inputStart(m_mode, m_curLayout->dump.left + 8 + bt * 3 + 1, m_curLayout->dump.top + 1 + line, 2, true, memByte(m_dumpCurAddr));
            break;
        }
        default:
            break;
    }
    if (uint16_t(m_dumpCurAddr - m_dumpCurStartAddr) & 0x8000)
        m_dumpCurStartAddr = m_dumpCurStartAddr - 16;
    if (uint16_t(m_dumpCurAddr - m_dumpCurStartAddr) >= (m_curLayout->dump.height - 1) * 16)
        m_dumpCurStartAddr = m_dumpCurStartAddr + 16;
}


void DebugWindow::dumpProcessInput()
{
    m_inputFromMode = AM_NONE;
    if (m_dumpInputAddr) {
        m_dumpCurStartAddr = uint16_t((m_inputReturnValue & 0xFFF0) - uint16_t((m_dumpCurAddr & 0xFFF0) - m_dumpCurStartAddr));
        m_dumpCurAddr = m_inputReturnValue;
    } else {
        writeByte(m_dumpCurAddr, m_inputReturnValue);
        dumpKbdProc(PK_RIGHT); // переходим к редактированию следующего байта
        m_inputFromMode = AM_DUMP;
        dumpKbdProc(PK_F2);
    }
}


// обработчик событий мыши секции дампа
void DebugWindow::dumpClick(int x, int y, PalMouseKey key)
{
    switch (key) {
    case PM_LEFT_CLICK:
        if (y == 0)
            return;

        m_dumpCurAddr = m_dumpCurStartAddr + (y - 1) * 16;

        if ((x >= 8) && (x <= 8 + 16 * 3))
             m_dumpCurAddr += (x - 8) / 3;

        break;
    case PM_LEFT_DBLCLICK:
        if ((x >= 8) && (x <= 8 + 16 * 3)) {
            dumpKbdProc(PK_F2);
        } else if ((x >= 3) && (x <= 6)) {
            dumpKbdProc(PK_A);
        }
        break;
    case PM_WHEEL_UP:
        dumpKbdProc(PK_UP);
        break;
    case PM_WHEEL_DOWN:
        dumpKbdProc(PK_DOWN);
        break;
    }
}


// ######## REGS section methods ########

// инициализация секции регистров
void DebugWindow::regsInit()
{
    m_regsCurReg = 0;
}

// отрисовка регистров
void DebugWindow::regsDraw()
{
    //clearBlock(41, 1, 46, 6, 1);

    int baseY = m_curLayout->regs.top;
    int baseX = m_curLayout->regs.left + 6;
    drawHexByte(baseX,     baseY,   m_states[m_stateNum].a, m_states[m_stateNum].a != m_states[1-m_stateNum].a);
    drawHexByte(baseX + 2, baseY++, m_states[m_stateNum].f, m_states[m_stateNum].f != m_states[1-m_stateNum].f);
    drawHexByte(baseX,     baseY,   m_states[m_stateNum].b, m_states[m_stateNum].b != m_states[1-m_stateNum].b);
    drawHexByte(baseX + 2, baseY++, m_states[m_stateNum].c, m_states[m_stateNum].c != m_states[1-m_stateNum].c);
    drawHexByte(baseX,     baseY,   m_states[m_stateNum].d, m_states[m_stateNum].d != m_states[1-m_stateNum].d);
    drawHexByte(baseX + 2, baseY++, m_states[m_stateNum].e, m_states[m_stateNum].e != m_states[1-m_stateNum].e);
    drawHexByte(baseX,     baseY,   m_states[m_stateNum].h, m_states[m_stateNum].h != m_states[1-m_stateNum].h);
    drawHexByte(baseX + 2, baseY++, m_states[m_stateNum].l, m_states[m_stateNum].l != m_states[1-m_stateNum].l);
    drawHexWord(baseX,     baseY++, m_states[m_stateNum].sp, m_states[m_stateNum].sp != m_states[1-m_stateNum].sp);
    drawHexWord(baseX,     baseY,   m_states[m_stateNum].pc, m_states[m_stateNum].pc != m_states[1-m_stateNum].pc);

    if (m_z80Mode) {
        baseY = m_curLayout->regs.top;
        baseX = m_curLayout->regs.left + 18;
        drawHexByte(baseX,     baseY,   m_states[m_stateNum].a2, m_states[m_stateNum].a2 != m_states[1-m_stateNum].a2);
        drawHexByte(baseX + 2, baseY++, m_states[m_stateNum].f2, m_states[m_stateNum].f2 != m_states[1-m_stateNum].f2);
        drawHexByte(baseX,     baseY,   m_states[m_stateNum].b2, m_states[m_stateNum].b2 != m_states[1-m_stateNum].b2);
        drawHexByte(baseX + 2, baseY++, m_states[m_stateNum].c2, m_states[m_stateNum].c2 != m_states[1-m_stateNum].c2);
        drawHexByte(baseX,     baseY,   m_states[m_stateNum].d2, m_states[m_stateNum].d2 != m_states[1-m_stateNum].d2);
        drawHexByte(baseX + 2, baseY++, m_states[m_stateNum].e2, m_states[m_stateNum].e2 != m_states[1-m_stateNum].e2);
        drawHexByte(baseX,     baseY,   m_states[m_stateNum].h2, m_states[m_stateNum].h2 != m_states[1-m_stateNum].h2);
        drawHexByte(baseX + 2, baseY++, m_states[m_stateNum].l2, m_states[m_stateNum].l2 != m_states[1-m_stateNum].l2);
        drawHexWord(baseX,     baseY++, m_states[m_stateNum].ix, m_states[m_stateNum].ix != m_states[1-m_stateNum].ix);
        drawHexWord(baseX,     baseY,   m_states[m_stateNum].iy, m_states[m_stateNum].iy != m_states[1-m_stateNum].iy);

        baseY = m_curLayout->regs.top + 7;
        baseX = m_curLayout->regs.left + 6;
        drawHexByte(baseX,     baseY,   m_states[m_stateNum].r, m_states[m_stateNum].r != m_states[1-m_stateNum].r);
        baseX += 12;
        drawHexByte(baseX,     baseY,   m_states[m_stateNum].im, m_states[m_stateNum].im != m_states[1-m_stateNum].im);
    }

    if (m_mode == AM_REGS)
        highlight(m_curLayout->regs.left + (m_z80Mode && m_regsCurReg > 5 ? 17 : 5), 1 + m_regsCurReg % 6, 3, 6);
}

uint16_t DebugWindow::regsGetCurRegValue()
{
    switch (m_regsCurReg) {
        case 0:
            return m_states[m_stateNum].a << 8 | m_states[m_stateNum].f;
        case 1:
            return m_states[m_stateNum].b << 8 | m_states[m_stateNum].c;
        case 2:
            return m_states[m_stateNum].d << 8 | m_states[m_stateNum].e;
        case 3:
            return m_states[m_stateNum].h << 8 | m_states[m_stateNum].l;
        case 4:
            return m_states[m_stateNum].sp;
        case 5:
            return m_states[m_stateNum].pc;
        case 6:
            return m_states[m_stateNum].a2 << 8 | m_states[m_stateNum].f2;
        case 7:
            return m_states[m_stateNum].b2 << 8 | m_states[m_stateNum].c2;
        case 8:
            return m_states[m_stateNum].d2 << 8 | m_states[m_stateNum].e2;
        case 9:
            return m_states[m_stateNum].h2 << 8 | m_states[m_stateNum].l2;
        case 10:
            return m_states[m_stateNum].ix;
        case 11:
            return m_states[m_stateNum].iy;
        default:
            return 0;
    }
}


void DebugWindow::regsSetCurRegValue(uint16_t value)
{
    switch (m_regsCurReg) {
        case 0:
            m_cpu->setAF(value);
            break;
        case 1:
            m_cpu->setBC(value);
            break;
        case 2:
            m_cpu->setDE(value);
            break;
        case 3:
            m_cpu->setHL(value);
            break;
        case 4:
            m_cpu->setSP(value);
            break;
        case 5:
            m_cpu->setPC(value);
            break;
        case 6:
            m_z80cpu->setAF2(value);
            break;
        case 7:
            m_z80cpu->setBC2(value);
            break;
        case 8:
            m_z80cpu->setDE2(value);
            break;
        case 9:
            m_z80cpu->setHL2(value);
            break;
        case 10:
            m_z80cpu->setIX(value);
            break;
        case 11:
            m_z80cpu->setIY(value);
            break;
        default:
            break;
    }
    fillCpuStatus();
}


// клавиатурный обработчик секции регистров
void DebugWindow::regsKbdProc(PalKeyCode keyCode)
{
    switch (keyCode) {
        case PK_UP:
            if (m_regsCurReg > 0)
                --m_regsCurReg;
            break;
        case PK_DOWN:
            if (m_regsCurReg < (m_z80Mode ? 11 : 5))
                ++m_regsCurReg;
            break;
        case PK_RIGHT:
        case PK_LEFT:
            if (!m_z80Mode)
                break;
            m_regsCurReg += 6;
            m_regsCurReg %= 12;
            break;
        case PK_PGUP:
            m_regsCurReg = 0;
            break;
        case PK_PGDN:
            m_regsCurReg = m_z80Mode ? 11 : 5;
            break;
        case PK_ENTER:
        case PK_F2:
            inputStart(m_mode, m_curLayout->regs.left + (m_z80Mode && m_regsCurReg > 5 ? 17 : 5) + 1, 1 + m_regsCurReg % 6, 4, true, regsGetCurRegValue());
            break;
        default:
            break;
    }
}


// обработчик событий мыши секции регистров
void DebugWindow::regsClick(int x, int y, PalMouseKey key)
{
    if (y > 5)
        return;

    switch (key) {
    case PM_LEFT_CLICK:
        if ((x >= 5) && (x <= 10))
            m_regsCurReg = y;
        else if (m_z80Mode && (x >= 17) && (x <= 22))
            m_regsCurReg = y + 6;
        break;
    case PM_LEFT_DBLCLICK:
        regsKbdProc(PK_F2);
        break;
    case PM_WHEEL_UP:
        regsKbdProc(PK_UP);
        break;
    case PM_WHEEL_DOWN:
        regsKbdProc(PK_DOWN);
        break;
    }
}


void DebugWindow::regsProcessInput()
{
    regsSetCurRegValue(m_inputReturnValue);
}



// ######## FLAGS section methods ########

// инициализация секции флагов
void DebugWindow::flagsInit()
{
    m_flagsCurFlag = 0;
}


// отрисовка флагов
void DebugWindow::flagsDraw()
{
    int baseY = m_curLayout->regs.top;
    int baseX = m_curLayout->flags.left + 5;
    if (!m_compactMode)
        baseX++;
    drawInt(baseX, baseY++, m_states[m_stateNum].fl_c, m_states[m_stateNum].fl_c != m_states[1-m_stateNum].fl_c);
    drawInt(baseX, baseY++, m_states[m_stateNum].fl_z, m_states[m_stateNum].fl_z != m_states[1-m_stateNum].fl_z);
    drawInt(baseX, baseY++, m_states[m_stateNum].fl_p, m_states[m_stateNum].fl_p != m_states[1-m_stateNum].fl_p);
    drawInt(baseX, baseY++, m_states[m_stateNum].fl_m, m_states[m_stateNum].fl_m != m_states[1-m_stateNum].fl_m);
    drawInt(baseX, baseY++, m_states[m_stateNum].fl_ac, m_states[m_stateNum].fl_ac != m_states[1-m_stateNum].fl_ac);
    if (m_z80Mode)
        drawInt(baseX, baseY++, m_states[m_stateNum].fl_n, m_states[m_stateNum].fl_n != m_states[1-m_stateNum].fl_n);
    if (!m_compactMode)
        drawInt(baseX, baseY + 1, m_states[m_stateNum].iff, m_states[m_stateNum].iff != m_states[1-m_stateNum].iff);

    if (m_mode == AM_FLAGS)
        highlight(m_curLayout->flags.left + 5, 1 + m_flagsCurFlag, 3, 3);
}


// клавиатурный обработчик секции флагов
void DebugWindow::flagsKbdProc(PalKeyCode keyCode)
{
    uint16_t af = m_states[m_stateNum].a << 8 | m_states[m_stateNum].f;
    int bitNo = m_flagsBits[m_flagsCurFlag];

    switch (keyCode) {
        case PK_UP:
            if (m_flagsCurFlag > 0)
                --m_flagsCurFlag;
            break;
        case PK_DOWN:
            if (m_flagsCurFlag < (m_z80Mode ? 5 : 4))
                ++m_flagsCurFlag;
            break;
        case PK_PGUP:
        case PK_LEFT:
            m_flagsCurFlag = 0;
            break;
        case PK_PGDN:
        case PK_RIGHT:
            m_flagsCurFlag = m_z80Mode ? 5 : 4;
            break;
        case PK_1:
            af |= 1 << bitNo;
            m_cpu->setAF(af);
            fillCpuStatus();
            break;
        case PK_0:
            af &= ~((uint16_t)1 << bitNo);
            m_cpu->setAF(af);
            fillCpuStatus();
            break;
        case PK_SPACE:
            af ^= 1 << bitNo;
            m_cpu->setAF(af);
            fillCpuStatus();
            break;
        default:
            break;
    }
}


// обработчик событий мыши секции флагов
void DebugWindow::flagsClick(int x, int y, PalMouseKey key)
{
    if ((y > 5) || ((y == 5) && !m_z80Mode))
        return;

    switch (key) {
    case PM_LEFT_CLICK:
        if ((x >= 5) && (x <= 7) && ((y < 5) || m_z80Mode))
            m_flagsCurFlag = y;
        break;
    case PM_LEFT_DBLCLICK:
        flagsKbdProc(PK_SPACE);
        break;
    case PM_WHEEL_UP:
        flagsKbdProc(PK_UP);
        break;
    case PM_WHEEL_DOWN:
        flagsKbdProc(PK_DOWN);
        break;
    }
}



// ######## BREAKPOINTS section methods ########

void DebugWindow::bpointsDraw()
{
    //clearBlock(m_curLayout->bpts.left, m_curLayout->bpts.top, m_curLayout->bpts.left + m_curLayout->bpts.width - 1, m_curLayout->bpts.top + m_curLayout->bpts.height - 1, 1);

    if (m_bpList.size() == 0) {
        setColors(10, 1);
        putString(m_curLayout->bpts.left + 1, m_curLayout->bpts.top + 1, "Emu80");
        putString(m_curLayout->bpts.left + 2, m_curLayout->bpts.top + 2, "v.4");
    } else {
        int i = 0;
        for (auto it = m_bpList.begin(); it != m_bpList.end(); it++, i++) {
            setColors(12, 1);
            putString(m_curLayout->bpts.left + 2 , i + m_curLayout->bpts.top, int2Hex((*it).addr, 4));
            setColors(7, 1);
            putString(m_curLayout->bpts.left + 1, i + m_curLayout->bpts.top, "x");
        }
    }

    if (m_mode == AM_BPOINTS)
        highlight(m_curLayout->bpts.left, m_curLayout->bpts.top + m_curBpoint, 3, 7);
}


// клавиатурный обработчик секции точек останова
void DebugWindow::bpointsKbdProc(PalKeyCode keyCode)
{
    switch (keyCode) {
        case PK_UP:
            if (m_curBpoint > 0)
                --m_curBpoint;
            break;
        case PK_DOWN:
            if (m_curBpoint < m_bpList.size() - 1)
                ++m_curBpoint;
            break;
        case PK_PGUP:
            m_curBpoint = 0;
            break;
        case PK_PGDN:
            m_curBpoint = m_bpList.size() - 1;
            break;
        case PK_ENTER: {
            auto it = m_bpList.begin();
            advance(it, m_curBpoint);
            codeGotoAddr((*it).addr);
            codeGotoAddr((*it).addr);
            m_mode = AM_CODE;
            break; }
        default:
            break;
    }
}


// обработчик событий мыши секции точек останова
void DebugWindow::bpointsClick(int x, int y, PalMouseKey key)
{
    if (y >= m_bpList.size())
        return;

    switch (key) {
    case PM_LEFT_CLICK:
        m_curBpoint = y;
        break;
    case PM_LEFT_DBLCLICK:
        bpointsKbdProc(PK_ENTER);
        break;
    case PM_WHEEL_UP:
        bpointsKbdProc(PK_UP);
        break;
    case PM_WHEEL_DOWN:
        bpointsKbdProc(PK_DOWN);
        break;
    }
}



//##### Code Breakpoint class ####

CodeBreakpoint::~CodeBreakpoint()
{
    if (m_cpu)
        m_cpu->removeHook(this);
}


bool CodeBreakpoint::hookProc()
{
    //m_isEnabled = false;
    if (m_skipCount > 0) {
        --m_skipCount;
        return false;
    }
    g_emulation->debugRequest(m_cpu);
    return true;
}
