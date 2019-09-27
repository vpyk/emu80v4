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

#include "qtPlatformConfig.h"
#include "ui_qtPlatformConfig.h"

#include "qtConfigWidget.h"

PlatformConfigDialog::PlatformConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PlatformConfigDialog)
{
    ui->setupUi(this);
}


PlatformConfigDialog::~PlatformConfigDialog()
{
    delete ui;
}


bool PlatformConfigDialog::hasConfig(QString platform)
{
    return false; //platform == "apogey";
}


void PlatformConfigDialog::getConfig(QString platform, std::map<std::string, std::string>& defines)
{
    return;
}


bool PlatformConfigDialog::configure(QString platform)
{
    ConfigWidget* cw = ConfigWidget::create(platform);
    QSizePolicy sp;
    sp.setVerticalPolicy(QSizePolicy::Expanding);
    sp.setVerticalStretch(1);
    sp.setHorizontalPolicy(QSizePolicy::Expanding);
    sp.setHorizontalStretch(1);
    cw->setSizePolicy(sp);
    static_cast<QBoxLayout*>(layout())->insertWidget(0, cw);
    cw->setConfig();
    return exec() == QDialog::Accepted;
}
