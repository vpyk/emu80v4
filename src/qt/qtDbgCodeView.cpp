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

#include <QInputDialog>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QWheelEvent>

#include "qtDbgCodeView.h"
#include "qtDbgCompat.h"
#include "qtDbgStyle.h"

#include "../DebugSession.h"


namespace {

// Bytes shown per instruction (the longest 8080/Z80/6502 instruction)
const int c_maxBytes = 4;

// Columns, in characters
const int c_gutterChars = 3;
const int c_addrChars = 6;
const int c_bytesChars = c_maxBytes * 3 + 1;

QString hex(unsigned value, int digits)
{
    return QString("%1").arg(value, digits, 16, QChar('0')).toUpper();
}

} // namespace


DbgCodeView::DbgCodeView(DebugSession* session, QWidget* parent)
    : QAbstractScrollArea(parent), m_session(session)
{
    setFont(dbgFont());
    setFocusPolicy(Qt::StrongFocus);
    setContextMenuPolicy(Qt::ActionsContextMenu);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    verticalScrollBar()->setRange(0, 0xFFFF);
    updateFontMetrics();
}


void DbgCodeView::updateFontMetrics()
{
    DbgMetrics metrics(font());
    m_charWidth = metrics.charWidth;
    m_lineHeight = metrics.lineHeight;
    m_ascent = metrics.ascent;
}


int DbgCodeView::visibleLines()
{
    int n = viewport()->height() / m_lineHeight;
    return n > 0 ? n : 1;
}


uint16_t DbgCodeView::nextInstrAddr(uint16_t addr)
{
    return uint16_t(addr + m_session->getTarget()->getInstrLength(addr));
}


// Finds the start of the instruction before addr: decoding from several points
// before it, the instruction that ends at addr most often wins
uint16_t DbgCodeView::prevInstrAddr(uint16_t addr)
{
    const int maxBack = 16;
    std::map<uint16_t, int> votes;
    CpuDebugTarget* target = m_session->getTarget();

    for (int back = maxBack; back >= 1; back--) {
        uint16_t a = uint16_t(addr - back);
        uint16_t last = a;
        for (;;) {
            uint16_t dist = uint16_t(addr - a);
            if (dist == 0) {
                votes[last]++;
                break;
            }
            if (dist > maxBack)
                break; // passed over addr
            last = a;
            a = uint16_t(a + target->getInstrLength(a));
        }
    }

    uint16_t best = uint16_t(addr - 1);
    int bestVotes = 0;
    for (auto it = votes.begin(); it != votes.end(); ++it) {
        // on a tie the closer start wins
        if (it->second > bestVotes || (it->second == bestVotes && uint16_t(addr - it->first) < uint16_t(addr - best))) {
            best = it->first;
            bestVotes = it->second;
        }
    }
    return best;
}


void DbgCodeView::buildLines()
{
    if (!canNavigate())
        return;

    CpuDebugTarget* target = m_session->getTarget();
    int n = visibleLines() + 1;
    m_lines.clear();
    m_lines.reserve(n);

    uint16_t addr = m_topAddr;
    for (int i = 0; i < n; i++) {
        DisasmInstr instr;
        m_session->disassemble(addr, instr);
        Line line;
        line.addr = addr;
        line.length = instr.length;
        line.text = QString::fromUtf8(instr.text.c_str());
        for (int j = 0; j < instr.length && j < c_maxBytes; j++) {
            if (j)
                line.bytes += ' ';
            line.bytes += hex(target->readMem(uint16_t(addr + j)), 2);
        }
        m_lines.push_back(line);
        addr = uint16_t(addr + instr.length);
    }
}


int DbgCodeView::lineIndex(uint16_t addr)
{
    int n = visibleLines();
    for (int i = 0; i < n && i < int(m_lines.size()); i++)
        if (m_lines[i].addr == addr)
            return i;
    return -1;
}


void DbgCodeView::syncScrollBar()
{
    QScrollBar* bar = verticalScrollBar();
    QSignalBlocker blocker(bar);
    bar->setPageStep(visibleLines() * 2);
    bar->setValue(m_topAddr);
}


void DbgCodeView::setTop(uint16_t addr)
{
    m_topAddr = addr;
    buildLines();
    syncScrollBar();
    viewport()->update();
}


// Makes the address visible; with center (or if it is not visible) puts it
// into the upper third of the view
void DbgCodeView::showAddr(uint16_t addr, bool center)
{
    if (!center && lineIndex(addr) >= 0)
        return;
    uint16_t top = addr;
    int n = visibleLines() / 3;
    for (int i = 0; i < n; i++)
        top = prevInstrAddr(top);
    setTop(top);
}


