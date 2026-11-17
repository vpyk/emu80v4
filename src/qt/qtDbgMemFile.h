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

#ifndef QTDBGMEMFILE_H
#define QTDBGMEMFILE_H

#include <QDialog>
#include <QString>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class DebugSession;


// The last values of the dialog, kept by the debugger window until the emulator exits
struct DbgMemFileParams {
    bool valid = false; // not set yet: the defaults are used
    int memory = 0;
    unsigned start = 0;
    unsigned end = 0;   // inclusive, for saving
    QString fileName;
};


// Saving a memory range to a binary file or loading a binary file into memory.
// The file is written or read on OK; on an error the dialog stays open.
class DbgMemFileDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode {
        MODE_SAVE,
        MODE_LOAD
    };

    // memory and addr: the defaults (the active dump and its cursor) if params aren't set
    DbgMemFileDialog(DebugSession* session, Mode mode, DbgMemFileParams& params,
                     int memory, unsigned addr, QWidget* parent = nullptr);

    void accept() override;

private slots:
    void onMemoryChanged();
    void onWholeMemory();
    void onBrowse();
    void updateState();

private:
    DebugSession* m_session;
    Mode m_mode;
    DbgMemFileParams& m_params;

    QComboBox* m_memoryCombo = nullptr;
    QLineEdit* m_startEdit;
    QLineEdit* m_endEdit = nullptr;
    QLineEdit* m_fileEdit;
    QLabel* m_infoLabel;
    QPushButton* m_okButton;
    QString m_confirmedFile; // chosen in the file dialog, which has asked about overwriting

    unsigned memorySize();
    int addrDigits();
    bool getAddr(QLineEdit* edit, unsigned& addr);
    // the current values are valid; info: the size or an error (error is set then)
    bool check(QString& info, bool& error);
    bool save();
    bool load();
};


#endif // QTDBGMEMFILE_H
