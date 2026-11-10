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

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

#include "qtDbgBreakpoints.h"
#include "qtDbgStyle.h"

#include "../DebugSession.h"


namespace {

QString hex(unsigned value, int digits)
{
    return QString("%1").arg(value, digits, 16, QChar('0')).toUpper();
}

const BreakpointType c_allTypes[] = {BT_EXEC, BT_READ, BT_WRITE, BT_ACSESS, BT_PORT_READ, BT_PORT_WRITE, BT_PORT_ACSESS};

QString typeLongName(BreakpointType type)
{
    switch (type) {
    case BT_EXEC:        return QObject::tr("Execution");
    case BT_READ:        return QObject::tr("Memory read");
    case BT_WRITE:       return QObject::tr("Memory write");
    case BT_ACSESS:      return QObject::tr("Memory access");
    case BT_PORT_READ:   return QObject::tr("Port read");
    case BT_PORT_WRITE:  return QObject::tr("Port write");
    case BT_PORT_ACSESS: return QObject::tr("Port access");
    }
    return QString();
}

QString addrText(const BreakpointDesc& bp)
{
    return getBreakpointClass(bp.type) == BC_IO && bp.addr < 0x100 ? hex(bp.addr, 2) : hex(bp.addr, 4);
}

// The table that tells whether a cell is being edited
class BpTableView : public QTableView
{
public:
    BpTableView(QWidget* parent) : QTableView(parent) {}
    bool isEditing() const {return state() == EditingState;}
};

bool isEditing(QTableView* table)
{
    return static_cast<BpTableView*>(table)->isEditing();
}

} // namespace


QString dbgBreakpointTypeShortName(BreakpointType type)
{
    switch (type) {
    case BT_EXEC:        return "X";
    case BT_READ:        return "R";
    case BT_WRITE:       return "W";
    case BT_ACSESS:      return "A";
    case BT_PORT_READ:   return "IR";
    case BT_PORT_WRITE:  return "IW";
    case BT_PORT_ACSESS: return "IA";
    }
    return QString();
}


//##### DbgBreakpointsModel ####

DbgBreakpointsModel::DbgBreakpointsModel(DebugSession* session, QObject* parent)
    : QAbstractTableModel(parent), m_session(session)
{
}


void DbgBreakpointsModel::refresh()
{
    beginResetModel();
    m_bps.clear();
    m_session->getBreakpoints()->list(m_bps);
    endResetModel();
}


const BreakpointDesc* DbgBreakpointsModel::getBreakpoint(int row) const
{
    return row >= 0 && row < int(m_bps.size()) ? &m_bps[row] : nullptr;
}


int DbgBreakpointsModel::findRow(int id) const
{
    for (int i = 0; i < int(m_bps.size()); i++)
        if (m_bps[i].id == id)
            return i;
    return -1;
}


int DbgBreakpointsModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : int(m_bps.size());
}


int DbgBreakpointsModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : COL_COUNT;
}


QVariant DbgBreakpointsModel::data(const QModelIndex& index, int role) const
{
    const BreakpointDesc* bp = getBreakpoint(index.row());
    if (!bp)
        return QVariant();

    if (role == Qt::CheckStateRole && index.column() == COL_ENABLED)
        return bp->enabled ? Qt::Checked : Qt::Unchecked;

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case COL_ENABLED: return bp->id;
        case COL_TYPE:    return dbgBreakpointTypeShortName(bp->type);
        case COL_ADDR:    return addrText(*bp);
        case COL_SKIP:    return bp->skipCount;
        case COL_HITS:    return bp->hitCount;
        case COL_OWNER:   return bp->owner == DC_EXTERNAL ? QString("MCP") : QString("UI");
        case COL_COMMENT: return QString::fromUtf8(bp->comment.c_str());
        }
    }

    if (role == Qt::ToolTipRole && index.column() == COL_TYPE)
        return typeLongName(bp->type);

    if (role == Qt::ToolTipRole && index.column() == COL_SKIP)
        return tr("Stop on every (skip + 1)-th hit; %1 hit(s) to skip before the next stop").arg(bp->remaining);

    if (role == Qt::FontRole && (index.column() == COL_ADDR || index.column() == COL_TYPE))
        return dbgFont();

    if (role == Qt::TextAlignmentRole && (index.column() == COL_SKIP || index.column() == COL_HITS))
        return int(Qt::AlignRight | Qt::AlignVCenter);

    return QVariant();
}


