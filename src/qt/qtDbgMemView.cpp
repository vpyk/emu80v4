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

#include <QAction>
#include <QComboBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QShortcut>
#include <QSignalBlocker>
#include <QToolTip>
#include <QVBoxLayout>

#include "qtDbgMemView.h"
#include "qtDbgCompat.h"
#include "qtDbgStyle.h"

#include "../DebugSession.h"
#include "../Platform.h"
#include "../CrtRenderer.h"


namespace {

QString hex(unsigned value, int digits)
{
    return QString("%1").arg(value, digits, 16, QChar('0')).toUpper();
}

// Cyrillic letters of KOI-8 at C0h...FFh
const char* c_koi8Cyrillic = u8"юабцдефгхийклмнопярстужвьызшэщчъЮАБЦДЕФГХИЙКЛМНОПЯРСТУЖВЬЫЗШЭЩЧЪ";

} // namespace


//##### DbgMemView ####

DbgMemView::DbgMemView(DebugSession* session, QWidget* parent)
    : QAbstractScrollArea(parent), m_session(session)
{
    setFont(dbgFont());
    setFocusPolicy(Qt::StrongFocus);
    setContextMenuPolicy(Qt::ActionsContextMenu);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    updateFontMetrics();
    setMemory(0);
}


void DbgMemView::updateFontMetrics()
{
    DbgMetrics metrics(font());
    m_charWidth = metrics.charWidth;
    m_lineHeight = metrics.lineHeight;
    m_ascent = metrics.ascent;
}


void DbgMemView::setMemory(int memory)
{
    const std::vector<DebugMemory>& memories = m_session->getMemories();
    if (memory < 0 || memory >= int(memories.size()))
        return;
    m_memory = memory;
    m_size = memories[memory].size;
    m_addrDigits = 4;
    while (m_addrDigits < 8 && (m_size - 1) >> (m_addrDigits * 4))
        ++m_addrDigits;
    m_hasPrev = false;
    m_prevData.clear();
    if (m_cursorAddr >= m_size)
        m_cursorAddr = 0;
    m_nibble = 0;
    updateLayout();
    ensureCursorVisible();
    readData();
    viewport()->update();
}


int DbgMemView::width16() const
{
    return (m_addrDigits + 2 + 16 * 3 + 1 + 1 + 16 + 1) * m_charWidth;
}


// 16 bytes per line if they fit, else 8
void DbgMemView::updateLayout()
{
    m_bytesPerLine = viewport()->width() >= width16() ? 16 : 8;
    syncScrollBar();
}


QSize DbgMemView::sizeHint() const
{
    int frame = frameWidth() * 2;
    return QSize(width16() + verticalScrollBar()->sizeHint().width() + frame, m_lineHeight * 12 + frame);
}


int DbgMemView::visibleLines()
{
    int n = viewport()->height() / m_lineHeight;
    return n > 0 ? n : 1;
}


unsigned DbgMemView::lineCount()
{
    return (m_size + m_bytesPerLine - 1) / m_bytesPerLine;
}


int DbgMemView::byteX(int index)
{
    int x = (m_addrDigits + 2) * m_charWidth + index * 3 * m_charWidth;
    if (index >= 8)
        x += m_charWidth; // a gap in the middle of a 16-byte line
    return x;
}


int DbgMemView::charsX()
{
    return byteX(m_bytesPerLine) + m_charWidth;
}


QChar DbgMemView::byteChar(uint8_t value)
{
    CodePage cp = m_session->getPlatform()->getCodePage();
    if (cp == CP_RK) {
        // the characters of the RK-86 font, including the graphics below 20h;
        // codes without a character are shown as dots
        if (value == 0x20)
            return QChar(' ');
        if (value < 0x80) {
            wchar_t ch = TextCrtRenderer::c_rkSymbols[value];
            if (ch != L' ')
                return QChar(static_cast<ushort>(ch));
        }
        return QChar('.');
    }

    if (value >= 0x20 && value < 0x60)
        return QChar(value);
    if (cp == CP_ASCII) {
        if (value >= 0x60 && value < 0x7F)
            return QChar(value);
    } else {
        if (value >= 0x60 && value < 0x7F)
            return QChar(value);
        if (value >= 0xC0)
            return QString::fromUtf8(c_koi8Cyrillic).at(value - 0xC0);
        if (value == 0xA3)
            return QChar(0x0451); // ё
        if (value == 0xB3)
            return QChar(0x0401); // Ё
    }
    return QChar('.');
}


