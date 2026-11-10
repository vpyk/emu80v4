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

#ifndef QTDBGBREAKPOINTS_H
#define QTDBGBREAKPOINTS_H

#include <functional>
#include <vector>

#include <QAbstractTableModel>
#include <QDialog>
#include <QWidget>

#include "../Breakpoints.h"

class QComboBox;
class QLineEdit;
class QTableView;
class DebugSession;


// All breakpoints of the debug session (of any type and owner) as a table.
// Enabled state, skip count and comment are edited in place.
class DbgBreakpointsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        COL_ENABLED,
        COL_TYPE,
        COL_ADDR,
        COL_SKIP,
        COL_HITS,
        COL_OWNER,
        COL_COMMENT,
        COL_COUNT
    };

    DbgBreakpointsModel(DebugSession* session, QObject* parent = nullptr);

    void refresh(); // rereads the list
    const BreakpointDesc* getBreakpoint(int row) const;
    int findRow(int id) const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

private:
    DebugSession* m_session;
    std::vector<BreakpointDesc> m_bps;
};


// Dialog for a new breakpoint: type (those the CPU supports) and address
class DbgAddBreakpointDialog : public QDialog
{
    Q_OBJECT

public:
    DbgAddBreakpointDialog(DebugSession* session, uint16_t addr, QWidget* parent = nullptr);

    BreakpointType getType();
    bool getAddr(uint16_t& addr);

private:
    QComboBox* m_typeCombo;
    QLineEdit* m_addrEdit;
};


// The breakpoints panel: the table and add/remove buttons
class DbgBreakpointsPanel : public QWidget
{
    Q_OBJECT

public:
    DbgBreakpointsPanel(DebugSession* session, QWidget* parent = nullptr);

    // Default address for a new breakpoint (e.g. the code view cursor)
    void setAddressProvider(std::function<uint16_t()> provider) {m_addressProvider = provider;}

signals:
    void showInCode(uint16_t addr);
    void showInMemory(uint16_t addr);

public slots:
    void onBreakpointsChanged();
    void onStateChanged(bool newStop);

private slots:
    void onAdd();
    void onRemove();
    void onRemoveAll();
    void onActivated(const QModelIndex& index);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    DebugSession* m_session;
    DbgBreakpointsModel* m_model;
    QTableView* m_table;
    std::function<uint16_t()> m_addressProvider;

    void refresh();
};


// One- or two-letter breakpoint type: X, R, W, A, IR, IW, IA
QString dbgBreakpointTypeShortName(BreakpointType type);


#endif // QTDBGBREAKPOINTS_H
