/*
 *  Emu80 v. 4.x
*  © Viktor Pykhonin <pyk@mail.ru>, 2019-2020
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
#include <QFileDialog>
#include <QStringList>

#include "qtConfigWidget.h"
#include "ui_qtApogeyConfig.h"

#include "../Pal.h"

ConfigWidget::ConfigWidget(QWidget* parent) : QWidget(parent)
{
    //m_settings = new QSettings();

    m_baseDir.setPath(QString::fromUtf8(palGetBasePath().c_str()));


}


ConfigWidget* ConfigWidget::create(QString platformName)
{
    ConfigWidget* widget;
    widget = new ApogeyConfigWidget();
    widget->m_platform = platformName;
    return widget;
}


void ConfigWidget::optBegin()
{
    m_settings.beginGroup(m_platform + "-config");
}


void ConfigWidget::optSave(QString option, QString value)
{
    m_settings.setValue(option, value);
}


QVariant ConfigWidget::optLoad(QString option)
{
    return m_settings.value(option, m_defValues[option]);
}


void ConfigWidget::optEnd()
{
    m_settings.endGroup();
}


QString ConfigWidget::selectFile(QString fileName, bool dirMode, QString title, QString filter)
{
    QStringList filters;
    filters.append(filter);
    filters.append(tr("All Files (*.*)"));
    QDir oldDir;

    if (!dirMode) {
        QFileInfo fileInfo;
        fileInfo.setFile(m_baseDir, fileName);
        oldDir = fileInfo.absoluteDir();
    } else
        oldDir.setPath(QDir(fileName).absolutePath());
    if (!oldDir.exists())
        oldDir = m_baseDir;

    QFileDialog fileDialog(this);
    fileDialog.setWindowTitle(title);
    fileDialog.setAcceptMode(QFileDialog::QFileDialog::AcceptOpen);
    fileDialog.setFileMode(dirMode ? QFileDialog::Directory : QFileDialog::ExistingFiles);
    fileDialog.setNameFilters(filters);
    fileDialog.setDirectory(oldDir);
    if (fileDialog.exec() == QDialog::Accepted) {
        fileName = fileDialog.selectedFiles().constFirst(); // Qt >= 5.6
        QString relFileName = m_baseDir.relativeFilePath(fileName);
        return relFileName.startsWith("..") ? fileName : relFileName;
    }
    return "";
}


void ConfigWidget::selectFile(QLabel* label, bool dirMode, QString title, QString filter)
{
    QString file = selectFile(label->text(), dirMode, title, filter);
    if (file != "")
        label->setText(file);
}


// ######## Apogey (& RK) config widget ########

ApogeyConfigWidget::ApogeyConfigWidget(QWidget *parent) :
    ConfigWidget(parent),
    ui(new Ui::ApogeyConfigWidget)
{
    ui->setupUi(this);

    connect(ui->romDiskToolButton, SIGNAL(clicked()), this, SLOT(onSelectRomDisk()));
    connect(ui->sdToolButton, SIGNAL(clicked()), this, SLOT(onSelectSd()));
}

void ApogeyConfigWidget::loadConfig()
{
    if (m_platform == "apogey") {
        m_defValues["CFG_ROMDISK_FILE"] = "apogey/romdisk512.bin";
        m_defValues["CFG_SD_DIR"] = "apogey/sdcard";
    } else { // if (m_platform == "rk86") {
        m_defValues["CFG_ROMDISK_FILE"] = "rk86/romdisk.bin";
        m_defValues["CFG_SD_DIR"] = "rk86/sdcard";
    }

    optBegin();
    /*QString extStorage = optLoad("CFG_EXT_STORAGE").toString();
    ui->sdRadioButton->setChecked(extStorage == "SD");
    ui->romDiskRadioButton->setChecked(extStorage == "ROMDISK");*/
    ui->romDiskLabel->setText(optLoad("CFG_ROMDISK_FILE").toString());
    ui->sdLabel->setText(optLoad("CFG_SD_DIR").toString());
    optEnd();
}

void ApogeyConfigWidget::saveConfig()
{
    optBegin();
    //QString extStorage = ui->sdRadioButton->isChecked() ? "SD" : "ROMDISK";
    //optSave("CFG_EXT_STORAGE", extStorage);
    optSave("CFG_ROMDISK_FILE", ui->romDiskLabel->text());
    optSave("CFG_SD_DIR", ui->sdLabel->text());
    optEnd();
}

void ApogeyConfigWidget::setDefaults()
{
    //ui->romDiskRadioButton->setChecked(true);
    ui->romDiskLabel->setText(m_defValues["CFG_ROMDISK_FILE"]);
    ui->sdLabel->setText(m_defValues["CFG_SD_DIR"]);
}


void ApogeyConfigWidget::onSelectRomDisk()
{
    selectFile(ui->romDiskLabel, false, tr("Select ROM Disk File"), tr("ROM Disk Files") + " (*.bin *.rom)");
}


void ApogeyConfigWidget::onSelectSd()
{
    selectFile(ui->sdLabel, true, tr("Select SD Card Folder"));
}