bool DbgMemView::charToByte(QChar ch, uint8_t& value)
{
    QChar variants[] = {ch, ch.toUpper()};
    for (QChar c : variants)
        for (int i = 0; i < 256; i++)
            if (byteChar(uint8_t(i)) == c && (c != QChar('.') || i == '.')) {
                value = uint8_t(i);
                return true;
            }
    return false;
}


bool DbgMemView::find(const std::vector<uint8_t>& bytes, bool forward)
{
    if (!canNavigate() || bytes.empty() || bytes.size() > m_size)
        return false;
    for (unsigned n = 1; n <= m_size; n++) {
        unsigned addr = forward ? (m_cursorAddr + n) % m_size : (m_cursorAddr + m_size - n) % m_size;
        if (addr + bytes.size() > m_size)
            continue;
        unsigned i = 0;
        while (i < bytes.size() && m_session->readMem(m_memory, addr + i) == bytes[i])
            i++;
        if (i == bytes.size()) {
            gotoAddr(addr);
            return true;
        }
    }
    return false;
}


void DbgMemView::readData()
{
    if (!canNavigate())
        return;
    m_dataStart = m_topLine * m_bytesPerLine;
    unsigned n = (visibleLines() + 1) * m_bytesPerLine;
    m_data.resize(n);
    for (unsigned i = 0; i < n; i++) {
        unsigned addr = m_dataStart + i;
        m_data[i] = addr < m_size ? m_session->readMem(m_memory, addr) : 0;
    }
}


void DbgMemView::syncScrollBar()
{
    QScrollBar* bar = verticalScrollBar();
    QSignalBlocker blocker(bar);
    int maxLine = int(lineCount()) - visibleLines();
    bar->setRange(0, maxLine > 0 ? maxLine : 0);
    bar->setPageStep(visibleLines());
    bar->setValue(int(m_topLine));
}


void DbgMemView::setTopLine(unsigned line)
{
    unsigned maxLine = lineCount() > unsigned(visibleLines()) ? lineCount() - visibleLines() : 0;
    m_topLine = line > maxLine ? maxLine : line;
    readData();
    syncScrollBar();
    viewport()->update();
}


void DbgMemView::ensureCursorVisible()
{
    unsigned line = m_cursorAddr / m_bytesPerLine;
    if (line < m_topLine)
        setTopLine(line);
    else if (line >= m_topLine + visibleLines())
        setTopLine(line - visibleLines() + 1);
}


void DbgMemView::gotoAddr(unsigned addr)
{
    if (addr >= m_size)
        return;
    m_cursorAddr = addr;
    m_nibble = 0;
    unsigned line = addr / m_bytesPerLine;
    if (line < m_topLine || line >= m_topLine + visibleLines())
        setTopLine(line);
    viewport()->update();
}


void DbgMemView::moveCursor(long delta)
{
    if (!canNavigate())
        return;
    long addr = long(m_cursorAddr) + delta;
    if (addr < 0)
        addr = 0;
    if (addr >= long(m_size))
        addr = m_size - 1;
    m_cursorAddr = unsigned(addr);
    m_nibble = 0;
    ensureCursorVisible();
    viewport()->update();
}


void DbgMemView::onStateChanged(bool newStop)
{
    m_running = m_session->isRunning();
    if (newStop && m_hasState) {
        m_prevStart = m_dataStart;
        m_prevData = m_data;
        m_hasPrev = true;
    }
    m_hasState = true;
    readData();
    viewport()->update();
}


void DbgMemView::onRunStateChanged(bool running)
{
    m_running = running;
    m_nibble = 0;
    viewport()->update();
}


