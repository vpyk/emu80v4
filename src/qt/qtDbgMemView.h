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

#ifndef QTDBGMEMVIEW_H
#define QTDBGMEMVIEW_H

#include <vector>

#include <QAbstractScrollArea>
#include <QWidget>

class QAction;
class QComboBox;
class QLineEdit;
class DebugSession;


// Hex dump of one of the debugger memories (DebugSession::getMemories()): address,
// 16 (or 8 in a narrow view) bytes and their characters in the platform's code page.
// Bytes are edited by typing hex digits at the cursor. Bytes changed since the
// previous stop are highlighted (within the part shown at that stop).
//
// Memory is read only while the emulation is stopped; while it runs the view keeps
// the data of the last stop, shows it as outdated and doesn't scroll.
class DbgMemView : public QAbstractScrollArea
{
    Q_OBJECT

public:
    DbgMemView(DebugSession* session, QWidget* parent = nullptr);

    void setMemory(int memory);
    int getMemory() {return m_memory;}
    unsigned getCursorAddr() {return m_cursorAddr;}
    void gotoAddr(unsigned addr);

    // 16 bytes per line, 12 lines
    QSize sizeHint() const override;

public slots:
    void onStateChanged(bool newStop);
    void onRunStateChanged(bool running);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;
    void changeEvent(QEvent* event) override;

private:
    DebugSession* m_session;
    int m_memory = 0;
    unsigned m_size = 0x10000;
    int m_addrDigits = 4;
    int m_bytesPerLine = 16;

    unsigned m_topLine = 0;
    unsigned m_cursorAddr = 0;
    int m_nibble = 0;           // the hex digit being typed at the cursor: 0 - high, 1 - low
    bool m_running = true;
    bool m_hasState = false;

    // data shown: from m_dataStart
    unsigned m_dataStart = 0;
    std::vector<uint8_t> m_data;
    // data shown at the previous stop, to highlight changes
    unsigned m_prevStart = 0;
    std::vector<uint8_t> m_prevData;
    bool m_hasPrev = false;

    int m_charWidth = 8;
    int m_lineHeight = 16;
    int m_ascent = 12;

    void updateFontMetrics();
    void updateLayout();
    int width16() const;    // the width of a 16-byte line
    int visibleLines();
    unsigned lineCount();
    void readData();
    void setTopLine(unsigned line);
    void ensureCursorVisible();
    void moveCursor(long delta);
    void syncScrollBar();
    bool canNavigate() {return !m_running && m_hasState;}
    int byteX(int index);   // x of a byte in the hex part
    int charsX();
    QChar byteChar(uint8_t value);
};


// The memory panel: memory selector, address field and the dump
class DbgMemPanel : public QWidget
{
    Q_OBJECT

public:
    DbgMemPanel(DebugSession* session, QWidget* parent = nullptr);

    DbgMemView* getView() {return m_view;}
    void showAddress(uint16_t addr); // in the CPU address space

signals:
    void showInCode(uint16_t addr);

public slots:
    void onStateChanged(bool newStop);
    void onRunStateChanged(bool running);

private slots:
    void onMemoryChanged(int index);
    void onAddressEntered();
    void onBreakpoint(int type);

private:
    DebugSession* m_session;
    QComboBox* m_memoryCombo;
    QLineEdit* m_addrEdit;
    DbgMemView* m_view;
    QAction* m_readBpAction;
    QAction* m_writeBpAction;
    QAction* m_accessBpAction;
    QAction* m_showInCodeAction;

    void updateActions();
};


#endif // QTDBGMEMVIEW_H
