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

#include <algorithm>

#include <QAction>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QSignalBlocker>

#include "qtDbgStackView.h"
#include "qtDbgCompat.h"
#include "qtDbgStyle.h"

#include "../DebugSession.h"
#include "../DebugTarget.h"


namespace {

// items above the top (already popped) that can be scrolled to
const int c_itemsAbove = 16;

QString hex(unsigned value, int digits)
{
    return QString("%1").arg(value, digits, 16, QChar('0')).toUpper();
}

} // namespace


DbgStackView::DbgStackView(DebugSession* session, QWidget* parent)
    : QAbstractScrollArea(parent), m_session(session)
{
    setFont(dbgFont());
    setFocusPolicy(Qt::StrongFocus);
    setContextMenuPolicy(Qt::ActionsContextMenu);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    updateFontMetrics();

    m_showInCodeAction = new QAction(tr("Show value in code"), this);
    m_showInMemoryAction = new QAction(tr("Show value in memory"), this);
    m_showItemInMemoryAction = new QAction(tr("Show stack item in memory"), this);
    connect(m_showInCodeAction, &QAction::triggered, this, [this]() {
        emit showInCode(uint16_t(cursorValue()));
    });
    connect(m_showInMemoryAction, &QAction::triggered, this, [this]() {
        emit showInMemory(uint16_t(cursorValue()));
    });
    connect(m_showItemInMemoryAction, &QAction::triggered, this, [this]() {
        emit showInMemory(itemAddr(m_cursorItem));
    });
    addActions({m_showInCodeAction, m_showInMemoryAction, m_showItemInMemoryAction});
    updateActions();
}


void DbgStackView::updateFontMetrics()
{
    DbgMetrics metrics(font());
    m_charWidth = metrics.charWidth;
    m_lineHeight = metrics.lineHeight;
    m_ascent = metrics.ascent;
}


QSize DbgStackView::sizeHint() const
{
    int frame = frameWidth() * 2;
    return QSize(m_charWidth * 18 + verticalScrollBar()->sizeHint().width() + frame, m_lineHeight * 8 + frame);
}


int DbgStackView::visibleLines() const
{
    int n = viewport()->height() / m_lineHeight;
    return n > 0 ? n : 1;
}


// The last item: the stack is shown up to the end of the address space
int DbgStackView::maxItem() const
{
    return (0x10000 - m_top) / m_itemSize - 1;
}


uint16_t DbgStackView::itemAddr(int item) const
{
    return uint16_t(m_top + item * m_itemSize);
}


// Little-endian item value
unsigned DbgStackView::readItem(int item)
{
    CpuDebugTarget* target = m_session->getTarget();
    uint16_t addr = itemAddr(item);
    unsigned value = 0;
    for (int i = 0; i < m_itemSize; i++)
        value |= unsigned(target->readMem(uint16_t(addr + i))) << (i * 8);
    return value;
}


unsigned DbgStackView::cursorValue()
{
    return canNavigate() ? readItem(m_cursorItem) : 0;
}


void DbgStackView::readData()
{
    if (!canNavigate())
        return;
    int n = visibleLines() + 1;
    m_values.resize(n);
    for (int i = 0; i < n; i++) {
        int item = m_firstItem + i;
        m_values[i] = item <= maxItem() ? readItem(item) : 0;
    }
}


void DbgStackView::syncScrollBar()
{
    QScrollBar* bar = verticalScrollBar();
    QSignalBlocker blocker(bar);
    bar->setRange(-c_itemsAbove, std::max(-c_itemsAbove, maxItem() - visibleLines() + 1));
    bar->setPageStep(visibleLines());
    bar->setValue(m_firstItem);
}


void DbgStackView::setFirstItem(int item)
{
    int last = std::max(-c_itemsAbove, maxItem() - visibleLines() + 1);
    m_firstItem = std::min(std::max(item, -c_itemsAbove), last);
    readData();
    syncScrollBar();
    viewport()->update();
}


void DbgStackView::moveCursor(int delta)
{
    if (!canNavigate())
        return;
    m_cursorItem = std::min(std::max(m_cursorItem + delta, -c_itemsAbove), maxItem());
    if (m_cursorItem < m_firstItem)
        setFirstItem(m_cursorItem);
    else if (m_cursorItem >= m_firstItem + visibleLines())
        setFirstItem(m_cursorItem - visibleLines() + 1);
    viewport()->update();
}


void DbgStackView::onStateChanged(bool newStop)
{
    m_running = m_session->isRunning();
    CpuDebugTarget* target = m_session->getTarget();

    if (newStop && m_hasState) {
        m_prevValues.clear();
        for (int i = 0; i < int(m_values.size()); i++) {
            int item = m_firstItem + i;
            if (item <= maxItem())
                m_prevValues.push_back(std::make_pair(itemAddr(item), m_values[i]));
        }
    }

    // a new top (or the first state) shows the stack from the top
    uint16_t top = target->getStackTop();
    if (top != m_top || !m_hasState) {
        m_top = top;
        m_itemSize = target->getStackItemSize();
        m_firstItem = 0;
        m_cursorItem = 0;
    }
    m_hasState = true;

    readData();
    syncScrollBar();
    updateActions();
    viewport()->update();
}