void DbgMemView::paintEvent(QPaintEvent*)
{
    QPainter painter(viewport());
    DbgColors colors(palette());
    bool stale = m_running;

    painter.fillRect(viewport()->rect(), colors.bg(stale));
    if (!m_hasState)
        return;

    int cw = m_charWidth;
    int lines = visibleLines() + 1;
    int chX = charsX();
    bool focused = hasFocus() && !stale;

    for (int line = 0; line < lines; line++) {
        unsigned lineAddr = (m_topLine + line) * m_bytesPerLine;
        if (lineAddr >= m_size)
            break;
        int y = line * m_lineHeight;
        int baseY = y + m_ascent;

        painter.setPen(colors.dimFg(stale));
        painter.drawText(cw / 2, baseY, hex(lineAddr, m_addrDigits));

        for (int i = 0; i < m_bytesPerLine; i++) {
            unsigned addr = lineAddr + i;
            if (addr >= m_size)
                break;
            unsigned idx = addr - m_dataStart;
            if (idx >= m_data.size())
                break;
            uint8_t value = m_data[idx];

            bool changed = false;
            if (m_hasPrev && addr >= m_prevStart && addr - m_prevStart < m_prevData.size())
                changed = m_prevData[addr - m_prevStart] != value;

            int x = byteX(i);
            if (addr == m_cursorAddr) {
                QRect hexRect(x - cw / 2, y, cw * 3, m_lineHeight);
                QRect chRect(chX + i * cw, y, cw, m_lineHeight);
                if (focused) // the part being typed in
                    painter.fillRect(m_charMode ? chRect : hexRect, colors.cursorBackground);
                painter.setPen(stale ? colors.staleCursorFrame : colors.cursorFrame);
                painter.setBrush(Qt::NoBrush);
                if (m_nibble && focused && !m_charMode)
                    painter.drawRect(QRect(x + cw, y, cw - 1, m_lineHeight - 1)); // the low digit is typed
                else
                    painter.drawRect(hexRect.adjusted(0, 0, -1, -1));
                painter.drawRect(chRect.adjusted(0, 0, -1, -1));
            }

            painter.setPen(changed && !stale ? colors.changedText : colors.fg(stale));
            painter.drawText(x, baseY, hex(value, 2));
            // each character in its own cell: some of them may come from a fallback font
            // of a different width
            painter.drawText(chX + i * cw, baseY, QString(byteChar(value)));
        }
    }
}


void DbgMemView::resizeEvent(QResizeEvent* event)
{
    QAbstractScrollArea::resizeEvent(event);
    int oldBpl = m_bytesPerLine;
    updateLayout();
    if (oldBpl != m_bytesPerLine)
        m_topLine = m_topLine * oldBpl / m_bytesPerLine; // keep the same address at the top
    setTopLine(m_topLine);
}


void DbgMemView::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange) {
        updateFontMetrics();
        updateLayout();
        readData();
    }
    QAbstractScrollArea::changeEvent(event);
    viewport()->update();
}


// Tab switches the cursor between the hex and character parts
bool DbgMemView::focusNextPrevChild(bool next)
{
    if (!canNavigate())
        return QAbstractScrollArea::focusNextPrevChild(next);
    m_charMode = !m_charMode;
    m_nibble = 0;
    viewport()->update();
    return true;
}


// The cursor background shows the focused view: the viewport is repainted on focus
// changes (the scroll area itself updates only its frame)
void DbgMemView::focusInEvent(QFocusEvent* event)
{
    QAbstractScrollArea::focusInEvent(event);
    viewport()->update();
    emit focused();
}


void DbgMemView::focusOutEvent(QFocusEvent* event)
{
    QAbstractScrollArea::focusOutEvent(event);
    viewport()->update();
}


void DbgMemView::keyPressEvent(QKeyEvent* event)
{
    if (!canNavigate()) {
        QAbstractScrollArea::keyPressEvent(event);
        return;
    }

    long page = long(visibleLines() - 1) * m_bytesPerLine;
    bool ctrl = event->modifiers() & Qt::ControlModifier;
    switch (event->key()) {
    case Qt::Key_Left:
        moveCursor(-1);
        return;
    case Qt::Key_Right:
        moveCursor(1);
        return;
    case Qt::Key_Up:
        moveCursor(-m_bytesPerLine);
        return;
    case Qt::Key_Down:
        moveCursor(m_bytesPerLine);
        return;
    case Qt::Key_PageUp:
        moveCursor(-page);
        return;
    case Qt::Key_PageDown:
        moveCursor(page);
        return;
    case Qt::Key_Home:
        moveCursor(ctrl ? -long(m_cursorAddr) : -long(m_cursorAddr % m_bytesPerLine));
        return;
    case Qt::Key_End:
        moveCursor(ctrl ? long(m_size) : long(m_bytesPerLine - 1 - m_cursorAddr % m_bytesPerLine));
        return;
    default:
        break;
    }

    QString text = event->text();

    // a printable character in the character part changes the byte at the cursor
    if (m_charMode) {
        uint8_t value;
        if (text.size() == 1 && !ctrl && text[0].isPrint() && charToByte(text[0], value)) {
            m_session->writeMem(m_memory, m_cursorAddr, value);
            moveCursor(1);
            readData();
            viewport()->update();
            return;
        }
        QAbstractScrollArea::keyPressEvent(event);
        return;
    }

    // hex digits change the byte at the cursor
    if (text.size() == 1 && !ctrl) {
        bool ok;
        unsigned digit = text.toUInt(&ok, 16);
        if (ok) {
            uint8_t value = m_session->readMem(m_memory, m_cursorAddr);
            if (m_nibble == 0)
                value = uint8_t((digit << 4) | (value & 0x0F));
            else
                value = uint8_t((value & 0xF0) | digit);
            m_session->writeMem(m_memory, m_cursorAddr, value);
            if (m_nibble == 0)
                m_nibble = 1;
            else
                moveCursor(1);
            readData();
            viewport()->update();
            return;
        }
    }
    QAbstractScrollArea::keyPressEvent(event);
}


