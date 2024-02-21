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

#include <QSettings>
#include <QFileDialog>
#include <QStringList>

#include "qtConfigWidget.h"
#include "ui_qtApogeyConfig.h"
#include "ui_qtKorvetConfig.h"
#include "ui_qtVectorConfig.h"

#include "../Pal.h"

ConfigWidget::ConfigWidget(QWidget* parent) : QWidget(parent)
{
    //m_settings = new QSettings();

    m_baseDir.setPath(QString::fromUtf8(palGetBasePath().c_str()));


}


ConfigWidget* ConfigWidget::create(QString platformName)
{
    ConfigWidget* widget;
    if (platformName == "korvet")
        widget = new KorvetConfigWidget();
    else if (platformName == "vector")
        widget = new VectorConfigWidget();
    else // if (platformName == "apogey" || platformName == "rk86" || platformName == "kr04" || platformName == "mikrosha" || platformName == "mikro80" || platformName == "ut8880")
    widget = new ApogeyConfigWidget();

    widget->m_platform = platformName;
    widget->tune();
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
    connect(ui->sdToolButton, SIGNAL(clicked()), this, SLOT(onSelectSdDir()));
    connect(ui->sdosToolButton, SIGNAL(clicked()), this, SLOT(onSelectSdImg()));
}


void ApogeyConfigWidget::tune()
{
    ui->sdEnableCheckBox->setVisible(m_platform == "kr04");
    ui->romDiskEnableCheckBox->setVisible(m_platform == "mikrosha");
    ui->sdosGroupBox->setVisible(m_platform == "rk86");
    ui->romDiskGroupBox->setVisible(m_platform != "kr04" && m_platform != "partner");
    ui->sdGroupBox->setVisible(m_platform != "mikrosha" && m_platform != "mikro80" && m_platform != "ut88");
}


void ApogeyConfigWidget::loadConfig()
{
    if (m_platform == "apogey") {
        m_defValues["CFG_ROMDISK_FILE"] = "apogey/romdisk512.bin";
        m_defValues["CFG_SD_DIR"] = "apogey/sdcard";
    } else if (m_platform == "rk86") {
        m_defValues["CFG_ROMDISK_FILE"] = "rk86/romdisk.bin";
        m_defValues["CFG_SD_DIR"] = "rk86/sdcard";
        m_defValues["CFG_SD_IMG"] = "rk86/sd_rk86.img";
        m_defValues["CFG_SD_TYPE"] = "HWMPVV";
    } else if (m_platform == "kr04") {
        m_defValues["CFG_SD_DIR"] = "kr04/sdcard";
        m_defValues["CFG_EXT_STORAGE"] = "SD";
    } else if (m_platform == "mikrosha") {
        m_defValues["CFG_ROMDISK_FILE"] = "mikrosha/extrom.bin";
        m_defValues["CFG_EXT_STORAGE"] = "NONE";
    } else if (m_platform == "partner") {
        m_defValues["CFG_SD_DIR"] = "partner/sdcard";
    } else if (m_platform == "mikro80") {
        m_defValues["CFG_ROMDISK_FILE"] = "mikro80/romdisk.bin";
    } else { // if (m_platform == "ut88") {
        m_defValues["CFG_ROMDISK_FILE"] = "ut88/romdisk.bin";
    }

    optBegin();
    /*QString extStorage = optLoad("CFG_EXT_STORAGE").toString();
    ui->sdRadioButton->setChecked(extStorage == "SD");
    ui->romDiskRadioButton->setChecked(extStorage == "ROMDISK");*/
    ui->romDiskLabel->setText(optLoad("CFG_ROMDISK_FILE").toString());
    ui->sdLabel->setText(optLoad("CFG_SD_DIR").toString());
    ui->sdosLabel->setText(optLoad("CFG_SD_IMG").toString());
    ui->sdEnableCheckBox->setChecked(optLoad("CFG_EXT_STORAGE").toString() == "SD");
    ui->romDiskEnableCheckBox->setChecked(optLoad("CFG_EXT_STORAGE").toString() == "ROMDISK");

    ui->hwmPvvRadioButton->setChecked(optLoad("CFG_SD_TYPE").toString() == "HWMPVV");
    ui->hwmPvvVv55RadioButton->setChecked(optLoad("CFG_SD_TYPE").toString() == "HWMPVV_VV55");
    ui->n8vemRadioButton->setChecked(optLoad("CFG_SD_TYPE").toString() == "N8VEM");
    ui->n8vemVv55RadioButton->setChecked(optLoad("CFG_SD_TYPE").toString() == "N8VEM_VV55");
    ui->msxRadioButton->setChecked(optLoad("CFG_SD_TYPE").toString() == "MSX");
    optEnd();
}

