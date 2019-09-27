/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2019
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

#ifndef QTPLATFORMCONFIG_H
#define QTPLATFORMCONFIG_H

#include <QDialog>

namespace Ui {
class PlatformConfigDialog;
}

class PlatformConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PlatformConfigDialog(QWidget *parent = nullptr);
    ~PlatformConfigDialog();

    static bool hasConfig(QString platform);
    bool configure(QString platform);
    void getConfig(QString platform, std::map<std::string, std::string>& defines);

private:
    Ui::PlatformConfigDialog *ui;
};

#endif // QTPLATFORMCONFIG_H