void DbgMemView::mousePressEvent(QMouseEvent* event)
{
    if (!canNavigate())
        return;
    QPoint pos = dbgMousePos(event);
    unsigned line = m_topLine + pos.y() / m_lineHeight;
    int index = -1;
    int chX = charsX();
    bool charPart = pos.x() >= chX;
    if (charPart)
        index = (pos.x() - chX) / m_charWidth;
    else
        for (int i = 0; i < m_bytesPerLine; i++)
            if (pos.x() >= byteX(i) - m_charWidth / 2 && pos.x() < byteX(i) + m_charWidth * 5 / 2)
                index = i;
    if (index < 0 || index >= m_bytesPerLine)
        return;
    unsigned addr = line * m_bytesPerLine + index;
    if (addr < m_size) {
        m_cursorAddr = addr;
        m_nibble = 0;
        m_charMode = charPart;
        viewport()->update();
    }
}


void DbgMemView::scrollContentsBy(int, int)
{
    if (!canNavigate()) {
        syncScrollBar();
        return;
    }
    m_topLine = unsigned(verticalScrollBar()->value());
    readData();
    viewport()->update();
}


//##### DbgMemPanel ####

DbgMemPanel::DbgMemPanel(DebugSession* session, QWidget* parent)
    : QWidget(parent), m_session(session)
{
    m_memoryCombo = new QComboBox(this);
    for (const DebugMemory& mem : m_session->getMemories())
        m_memoryCombo->addItem(QString("%1 (%2)").arg(QString::fromUtf8(mem.name.c_str())).arg(hex(mem.size, 1)));

    m_addrEdit = new QLineEdit(this);
    m_addrEdit->setFont(dbgFont());
    m_addrEdit->setMaxLength(8);
    m_addrEdit->setFixedWidth(DbgMetrics(dbgFont()).charWidth * 10 + 6);
    m_addrEdit->setToolTip(tr("Address (hex), Enter to go"));

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setFont(dbgFont());
    m_searchEdit->setPlaceholderText(tr("Find"));
    m_searchEdit->setToolTip(tr("Hex bytes (C3 00 F8) or text in quotes (\"TEXT\"); "
                                "Enter, F3 - find next, Shift-F3 - find previous"));
    m_searchEdit->setFixedWidth(DbgMetrics(dbgFont()).charWidth * 24 + 6);

    m_view = new DbgMemView(session, this);
    connect(m_view, &DbgMemView::focused, this, &DbgMemPanel::activated);

    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->addWidget(m_memoryCombo);
    topLayout->addWidget(new QLabel(tr("Address:"), this));
    topLayout->addWidget(m_addrEdit);
    topLayout->addSpacing(DbgMetrics(dbgFont()).charWidth * 2);
    topLayout->addWidget(m_searchEdit);
    topLayout->addStretch(1);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(topLayout);
    layout->addWidget(m_view, 1);

    connect(m_memoryCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &DbgMemPanel::onMemoryChanged);
    connect(m_addrEdit, &QLineEdit::returnPressed, this, &DbgMemPanel::onAddressEntered);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, [this]() {onFind(true);});
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this]() {
        m_searchEdit->setPalette(QPalette()); // the error color is reset on editing
    });

    // search keys work in the whole panel
    QShortcut* findShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);
    findShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(findShortcut, &QShortcut::activated, this, [this]() {
        m_searchEdit->setFocus();
        m_searchEdit->selectAll();
    });
    QShortcut* nextShortcut = new QShortcut(QKeySequence("F3"), this);
    nextShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(nextShortcut, &QShortcut::activated, this, [this]() {onFind(true);});
    QShortcut* prevShortcut = new QShortcut(QKeySequence("Shift+F3"), this);
    prevShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(prevShortcut, &QShortcut::activated, this, [this]() {onFind(false);});

    // context menu of the dump
    m_readBpAction = new QAction(tr("Breakpoint on read"), this);
    m_writeBpAction = new QAction(tr("Breakpoint on write"), this);
    m_accessBpAction = new QAction(tr("Breakpoint on access"), this);
    m_showInCodeAction = new QAction(tr("Show in code"), this);
    connect(m_readBpAction, &QAction::triggered, this, [this]() {onBreakpoint(BT_READ);});
    connect(m_writeBpAction, &QAction::triggered, this, [this]() {onBreakpoint(BT_WRITE);});
    connect(m_accessBpAction, &QAction::triggered, this, [this]() {onBreakpoint(BT_ACSESS);});
    connect(m_showInCodeAction, &QAction::triggered, this, [this]() {
        emit showInCode(uint16_t(m_view->getCursorAddr()));
    });
    QAction* separator = new QAction(this);
    separator->setSeparator(true);
    m_view->addActions({m_readBpAction, m_writeBpAction, m_accessBpAction, separator, m_showInCodeAction});

    updateActions();
}