void DbgCodeView::moveCursor(int lines)
{
    if (!canNavigate())
        return;

    if (lineIndex(m_cursorAddr) < 0)
        showAddr(m_cursorAddr, false);

    int visible = visibleLines();
    for (; lines > 0; lines--) {
        int idx = lineIndex(m_cursorAddr);
        if (idx + 1 < visible && idx + 1 < int(m_lines.size()))
            m_cursorAddr = m_lines[idx + 1].addr;
        else {
            setTop(nextInstrAddr(m_topAddr));
            m_cursorAddr = m_lines[std::min(visible, int(m_lines.size())) - 1].addr;
        }
    }
    for (; lines < 0; lines++) {
        int idx = lineIndex(m_cursorAddr);
        if (idx > 0)
            m_cursorAddr = m_lines[idx - 1].addr;
        else {
            setTop(prevInstrAddr(m_topAddr));
            m_cursorAddr = m_topAddr;
        }
    }
    viewport()->update();
}


void DbgCodeView::onStateChanged(bool newStop)
{
    m_hasState = true;
    m_running = m_session->isRunning();
    uint16_t pc = m_session->getTarget()->getPC();
    bool pcChanged = pc != m_pc;
    m_pc = pc;

    if (m_lines.empty() || newStop) {
        m_cursorAddr = m_pc;
        buildLines();
        showAddr(m_pc, false);
        // the view may have no real size yet (the window is being shown)
        m_showPcPending = !isVisible();
    } else {
        buildLines(); // memory may have changed
        if (pcChanged)
            showAddr(m_pc, false);
    }
    viewport()->update();
}


void DbgCodeView::onBreakpointsChanged()
{
    m_breakpoints.clear();
    std::vector<BreakpointDesc> bps;
    m_session->getBreakpoints()->list(bps);
    for (const BreakpointDesc& bp : bps)
        if (bp.type == BT_EXEC) {
            BpMark& mark = m_breakpoints[bp.addr];
            mark.enabled = bp.enabled;
            mark.external = bp.owner == DC_EXTERNAL;
        }
    viewport()->update();
}


void DbgCodeView::onRunStateChanged(bool running)
{
    m_running = running;
    viewport()->update();
}


void DbgCodeView::toggleBreakpoint()
{
    BreakpointManager* bpm = m_session->getBreakpoints();
    int id = bpm->find(BT_EXEC, m_cursorAddr);
    if (id >= 0)
        bpm->remove(id);
    else
        bpm->add(BT_EXEC, m_cursorAddr, DC_UI);
    onBreakpointsChanged();
}


void DbgCodeView::setPcToCursor()
{
    if (!canNavigate())
        return;
    m_session->setPC(m_cursorAddr);
    m_pc = m_cursorAddr;
    viewport()->update();
}


void DbgCodeView::gotoPc()
{
    if (!canNavigate())
        return;
    m_cursorAddr = m_pc;
    showAddr(m_pc, false);
    viewport()->update();
}


void DbgCodeView::gotoAddress()
{
    if (!canNavigate())
        return;
    bool ok;
    QString text = QInputDialog::getText(this, tr("Go to address"), tr("Address (hex):"),
                                         QLineEdit::Normal, hex(m_cursorAddr, 4), &ok);
    if (!ok)
        return;
    unsigned addr = text.trimmed().toUInt(&ok, 16);
    if (!ok || addr > 0xFFFF)
        return;
    m_cursorAddr = uint16_t(addr);
    showAddr(m_cursorAddr, false);
    viewport()->update();
}


