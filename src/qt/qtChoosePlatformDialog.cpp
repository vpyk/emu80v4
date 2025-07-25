﻿/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2025
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

#include <QSettings>

#include "qtChoosePlatformDialog.h"
#include "ui_qtChoosePlatformDialog.h"

#include "qtMainWindow.h" // for INI_CODEC

ChoosePlatformDialog::ChoosePlatformDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChoosePlatformDialog)
{
    ui->setupUi(this);
}


ChoosePlatformDialog::~ChoosePlatformDialog()
{
    delete ui;
}


bool ChoosePlatformDialog::execute(std::vector<PlatformInfo>& pi, int& pos, bool& newWnd, QString runFileName, bool setDef)
{
    Q_UNUSED(runFileName)
    Q_UNUSED(setDef)

    QSettings settings;
    SET_INI_CODEC(settings);

    settings.beginGroup("system");
    bool selectedPlatformsOnly = settings.value("selectedPlatformsOnly").toString() == "yes";
    QStringList selectedPlatforms = settings.value("enabledPlatforms").toStringList();
    settings.endGroup();

    bool empty = true;

    //ui->defaultCheckBox->setChecked(setDef);
    for (auto it = pi.begin(); it != pi.end(); it++) {
        QString s = QString::fromUtf8((*it).platformName.c_str());

        QListWidgetItem* item = new QListWidgetItem(s);
        ui->platformListWidget->addItem(item);

        if (selectedPlatformsOnly) {
            QString platform = QString::fromUtf8((*it).objName.c_str());
            QString platformGroup = platform.left(platform.indexOf("."));
            if (!selectedPlatforms.contains(platformGroup))
                item->setHidden(true);
            else
                empty = false;
        }

        if ((*it).objName.find(".") == std::string::npos) {
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
        }
    }

    if (selectedPlatformsOnly && empty) {
        ui->okPushButton->setEnabled(false);
        ui->defaultCheckBox->setEnabled(false);
    }

    ui->platformListWidget->setCurrentRow(pos);
    if (exec() == QDialog::Accepted) {
        pos = ui->platformListWidget->currentRow();
        newWnd = false; //m_newWndCheckBox->GetValue();

        if (ui->defaultCheckBox->isChecked()) {
            QSettings settings;
            SET_INI_CODEC(settings);
            settings.beginGroup("system");
            settings.setValue("platform", QString::fromUtf8(pi[pos].objName.c_str()));
        }

        return true;
    }
    return false;
}

void ChoosePlatformDialog::on_platformListWidget_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index)
    accept();
}
