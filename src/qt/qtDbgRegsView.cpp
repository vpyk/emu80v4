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

#include <QCheckBox>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#include "qtDbgRegsView.h"
#include "qtDbgStyle.h"

#include "../DebugSession.h"


namespace {

// Registers per row in the register groups
const int c_regsPerRow = 2;

QString hex(unsigned value, int digits)
{
    return QString("%1").arg(value, digits, 16, QChar('0')).toUpper();
}

} // namespace


DbgRegsView::DbgRegsView(DebugSession* session, QWidget* parent)
    : QWidget(parent), m_session(session)
{
    createWidgets();
    onRunStateChanged(m_session->isRunning());
}


void DbgRegsView::createWidgets()
{
    CpuDebugTarget* target = m_session->getTarget();
    QFont font = dbgFont();
    DbgMetrics metrics(font);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QGridLayout* grids[3] = {}; // DRG_MAIN, DRG_ALT, DRG_SPECIAL
    int gridCounts[3] = {};
    QHBoxLayout* flagsLayout = nullptr;

    int n = target->getRegCount();
    m_regs.reserve(n);
    for (int i = 0; i < n; i++) {
        const DebugRegInfo& info = target->getRegInfo(i);
        m_regs.push_back(Reg());
        Reg& reg = m_regs.back();
        reg.index = i;
        reg.digits = info.bits > 1 ? (info.bits + 3) / 4 : 0;

        QWidget* widget;
        if (reg.digits) {
            reg.edit = new QLineEdit(this);
            reg.edit->setFont(font);
            reg.edit->setMaxLength(reg.digits);
            reg.edit->setAlignment(Qt::AlignRight);
            reg.edit->setFixedWidth(metrics.charWidth * (reg.digits + 2) + 6);
            QLineEdit* edit = reg.edit;
            connect(edit, &QLineEdit::editingFinished, this, [this, i]() {commitEdit(m_regs[i]);});
            widget = edit;
        } else {
            reg.check = new QCheckBox(QString::fromUtf8(info.name), this);
            reg.check->setFont(font);
            connect(reg.check, &QCheckBox::clicked, this, [this, i](bool checked) {commitCheck(m_regs[i], checked);});
            widget = reg.check;
        }

        if (info.group == DRG_FLAG) {
            if (!flagsLayout) {
                flagsLayout = new QHBoxLayout;
                mainLayout->addLayout(flagsLayout);
            }
            flagsLayout->addWidget(widget);
            continue;
        }

        int g = info.group == DRG_MAIN ? 0 : info.group == DRG_ALT ? 1 : 2;
        if (!grids[g]) {
            grids[g] = new QGridLayout;
            grids[g]->setHorizontalSpacing(metrics.charWidth);
            mainLayout->addLayout(grids[g]);
        }
        int row = gridCounts[g] / c_regsPerRow;
        int col = gridCounts[g] % c_regsPerRow * 2;
        ++gridCounts[g];
        if (reg.edit) {
            QLabel* label = new QLabel(QString::fromUtf8(info.name), this);
            label->setFont(font);
            label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            label->setBuddy(reg.edit);
            grids[g]->addWidget(label, row, col);
            grids[g]->addWidget(reg.edit, row, col + 1);
        } else
            grids[g]->addWidget(widget, row, col, 1, 2);
    }

    // keep the fields together at the top left
    for (QGridLayout* grid : grids)
        if (grid)
            grid->setColumnStretch(c_regsPerRow * 2, 1);
    if (flagsLayout)
        flagsLayout->addStretch(1);
    mainLayout->addStretch(1);
}


void DbgRegsView::onStateChanged(bool newStop)
{
    CpuDebugTarget* target = m_session->getTarget();
    if (newStop && m_hasState) {
        for (Reg& reg : m_regs)
            reg.prevValue = reg.value;
        m_hasPrev = true;
    }
    for (Reg& reg : m_regs)
        reg.value = target->getReg(reg.index);
    if (!m_hasPrev)
        for (Reg& reg : m_regs)
            reg.prevValue = reg.value;
    m_hasState = true;
    m_running = m_session->isRunning();
    showValues();
    updateEnabled();
}


void DbgRegsView::showValues()
{
    for (Reg& reg : m_regs) {
        if (reg.edit) {
            // don't overwrite a value being typed
            if (!(reg.edit->hasFocus() && reg.edit->isModified())) {
                reg.edit->setText(hex(reg.value, reg.digits));
                reg.edit->setModified(false);
            }
        } else
            reg.check->setChecked(reg.value != 0);
    }
    updateColors();
}


// Changed values in the "changed" color; disabled fields (while running) use the
// palette's disabled colors, which show them as outdated
void DbgRegsView::updateColors()
{
    DbgColors colors(palette());
    for (Reg& reg : m_regs) {
        QWidget* widget = reg.edit ? static_cast<QWidget*>(reg.edit) : static_cast<QWidget*>(reg.check);
        bool changed = m_hasState && reg.value != reg.prevValue;
        QPalette pal = palette();
        if (changed) {
            pal.setColor(QPalette::Active, QPalette::Text, colors.changedText);
            pal.setColor(QPalette::Inactive, QPalette::Text, colors.changedText);
            pal.setColor(QPalette::Active, QPalette::WindowText, colors.changedText);
            pal.setColor(QPalette::Inactive, QPalette::WindowText, colors.changedText);
        }
        widget->setPalette(pal);
    }
}


void DbgRegsView::onRunStateChanged(bool running)
{
    m_running = running;
    updateEnabled();
}


// The fields can be changed only while stopped
void DbgRegsView::updateEnabled()
{
    bool enabled = !m_running && m_hasState;
    for (Reg& reg : m_regs) {
        if (reg.edit)
            reg.edit->setEnabled(enabled);
        else
            reg.check->setEnabled(enabled);
    }
}


void DbgRegsView::commitEdit(Reg& reg)
{
    if (!reg.edit->isModified())
        return;
    reg.edit->setModified(false);
    if (m_session->isRunning())
        return;

    bool ok;
    unsigned value = reg.edit->text().trimmed().toUInt(&ok, 16);
    unsigned maxValue = (1u << m_session->getTarget()->getRegInfo(reg.index).bits) - 1;
    if (ok && value <= maxValue && value != reg.value) {
        m_session->setReg(reg.index, value);
        // the window sees the change and refreshes all views (a flag may depend on it)
        reg.value = m_session->getTarget()->getReg(reg.index);
    }
    reg.edit->setText(hex(reg.value, reg.digits));
    reg.edit->setModified(false);
    updateColors();
}


void DbgRegsView::commitCheck(Reg& reg, bool checked)
{
    if (m_session->isRunning())
        return;
    m_session->setReg(reg.index, checked ? 1 : 0);
    reg.value = m_session->getTarget()->getReg(reg.index);
    reg.check->setChecked(reg.value != 0);
    updateColors();
}


void DbgRegsView::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    // the theme has changed: the fields have own palettes derived from ours
    if (event->type() == QEvent::PaletteChange)
        updateColors();
}
