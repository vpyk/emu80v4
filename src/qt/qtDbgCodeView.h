/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2026
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

#ifndef QTDBGCODEVIEW_H
#define QTDBGCODEVIEW_H

#include <map>
#include <vector>

#include <QAbstractScrollArea>
#include <QString>

class DebugSession;


// Disassembler view of the debugger window: breakpoint column, PC marker, address,
// instruction bytes and mnemonic. Lines are instructions of variable length, so the
// view is addressed by the address of its top line.
//
// Memory is read only while the emulation is stopped; while it runs the view keeps
// the lines of the last stop, shows them as outdated and doesn't scroll.
class DbgCodeView : public QAbstractScrollArea
{
    Q_OBJECT

public:
    DbgCodeView(DebugSession* session, QWidget* parent = nullptr);

    uint16_t getCursorAddr() {return m_cursorAddr;}

public slots:
    void onStateChanged(bool newStop);
    void onBreakpointsChanged();
    void onRunStateChanged(bool running);

    void toggleBreakpoint();    // at the cursor
    void setPcToCursor();
    void gotoPc();
    void gotoAddress();         // asks for the address

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    struct Line {
        uint16_t addr;
        int length;
        QString bytes;
        QString text;
    };

    struct BpMark {
        bool enabled;
        bool external; // set by MCP
    };

    DebugSession* m_session;

    uint16_t m_topAddr = 0;
    uint16_t m_cursorAddr = 0;
    uint16_t m_pc = 0;
    bool m_running = true;
    bool m_hasState = false; // stopped at least once: the lines are valid
    bool m_showPcPending = false; // PC is to be shown when the view gets its size
    std::vector<Line> m_lines;
    std::map<uint16_t, BpMark> m_breakpoints; // exec breakpoints by address

    int m_charWidth = 8;
    int m_lineHeight = 16;
    int m_ascent = 12;

    void updateFontMetrics();
    int visibleLines();     // fully visible
    void buildLines();
    int lineIndex(uint16_t addr); // index in m_lines among the fully visible lines, -1 if none
    uint16_t nextInstrAddr(uint16_t addr);
    uint16_t prevInstrAddr(uint16_t addr);
    void setTop(uint16_t addr);
    void showAddr(uint16_t addr, bool center); // scrolls to make the address visible
    void moveCursor(int lines);
    void syncScrollBar();
    void showPendingPc();
    bool canNavigate() {return !m_running && m_hasState;}
};


#endif // QTDBGCODEVIEW_H