void DbgStackView::onRunStateChanged(bool running)
{
    m_running = running;
    updateActions();
    viewport()->update();
}


void DbgStackView::updateActions()
{
    for (QAction* action : actions())
        action->setEnabled(canNavigate());
}


void DbgStackView::paintEvent(QPaintEvent*)
{
    QPainter painter(viewport());
    DbgColors colors(palette());
    bool stale = m_running;
    int width = viewport()->width();

    painter.fillRect(viewport()->rect(), colors.bg(stale));
    if (!m_hasState)
        return;

    int cw = m_charWidth;
    bool focused = hasFocus() && !stale;
    int lines = std::min(visibleLines() + 1, int(m_values.size()));

    for (int i = 0; i < lines; i++) {
        int item = m_firstItem + i;
        if (item > maxItem())
            break;
        int y = i * m_lineHeight;
        int baseY = y + m_ascent;
        uint16_t addr = itemAddr(item);
        unsigned value = m_values[i];

        QRect lineRect(0, y, width, m_lineHeight);
        if (item == 0)
            painter.fillRect(lineRect, colors.pcBg(stale));
        else if (item == m_cursorItem && focused)
            painter.fillRect(lineRect, colors.cursorBackground);
        if (item == m_cursorItem) {
            painter.setPen(stale ? colors.staleCursorFrame : colors.cursorFrame);
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(lineRect.adjusted(0, 0, -1, -1));
        }

        // offset from the top: SP, +02, -02...
        QString label = item == 0 ? QString("SP")
                                  : QString(item > 0 ? "+" : "-") + hex(unsigned(std::abs(item) * m_itemSize), 2);
        painter.setPen(colors.dimFg(stale));
        painter.drawText(cw / 2 + (5 - label.size()) * cw, baseY, label);
        painter.drawText(cw * 7, baseY, hex(addr, 4));

        bool changed = false;
        for (const auto& prev : m_prevValues)
            if (prev.first == addr) {
                changed = prev.second != value;
                break;
            }
        if (item < 0)
            painter.setPen(colors.dimFg(stale)); // already popped
        else
            painter.setPen(changed && !stale ? colors.changedText : colors.fg(stale));
        painter.drawText(cw * 13, baseY, hex(value, m_itemSize * 2));
    }
}


void DbgStackView::resizeEvent(QResizeEvent* event)
{
    QAbstractScrollArea::resizeEvent(event);
    if (canNavigate())
        setFirstItem(m_firstItem);
}


// The cursor background shows the focused view: the viewport is repainted on focus
// changes (the scroll area itself updates only its frame)
void DbgStackView::focusInEvent(QFocusEvent* event)
{
    QAbstractScrollArea::focusInEvent(event);
    viewport()->update();
}


void DbgStackView::focusOutEvent(QFocusEvent* event)
{
    QAbstractScrollArea::focusOutEvent(event);
    viewport()->update();
}


void DbgStackView::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange) {
        updateFontMetrics();
        readData();
    }
    QAbstractScrollArea::changeEvent(event);
    viewport()->update();
}


void DbgStackView::keyPressEvent(QKeyEvent* event)
{
    if (!canNavigate()) {
        QAbstractScrollArea::keyPressEvent(event);
        return;
    }

    int page = visibleLines() - 1;
    switch (event->key()) {
    case Qt::Key_Up:
        moveCursor(-1);
        return;
    case Qt::Key_Down:
        moveCursor(1);
        return;
    case Qt::Key_PageUp:
        moveCursor(-page);
        return;
    case Qt::Key_PageDown:
        moveCursor(page);
        return;
    case Qt::Key_Home:
        m_cursorItem = 0;
        setFirstItem(0);
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        emit showInCode(uint16_t(cursorValue()));
        return;
    default:
        break;
    }
    QAbstractScrollArea::keyPressEvent(event);
}


void DbgStackView::mousePressEvent(QMouseEvent* event)
{
    if (!canNavigate())
        return;
    int item = m_firstItem + dbgMousePos(event).y() / m_lineHeight;
    if (item >= -c_itemsAbove && item <= maxItem()) {
        m_cursorItem = item;
        viewport()->update();
    }
}


void DbgStackView::mouseDoubleClickEvent(QMouseEvent* event)
{
    mousePressEvent(event);
    if (canNavigate())
        emit showInCode(uint16_t(cursorValue()));
}


void DbgStackView::scrollContentsBy(int, int)
{
    if (!canNavigate()) {
        syncScrollBar();
        return;
    }
    m_firstItem = verticalScrollBar()->value();
    readData();
    viewport()->update();
}