void DbgCodeView::paintEvent(QPaintEvent*)
{
    QPainter painter(viewport());

    // Outdated data (the emulation is running) is shown in the stale colors
    DbgColors colors(palette());
    bool stale = m_running;
    QColor textColor = colors.fg(stale);
    QColor dimColor = colors.dimFg(stale);

    painter.fillRect(viewport()->rect(), colors.bg(stale));
    if (!m_hasState)
        return;

    int cw = m_charWidth;
    int gutterX = 0;
    int addrX = c_gutterChars * cw;
    int bytesX = addrX + c_addrChars * cw;
    int textX = bytesX + c_bytesChars * cw;
    int width = viewport()->width();

    painter.setRenderHint(QPainter::Antialiasing, true);

    for (int i = 0; i < int(m_lines.size()); i++) {
        const Line& line = m_lines[i];
        int y = i * m_lineHeight;
        if (y >= viewport()->height())
            break;

        if (line.addr == m_pc)
            painter.fillRect(addrX, y, width - addrX, m_lineHeight, colors.pcBg(stale));
        else if (line.addr == m_cursorAddr && hasFocus() && !stale)
            painter.fillRect(addrX, y, width - addrX, m_lineHeight, colors.cursorBackground);
        if (line.addr == m_cursorAddr) {
            painter.setPen(QPen(hasFocus() && !stale ? colors.cursorFrame : colors.cursorFrameUnfocused, 1));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(QRectF(addrX + 0.5, y + 0.5, width - addrX - 1, m_lineHeight - 1));
        }

        // breakpoint: filled circle if enabled, a ring if disabled; MCP breakpoints in another color
        auto bp = m_breakpoints.find(line.addr);
        if (bp != m_breakpoints.end()) {
            QColor bpColor = bp->second.external ? colors.externalBreakpoint : colors.breakpoint;
            if (stale)
                bpColor = dbgStaleColor(colors, bpColor);
            double d = m_lineHeight * 0.6;
            QRectF r(gutterX + cw * 0.3, y + (m_lineHeight - d) / 2, d, d);
            painter.setPen(QPen(bpColor, 1.5));
            painter.setBrush(bp->second.enabled ? QBrush(bpColor) : Qt::NoBrush);
            painter.drawEllipse(r);
        }

        // PC: an arrow
        if (line.addr == m_pc) {
            double h = m_lineHeight * 0.5;
            double x = gutterX + cw * 1.6;
            double yc = y + m_lineHeight / 2.0;
            QPainterPath arrow;
            arrow.moveTo(x, yc - h / 2);
            arrow.lineTo(x + h * 0.8, yc);
            arrow.lineTo(x, yc + h / 2);
            arrow.closeSubpath();
            painter.setPen(Qt::NoPen);
            painter.setBrush(stale ? dimColor : colors.pcMarker);
            painter.drawPath(arrow);
        }

        int baseY = y + m_ascent;
        painter.setPen(dimColor);
        painter.drawText(addrX + cw / 2, baseY, hex(line.addr, 4));
        painter.drawText(bytesX, baseY, line.bytes);
        painter.setPen(textColor);
        painter.drawText(textX, baseY, line.text);
    }
}


void DbgCodeView::showPendingPc()
{
    if (m_showPcPending && isVisible() && canNavigate()) {
        m_showPcPending = false;
        showAddr(m_pc, true);
    }
}


void DbgCodeView::resizeEvent(QResizeEvent* event)
{
    QAbstractScrollArea::resizeEvent(event);
    buildLines();
    syncScrollBar();
    showPendingPc();
}


void DbgCodeView::showEvent(QShowEvent* event)
{
    QAbstractScrollArea::showEvent(event);
    showPendingPc();
}


void DbgCodeView::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange) {
        updateFontMetrics();
        buildLines();
    }
    QAbstractScrollArea::changeEvent(event);
    viewport()->update();
}


void DbgCodeView::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Up:
        moveCursor(-1);
        break;
    case Qt::Key_Down:
        moveCursor(1);
        break;
    case Qt::Key_PageUp:
        moveCursor(-(visibleLines() - 1));
        break;
    case Qt::Key_PageDown:
        moveCursor(visibleLines() - 1);
        break;
    case Qt::Key_Home:
        gotoPc();
        break;
    default:
        QAbstractScrollArea::keyPressEvent(event);
    }
}


void DbgCodeView::mousePressEvent(QMouseEvent* event)
{
    QPoint pos = dbgMousePos(event);
    int idx = pos.y() / m_lineHeight;
    if (!m_hasState || idx >= visibleLines() + 1 || idx >= int(m_lines.size()))
        return;
    m_cursorAddr = m_lines[idx].addr;
    if (event->button() == Qt::LeftButton && pos.x() < c_gutterChars * m_charWidth)
        toggleBreakpoint();
    viewport()->update();
}


void DbgCodeView::wheelEvent(QWheelEvent* event)
{
    if (!canNavigate())
        return;
    int steps = -event->angleDelta().y() / 40; // 3 lines per notch
    for (; steps > 0; steps--)
        setTop(nextInstrAddr(m_topAddr));
    for (; steps < 0; steps++)
        setTop(prevInstrAddr(m_topAddr));
    event->accept();
}


// The scroll bar is dragged: its value is the top address
void DbgCodeView::scrollContentsBy(int, int)
{
    if (!canNavigate()) {
        syncScrollBar();
        return;
    }
    m_topAddr = uint16_t(verticalScrollBar()->value());
    buildLines();
    viewport()->update();
}
