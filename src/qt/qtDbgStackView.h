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

#ifndef QTDBGSTACKVIEW_H
#define QTDBGSTACKVIEW_H

#include <vector>

#include <QAbstractScrollArea>

class QAction;
class DebugSession;


// The stack from its top item (SP) down: offset from SP, item address and value.
// A few items above the top (already popped) can be scrolled to and are dimmed.
// Values changed since the previous stop are highlighted. Enter or a double click
// shows the value (a return address) in the code view.
//
// Memory is read only while the emulation is stopped; while it runs the view keeps
// the data of the last stop, shows it as outdated and doesn't scroll.
class DbgStackView : public QAbstractScrollArea
{
    Q_OBJECT

public:
    DbgStackView(DebugSession* session, QWidget* parent = nullptr);

    QSize sizeHint() const override;

signals:
    void showInCode(uint16_t addr);
    void showInMemory(uint16_t addr);

public slots:
    void onStateChanged(bool newStop);
    void onRunStateChanged(bool running);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;
    void changeEvent(QEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    DebugSession* m_session;
    bool m_running = true;
    bool m_hasState = false;

    uint16_t m_top = 0;     // the address of the top item
    int m_itemSize = 2;
    int m_firstItem = 0;    // the item in the first line, relative to the top (may be negative)
    int m_cursorItem = 0;

    // values shown: of items from m_firstItem
    std::vector<unsigned> m_values;
    // values at the previous stop by address, to highlight changes
    std::vector<std::pair<uint16_t, unsigned>> m_prevValues;

    int m_charWidth = 8;
    int m_lineHeight = 16;
    int m_ascent = 12;

    QAction* m_showInCodeAction;
    QAction* m_showInMemoryAction;
    QAction* m_showItemInMemoryAction;

    void updateFontMetrics();
    int visibleLines() const;
    int maxItem() const;
    int lastFirstItem() const;
    uint16_t itemAddr(int item) const;
    unsigned readItem(int item);
    unsigned cursorValue();
    void readData();
    void setFirstItem(int item);
    void moveCursor(int delta);
    void syncScrollBar();
    bool canNavigate() {return !m_running && m_hasState;}
    void updateActions();
};


#endif // QTDBGSTACKVIEW_H