QVariant DbgBreakpointsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();
    switch (section) {
    case COL_ENABLED: return tr("#");
    case COL_TYPE:    return tr("Type");
    case COL_ADDR:    return tr("Address");
    case COL_SKIP:    return tr("Skip");
    case COL_HITS:    return tr("Hits");
    case COL_OWNER:   return tr("Owner");
    case COL_COMMENT: return tr("Comment");
    }
    return QVariant();
}


Qt::ItemFlags DbgBreakpointsModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (index.column() == COL_ENABLED)
        f |= Qt::ItemIsUserCheckable;
    if (index.column() == COL_SKIP || index.column() == COL_COMMENT)
        f |= Qt::ItemIsEditable;
    return f;
}


bool DbgBreakpointsModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    const BreakpointDesc* bp = getBreakpoint(index.row());
    if (!bp)
        return false;
    BreakpointManager* bpm = m_session->getBreakpoints();
    int id = bp->id;

    bool ok = false;
    if (role == Qt::CheckStateRole && index.column() == COL_ENABLED)
        ok = bpm->setEnabled(id, value.toInt() == Qt::Checked);
    else if (role == Qt::EditRole && index.column() == COL_SKIP) {
        int skip = value.toInt(&ok);
        ok = ok && skip >= 0 && bpm->setSkipCount(id, skip);
    } else if (role == Qt::EditRole && index.column() == COL_COMMENT)
        ok = bpm->setComment(id, value.toString().toUtf8().constData());

    if (ok) {
        bpm->get(id, m_bps[index.row()]);
        emit dataChanged(this->index(index.row(), 0), this->index(index.row(), COL_COUNT - 1));
    }
    return ok;
}


//##### DbgAddBreakpointDialog ####

DbgAddBreakpointDialog::DbgAddBreakpointDialog(DebugSession* session, uint16_t addr, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Add breakpoint"));

    m_typeCombo = new QComboBox(this);
    BreakpointManager* bpm = session->getBreakpoints();
    for (BreakpointType type : c_allTypes)
        if (bpm->isSupported(type))
            m_typeCombo->addItem(dbgBreakpointTypeShortName(type) + " - " + typeLongName(type), int(type));

    m_addrEdit = new QLineEdit(hex(addr, 4), this);
    m_addrEdit->setFont(dbgFont());
    m_addrEdit->setMaxLength(4);
    m_addrEdit->selectAll();

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QFormLayout* layout = new QFormLayout(this);
    layout->addRow(tr("Type:"), m_typeCombo);
    layout->addRow(tr("Address (hex):"), m_addrEdit);
    layout->addRow(buttons);
    m_addrEdit->setFocus();
}


BreakpointType DbgAddBreakpointDialog::getType()
{
    return BreakpointType(m_typeCombo->currentData().toInt());
}


bool DbgAddBreakpointDialog::getAddr(uint16_t& addr)
{
    bool ok;
    unsigned value = m_addrEdit->text().trimmed().toUInt(&ok, 16);
    if (!ok || value > 0xFFFF)
        return false;
    addr = uint16_t(value);
    return true;
}


//##### DbgBreakpointsPanel ####