void ApogeyConfigWidget::saveConfig()
{
    optBegin();
    //QString extStorage = ui->sdRadioButton->isChecked() ? "SD" : "ROMDISK";
    //optSave("CFG_EXT_STORAGE", extStorage);
    if (m_platform == "mikrosha") {
        optSave("CFG_EXT_STORAGE", ui->romDiskEnableCheckBox->isChecked() ? "ROMDISK" : "NONE");
        optSave("CFG_ROMDISK_FILE", ui->romDiskLabel->text());
    } else if (m_platform == "kr04") {
        optSave("CFG_EXT_STORAGE", ui->sdEnableCheckBox->isChecked() ? "SD" : "NONE");
        optSave("CFG_SD_DIR", ui->sdLabel->text());
    } else if (m_platform == "partner") {
        optSave("CFG_SD_DIR", ui->sdLabel->text());
    } else if (m_platform == "mikro80") {
        optSave("CFG_ROMDISK_FILE", ui->romDiskLabel->text());
    } else {
        optSave("CFG_ROMDISK_FILE", ui->romDiskLabel->text());
        optSave("CFG_SD_DIR", ui->sdLabel->text());
        if (m_platform != "apogey") {
            optSave("CFG_SD_IMG", ui->sdosLabel->text());
            QString sdType;
            if (ui->hwmPvvVv55RadioButton->isChecked())
                sdType = "HWMPVV_VV55";
            else if (ui->n8vemRadioButton->isChecked())
                sdType = "N8VEM";
            else if (ui->n8vemVv55RadioButton->isChecked())
                sdType = "N8VEM_VV55";
            else if (ui->msxRadioButton->isChecked())
                sdType = "MSX";
            else //if (ui->hwmPvvRadioButton->isChecked())
                sdType = "HWMPVV";
            optSave("CFG_SD_TYPE", sdType);
        }
    }
    optEnd();
}

void ApogeyConfigWidget::setDefaults()
{
    //ui->romDiskRadioButton->setChecked(true);
    ui->romDiskLabel->setText(m_defValues["CFG_ROMDISK_FILE"]);
    ui->sdLabel->setText(m_defValues["CFG_SD_DIR"]);
    ui->sdosLabel->setText(m_defValues["CFG_SD_IMG"]);
    ui->sdEnableCheckBox->setChecked(m_defValues["CFG_EXT_STORAGE"] == "SD");
    ui->romDiskEnableCheckBox->setChecked(m_defValues["CFG_EXT_STORAGE"] == "ROMDISK");
    ui->hwmPvvRadioButton->setChecked(m_defValues["CFG_SD_TYPE"] == "HWMPVV");
}


void ApogeyConfigWidget::onSelectRomDisk()
{
    selectFile(ui->romDiskLabel, false, tr("Select ROM Disk File"), tr("ROM Disk Files") + " (*.bin *.rom)");
}


void ApogeyConfigWidget::onSelectSdDir()
{
    selectFile(ui->sdLabel, true, tr("Select SD Card Folder"));
}


void ApogeyConfigWidget::onSelectSdImg()
{
    selectFile(ui->sdosLabel, false, tr("Select SD Card Image File"));
}


// ######## Korvet config widget ########

KorvetConfigWidget::KorvetConfigWidget(QWidget *parent) :
    ConfigWidget(parent),
    ui(new Ui::KorvetConfigWidget)
{
    ui->setupUi(this);
}


void KorvetConfigWidget::loadConfig()
{
    m_defValues["CFG_PPI3"] = "AY";

    optBegin();
    ui->psgCheckBox->setChecked(optLoad("CFG_PPI3").toString() == "AY");
    optEnd();
}

void KorvetConfigWidget::saveConfig()
{
    optBegin();
    optSave("CFG_PPI3", ui->psgCheckBox->isChecked() ? "AY" : "NONE");
    optEnd();
}

void KorvetConfigWidget::setDefaults()
{
    ui->psgCheckBox->setChecked(m_defValues["CFG_PPI3"] == "AY");
}


// ######## Vector config widget ########

VectorConfigWidget::VectorConfigWidget(QWidget *parent) :
    ConfigWidget(parent),
    ui(new Ui::VectorConfigWidget)
{
    ui->setupUi(this);
}


void VectorConfigWidget::loadConfig()
{
    m_defValues["CFG_EDD"] = "EDD";

    optBegin();
    QString val = optLoad("CFG_EDD").toString();
    optEnd();

    if (val == "NONE")
        ui->noneRadioButton->setChecked(true);
    else if (val == "EDD")
        ui->edd1RadioButton->setChecked(true);
    else if (val == "EDDx2")
        ui->edd2RadioButton->setChecked(true);
    else if (val == "ERAM")
        ui->eramRadioButton->setChecked(true);
}

void VectorConfigWidget::saveConfig()
{
    QString val;
    if (ui->noneRadioButton->isChecked())
        val = "NONE";
    else if (ui->edd1RadioButton->isChecked())
        val = "EDD";
    else if (ui->edd2RadioButton->isChecked())
        val = "EDDx2";
    else if (ui->eramRadioButton->isChecked())
        val = "ERAM";

    optBegin();
    optSave("CFG_EDD", val);
    optEnd();
}

void VectorConfigWidget::setDefaults()
{
    ui->edd1RadioButton->setChecked(m_defValues["CFG_EDD"] == "EDD");
}