void DbgMemPanel::showAddress(uint16_t addr)
{
    if (m_session->isRunning())
        return;
    m_memoryCombo->setCurrentIndex(0); // cpu
    m_view->gotoAddr(addr);
    m_view->setFocus();
}


void DbgMemPanel::onMemoryChanged(int index)
{
    m_view->setMemory(index);
    updateActions();
}


void DbgMemPanel::onAddressEntered()
{
    bool ok;
    unsigned addr = m_addrEdit->text().trimmed().toUInt(&ok, 16);
    if (ok)
        m_view->gotoAddr(addr);
    m_view->setFocus();
}


bool DbgMemPanel::parsePattern(const QString& text, std::vector<uint8_t>& bytes)
{
    bytes.clear();
    QString s = text.trimmed();
    if (s.startsWith('"')) {
        s = s.mid(1);
        if (s.endsWith('"'))
            s.chop(1);
        for (QChar ch : s) {
            uint8_t value;
            if (!m_view->charToByte(ch, value))
                return false;
            bytes.push_back(value);
        }
    } else {
        s.remove(' ');
        if (s.size() % 2)
            return false;
        for (int i = 0; i < s.size(); i += 2) {
            bool ok;
            unsigned value = s.mid(i, 2).toUInt(&ok, 16);
            if (!ok)
                return false;
            bytes.push_back(uint8_t(value));
        }
    }
    return !bytes.empty();
}


void DbgMemPanel::onFind(bool forward)
{
    if (m_session->isRunning() || m_searchEdit->text().trimmed().isEmpty())
        return;
    std::vector<uint8_t> bytes;
    QString message;
    if (!parsePattern(m_searchEdit->text(), bytes))
        message = tr("Invalid search pattern");
    else if (!m_view->find(bytes, forward))
        message = tr("Not found");
    if (message.isEmpty())
        return;
    // the message near the field, the field in the error color
    QPalette pal = m_searchEdit->palette();
    pal.setColor(QPalette::Text, DbgColors(palette()).changedText);
    m_searchEdit->setPalette(pal);
    QToolTip::showText(m_searchEdit->mapToGlobal(QPoint(0, m_searchEdit->height())), message, m_searchEdit);
}


// Memory breakpoints are set in the CPU address space only
void DbgMemPanel::onBreakpoint(int type)
{
    if (m_view->getMemory() != 0)
        return;
    BreakpointManager* bpm = m_session->getBreakpoints();
    bpm->add(BreakpointType(type), uint16_t(m_view->getCursorAddr()), DC_UI);
}


void DbgMemPanel::updateActions()
{
    bool cpu = m_view->getMemory() == 0;
    bool stopped = !m_session->isRunning();
    m_readBpAction->setEnabled(cpu);
    m_writeBpAction->setEnabled(cpu);
    m_accessBpAction->setEnabled(cpu);
    m_showInCodeAction->setEnabled(cpu && stopped);
    m_addrEdit->setEnabled(stopped);
    m_searchEdit->setEnabled(stopped);
    m_memoryCombo->setEnabled(stopped);
}


void DbgMemPanel::onStateChanged(bool newStop)
{
    m_view->onStateChanged(newStop);
    updateActions();
}


void DbgMemPanel::onRunStateChanged(bool running)
{
    m_view->onRunStateChanged(running);
    updateActions();
}
