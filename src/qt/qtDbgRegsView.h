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

#ifndef QTDBGREGSVIEW_H
#define QTDBGREGSVIEW_H

#include <vector>

#include <QWidget>

class QCheckBox;
class QLineEdit;
class DebugSession;


// Registers and flags of the CPU, generated from the debug target's register table:
// registers are hex fields, 1-bit registers and flags are check boxes. A value is
// changed with Enter (or by leaving the field) and a check box click. Values changed
// since the previous stop are highlighted. While the emulation runs the fields are
// disabled and keep the values of the last stop.
class DbgRegsView : public QWidget
{
    Q_OBJECT

public:
    DbgRegsView(DebugSession* session, QWidget* parent = nullptr);

public slots:
    void onStateChanged(bool newStop);
    void onRunStateChanged(bool running);

protected:
    void changeEvent(QEvent* event) override;

private:
    struct Reg {
        int index;              // in the target's register table
        int digits;             // hex digits, 0 for a check box
        QLineEdit* edit = nullptr;
        QCheckBox* check = nullptr;
        unsigned value = 0;     // shown
        unsigned prevValue = 0; // at the previous stop
    };

    DebugSession* m_session;
    std::vector<Reg> m_regs;
    bool m_running = true;
    bool m_hasPrev = false;     // there was a previous stop to compare with
    bool m_hasState = false;

    void createWidgets();
    void showValues();
    void updateColors();
    void updateEnabled();
    void commitEdit(Reg& reg);
    void commitCheck(Reg& reg, bool checked);
};


#endif // QTDBGREGSVIEW_H