DbgBreakpointsPanel::DbgBreakpointsPanel(DebugSession* session, QWidget* parent)
    : QWidget(parent), m_session(session)
{
    m_model = new DbgBreakpointsModel(session, this);

    m_table = new BpTableView(this);
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed |
                             QAbstractItemView::AnyKeyPressed);
    m_table->verticalHeader()->hide();
    m_table->verticalHeader()->setDefaultSectionSize(DbgMetrics(dbgFont()).lineHeight + 6);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setHighlightSections(false);
    m_table->installEventFilter(this);
    connect(m_table, &QTableView::activated, this, &DbgBreakpointsPanel::onActivated);

    QPushButton* addButton = new QPushButton(tr("Add..."), this);
    QPushButton* removeButton = new QPushButton(tr("Remove"), this);
    QPushButton* removeAllButton = new QPushButton(tr("Remove all"), this);
    connect(addButton, &QPushButton::clicked, this, &DbgBreakpointsPanel::onAdd);
    connect(removeButton, &QPushButton::clicked, this, &DbgBreakpointsPanel::onRemove);
    connect(removeAllButton, &QPushButton::clicked, this, &DbgBreakpointsPanel::onRemoveAll);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);
    buttonLayout->addWidget(removeAllButton);
    buttonLayout->addStretch(1);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(buttonLayout);
    layout->addWidget(m_table, 1);

    refresh();
    m_table->resizeColumnsToContents();
}


// Rereads the list keeping the selection (by breakpoint id)
void DbgBreakpointsPanel::refresh()
{
    std::vector<int> selectedIds;
    for (const QModelIndex& index : m_table->selectionModel()->selectedRows())
        if (const BreakpointDesc* bp = m_model->getBreakpoint(index.row()))
            selectedIds.push_back(bp->id);
    int currentId = -1;
    if (const BreakpointDesc* bp = m_model->getBreakpoint(m_table->currentIndex().row()))
        currentId = bp->id;
    int currentColumn = m_table->currentIndex().column();

    m_model->refresh();

    for (int id : selectedIds) {
        int row = m_model->findRow(id);
        if (row >= 0)
            m_table->selectionModel()->select(m_model->index(row, 0),
                                              QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
    int row = m_model->findRow(currentId);
    if (row >= 0)
        m_table->selectionModel()->setCurrentIndex(m_model->index(row, currentColumn < 0 ? 0 : currentColumn),
                                                   QItemSelectionModel::NoUpdate);
}


void DbgBreakpointsPanel::onBreakpointsChanged()
{
    // don't disturb an edit in progress; the list is reread after it
    if (isEditing(m_table))
        return;
    refresh();
}


// Hit counts change on stops
void DbgBreakpointsPanel::onStateChanged(bool newStop)
{
    if (newStop)
        onBreakpointsChanged();
}


void DbgBreakpointsPanel::onAdd()
{
    uint16_t addr = m_addressProvider ? m_addressProvider() : 0;
    DbgAddBreakpointDialog dialog(m_session, addr, this);
    if (dialog.exec() != QDialog::Accepted || !dialog.getAddr(addr))
        return;
    int id = m_session->getBreakpoints()->add(dialog.getType(), addr, DC_UI);
    refresh();
    int row = m_model->findRow(id);
    if (row >= 0)
        m_table->selectRow(row);
}


void DbgBreakpointsPanel::onRemove()
{
    std::vector<int> ids;
    for (const QModelIndex& index : m_table->selectionModel()->selectedRows())
        if (const BreakpointDesc* bp = m_model->getBreakpoint(index.row()))
            ids.push_back(bp->id);
    BreakpointManager* bpm = m_session->getBreakpoints();
    for (int id : ids)
        bpm->remove(id);
    refresh();
}


void DbgBreakpointsPanel::onRemoveAll()
{
    m_session->getBreakpoints()->clear();
    refresh();
}


// Enter or double click on the type or address: show the address
void DbgBreakpointsPanel::onActivated(const QModelIndex& index)
{
    const BreakpointDesc* bp = m_model->getBreakpoint(index.row());
    if (!bp || (index.column() != DbgBreakpointsModel::COL_TYPE && index.column() != DbgBreakpointsModel::COL_ADDR))
        return;
    BreakpointClass bpClass = getBreakpointClass(bp->type);
    if (bpClass == BC_EXEC)
        emit showInCode(bp->addr);
    else if (bpClass == BC_MEM)
        emit showInMemory(bp->addr);
}


bool DbgBreakpointsPanel::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_table && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Delete && !isEditing(m_table)) {
            onRemove();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Insert) {
            onAdd();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}
