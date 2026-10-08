/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2018
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

#ifndef QTCHOOSEPLATFORMDIALOG_H
#define QTCHOOSEPLATFORMDIALOG_H

#include <QDialog>

#include <vector>

#include "../EmuTypes.h"

namespace Ui {
class ChoosePlatformDialog;
}

class ChoosePlatformDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChoosePlatformDialog(QWidget *parent = 0);
    ~ChoosePlatformDialog();

    bool execute(std::vector<PlatformInfo>& pi, int& pos, bool& newWnd, QString runFileName, bool setDef);

private slots:
    void on_platformListWidget_doubleClicked(const QModelIndex &index);

private:
    Ui::ChoosePlatformDialog *ui;
};

#endif // QTCHOOSEPLATFORMDIALOG_H
