/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2019-2024
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

QString PlatformConfigDialog::getGroupName(QString name)
{
    return name.section('.', 0, 0);
}


bool PlatformConfigDialog::hasConfig(QString platform)
{
    QString groupName = getGroupName(platform);
    return groupName == "apogey" || groupName == "rk86" || groupName == "korvet" ||
           groupName == "kr04" || groupName == "vector" || groupName == "mikrosha" ||
           groupName == "partner" || groupName == "mikro80" || groupName == "ut88";
}


bool PlatformConfigDialog::configure(QString platform)
{
    m_configWidget = ConfigWidget::create(getGroupName(platform));
    connect(ui->defaultsButton, SIGNAL(clicked()), this, SLOT(onDefaults()));
    QSizePolicy sp;
    sp.setVerticalPolicy(QSizePolicy::Expanding);
    sp.setVerticalStretch(1);
    sp.setHorizontalPolicy(QSizePolicy::Expanding);
    sp.setHorizontalStretch(1);
    m_configWidget->setSizePolicy(sp);
    static_cast<QBoxLayout*>(layout())->insertWidget(0, m_configWidget);
    m_configWidget->loadConfig();
    bool res = exec() == QDialog::Accepted;
    if (res)
        m_configWidget->saveConfig();
    delete m_configWidget;
    m_configWidget = nullptr;
    return res;
}


void PlatformConfigDialog::onDefaults()
{
    if (m_configWidget)
        m_configWidget->setDefaults();
}
