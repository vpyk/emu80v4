/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2024
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
#include <QMessageBox>

#include "qtSettingsDialog.h"
#include "ui_qtSettingsDialog.h"

#include "qtMainWindow.h"

#include "../EmuCalls.h"


SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    m_mainWindow = (MainWindow*)parent;
    m_platform = m_mainWindow->getPlatformObjectName();
}


SettingsDialog::~SettingsDialog()
{
    delete ui;
}


void SettingsDialog::initConfig()
{
    clearConfig();
    readRunningConfig();
    writeInitialSavedConfig();
    loadSavedConfig();
    saveRunningConfig();
    fillControlValues();
}


void SettingsDialog::updateConfig()
{
    readRunningConfig();
    fillControlValues();
}


void SettingsDialog::saveConfig()
{
    saveStoredConfig();
}


void SettingsDialog::execute()
{
    updateConfig();
    show();
}


QString SettingsDialog::getRunningConfigValue(QString option)
{
    int dotPos = option.lastIndexOf(".");
    std::string obj = option.mid(0, dotPos).toUtf8().constData();
    std::string prop = option.mid(dotPos + 1).toUtf8().constData();

    if (obj != "platform") {
        if (obj != "emulation" && obj != "wavReader")
            obj = m_platform + "." + obj;
    } else
        obj = m_platform;

    return QString::fromUtf8(emuGetPropertyValue(obj, prop).c_str());
}


void SettingsDialog::setRunningConfigValue(QString option, QString value)
{
    int dotPos = option.lastIndexOf(".");
    std::string obj = option.mid(0, dotPos).toUtf8().constData();
    std::string prop = option.mid(dotPos + 1).toUtf8().constData();
    std::string val = value.toUtf8().constData();

    if (obj != "platform") {
        if (obj != "emulation" && obj != "wavReader")
            obj = m_platform + "." + obj;
    } else
        obj = m_platform;

    emuSetPropertyValue(obj, prop, val);
}


void SettingsDialog::loadRunningConfigValue(QString option, bool force)
{
    QString value = getRunningConfigValue(option);
    if (value != "" || force)
        m_options[option] = value;
}


// Очищает настройки в m_options
void SettingsDialog::clearConfig()
{
    m_options.clear();
}


// Считывает текущие работающие настройки в m_options
void SettingsDialog::readRunningConfig()
{
    m_platform = m_mainWindow->getPlatformObjectName();
    m_platformGroup = QString::fromUtf8(m_mainWindow->getPlatformGroupName().c_str());

    loadRunningConfigValue("emulation.volume");
    loadRunningConfigValue("emulation.debug8080MnemoUpperCase");
    loadRunningConfigValue("emulation.debugZ80MnemoUpperCase");
    loadRunningConfigValue("emulation.debugForceZ80Mnemonics");
    loadRunningConfigValue("emulation.debugSwapF5F9");
    loadRunningConfigValue("emulation.debugResetKeys");
    loadRunningConfigValue("window.windowStyle");
    loadRunningConfigValue("window.frameScale");
    loadRunningConfigValue("window.smoothing");
    loadRunningConfigValue("window.aspectCorrection");
    loadRunningConfigValue("window.squarePixels");
    loadRunningConfigValue("window.wideScreen");
    loadRunningConfigValue("window.customScreenFormat");
    loadRunningConfigValue("window.fieldsMixing");
    loadRunningConfigValue("window.defaultWindowWidth");
    loadRunningConfigValue("window.defaultWindowHeight");
    loadRunningConfigValue("cpu.debugOnHalt");
    loadRunningConfigValue("cpu.debugOnIllegalCmd");
    loadRunningConfigValue("crtRenderer.colorMode");
    loadRunningConfigValue("crtRenderer.altRenderer");
    loadRunningConfigValue("crtRenderer.visibleArea");
    loadRunningConfigValue("wavReader.channel");
    loadRunningConfigValue("wavReader.speedUpFactor");
    loadRunningConfigValue("tapeGrp.enabled");
    loadRunningConfigValue("tapeSoundSource.muted");
    loadRunningConfigValue("tapeInHook.suspendAfterResetForMs");
    loadRunningConfigValue("loader.allowMultiblock");
    loadRunningConfigValue("kbdLayout.layout");
    loadRunningConfigValue("kbdLayout.numpadJoystick");
    loadRunningConfigValue("kbdLayout.downAsNumpad5");
    loadRunningConfigValue("kbdLayout.upAsNumpad5");
    loadRunningConfigValue("keyboard.matrix");
    loadRunningConfigValue("platform.codePage");
    loadRunningConfigValue("platform.fastReset");
    loadRunningConfigValue("psgSoundSource.mixing");
    loadRunningConfigValue("diskA.readOnly");
    loadRunningConfigValue("diskB.readOnly");
    loadRunningConfigValue("diskC.readOnly");
    loadRunningConfigValue("diskD.readOnly");
    loadRunningConfigValue("hdd.readOnly");
    loadRunningConfigValue("diskA.autoMount");
    loadRunningConfigValue("diskB.autoMount");
    loadRunningConfigValue("diskC.autoMount");
    loadRunningConfigValue("diskD.autoMount");
    loadRunningConfigValue("hdd.autoMount");
    loadRunningConfigValue("diskA.permanentFileName", m_options.contains("diskA.autoMount"));
    loadRunningConfigValue("diskB.permanentFileName", m_options.contains("diskB.autoMount"));
    loadRunningConfigValue("diskC.permanentFileName", m_options.contains("diskC.autoMount"));
    loadRunningConfigValue("diskD.permanentFileName", m_options.contains("diskD.autoMount"));
    loadRunningConfigValue("hdd.permanentFileName", m_options.contains("hdd.autoMount"));
    loadRunningConfigValue("ramDisk.autoLoad");
    loadRunningConfigValue("ramDisk.autoSave");
    loadRunningConfigValue("ramDisk.permanentFileName", m_options.contains("ramDisk.autoLoad"));
    loadRunningConfigValue("ramDisk2.autoLoad");
    loadRunningConfigValue("ramDisk2.autoSave");
    loadRunningConfigValue("ramDisk2.permanentFileName", m_options.contains("ramDisk2.autoLoad"));
}


// Обновляет ini-файл, записывая из m_options значения настроек, которых еще нет в ini-файле
void SettingsDialog::writeInitialSavedConfig()
{
    m_initialOptions = m_options;

    QSettings settings;

    settings.beginGroup(m_platformGroup);
    foreach (QString option, m_options.keys()) {
        QString value = m_options.value(option);
        if (/*value != "" &&*/ option.left(10) != "emulation." && option != "locale" && option != "showHelp" &&
                option != "maxFps" && option != "sampleRate" && option != "vsync" && option != "limitFps" &&
                !settings.contains(option))
            settings.setValue(option, value);
    }
    settings.endGroup();
}


// Загружает в m_options настройки из ini-файла
void SettingsDialog::loadSavedConfig()
{
    QSettings settings;

    settings.beginGroup("system");
    m_options["locale"] = settings.value("locale").toString();
    //m_options["glDriver"] = settings.value("glDriver").toString();
    m_options["maxFps"] = settings.value("maxFps").toString();
    m_options["limitFps"] = settings.value("limitFps").toString();
    m_options["vsync"] = settings.value("vsync").toString();
    m_options["sampleRate"] = settings.value("sampleRate").toString();
    m_options["showHelp"] = settings.value("showHelp", "yes").toString();
    m_options["preserveSize"] = settings.value("preserveSize", "yes").toString();
    settings.endGroup();

    settings.beginGroup(m_platformGroup);
    foreach (QString option, m_options.keys()) {
        if (settings.contains(option))
            m_options[option] = settings.value(option).toString();
    }
    settings.endGroup();

    settings.beginGroup("common");
    foreach (QString option, m_options.keys()) {
        if (settings.contains(option))
            m_options[option] = settings.value(option).toString();
    }
    settings.endGroup();
}


// Устанавливает параметры элементов оправления в соответствии с m_options
void SettingsDialog::fillControlValues()
{
    QString val;

    // Language
    val = m_options["locale"];
    if (val == "en")
        ui->langComboBox->setCurrentIndex(1);
    else if (val == "ru")
        ui->langComboBox->setCurrentIndex(2);
    else // if (val == "system")
        ui->langComboBox->setCurrentIndex(0);

    // Show help
    ui->showHelpCheckBox->setChecked(m_options["showHelp"] == "yes");

    // Preserve size
    ui->preserveSizeCheckBox->setChecked(m_options["preserveSize"] == "yes");

    // OpenGL driver
    /*val = m_options["glDriver"];
    ui->driverComboBox->setCurrentIndex(val == "es" ? 1 : 0);*/

    // Frame rate
    int fr = m_options["maxFps"].toInt();
    ui->fpsSpinBox->setValue(fr);

    // Frame rate limitation
    bool limitFps = m_options["limitFps"] == "true";
    ui->limitFpsCheckBox->setChecked(limitFps);

    // VSync
    val = m_options["vsync"];
    ui->vsyncCheckBox->setChecked(val == "yes");

    // Sample rate
    val = m_options["sampleRate"];
    ui->srComboBox->setCurrentText(val);

    // Debugger options
    ui->upper8080checkBox->setChecked(m_options["emulation.debug8080MnemoUpperCase"] == "yes");
    ui->upperZ80checkBox->setChecked(m_options["emulation.debugZ80MnemoUpperCase"] == "yes");
    ui->forceZ80checkBox->setChecked(m_options["emulation.debugForceZ80Mnemonics"] == "yes");
    ui->swapF5F9checkBox->setChecked(m_options["emulation.debugSwapF5F9"] == "yes");
    ui->resetKeysCheckBox->setChecked(m_options["emulation.debugResetKeys"] == "yes");

    // Debugger code page
    val = m_options["platform.codePage"];
    ui->rkCodePageRadioButton->setChecked(val == "rk");
    ui->koi8CodePageRadioButton->setChecked(val == "koi8");

    // Volume
    val = m_options["emulation.volume"];
    int volume = val.toInt();
    ui->muteCheckBox->setVisible(false); //временно!
    ui->volumeSpinBox->setValue(volume);
    ui->volumeSlider->setValue(volume);
    ui->warnLabel->setVisible(volume > 6);

    // Window style
    val = m_options["window.windowStyle"];
    ui->autoSizeRadioButton->setChecked(val == "autosize");
    ui->userSizeRadioButton->setChecked(val == "resizable");
    ui->fixedSizeRadioButton->setChecked(val == "fixed");
    bool fixed = ui->fixedSizeRadioButton->isChecked();
    ui->widthLineEdit->setEnabled(fixed);
    ui->heightLineEdit->setEnabled(fixed);

    ui->widthLineEdit->setText(m_options["window.defaultWindowWidth"]);
    ui->heightLineEdit->setText(m_options["window.defaultWindowHeight"]);

    // Scaling
    val = m_options["window.frameScale"];
    if (val == "1x") {
        ui->fixedScaleRadioButton->setChecked(true);
        ui->fixedScaleComboBox->setCurrentIndex(0);
    } else if (val == "1.5x") {
        ui->fixedScaleRadioButton->setChecked(true);
        ui->fixedScaleComboBox->setCurrentIndex(1);
    } else if (val == "2x") {
        ui->fixedScaleRadioButton->setChecked(true);
        ui->fixedScaleComboBox->setCurrentIndex(2);
    } else if (val == "2.5x") {
        ui->fixedScaleRadioButton->setChecked(true);
        ui->fixedScaleComboBox->setCurrentIndex(3);
    } else if (val == "3x") {
        ui->fixedScaleRadioButton->setChecked(true);
        ui->fixedScaleComboBox->setCurrentIndex(4);
    } else if (val == "4x") {
        ui->fixedScaleRadioButton->setChecked(true);
        ui->fixedScaleComboBox->setCurrentIndex(5);
    } else if (val == "5x") {
        ui->fixedScaleRadioButton->setChecked(true);
        ui->fixedScaleComboBox->setCurrentIndex(6);
    } else if (val == "2x3") {
        ui->fixedScaleRadioButton->setChecked(true);
        ui->fixedScaleComboBox->setCurrentIndex(2);
    } else if (val == "3x5") {
        ui->fixedScaleRadioButton->setChecked(true);
        ui->fixedScaleComboBox->setCurrentIndex(4);
    } else if (val == "4x6") {
        ui->fixedScaleRadioButton->setChecked(true);
        ui->fixedScaleComboBox->setCurrentIndex(5);
    } else {
        ui->fixedScaleRadioButton->setChecked(false);
        ui->stretchRadioButton->setChecked(val == "fit");
        ui->stretchPropIntRadioButton->setChecked(val == "bestfit");
    }

    // Smoothing
    val = m_options["window.smoothing"];
    ui->smoothingNearestRadioButton->setChecked(val == "nearest");
    ui->smoothingSharpRadioButton->setChecked(val == "sharp");
    ui->smoothingBilinearRadioButton->setChecked(val == "bilinear");

    // Aspect ratio
    val = m_options["window.aspectCorrection"];
    ui->aspectCheckBox->setChecked(val == "yes");

    // Adaptive aspect ratio
    val = m_options["window.squarePixels"];
    ui->adaptArRadioButton->setChecked(val == "no");
    ui->sqrArRadioButton->setChecked(val == "yes");

    // Wide screen
    val = m_options["window.wideScreen"];
    ui->wideComboBox->setCurrentIndex(val == "yes" ? 1 : val == "custom" ? 2 : 0);

    // Screen format
    val = m_options["window.customScreenFormat"];
    ui->customFormatLineEdit->setText(val);

    // Debug on HLT
    val = m_options["cpu.debugOnHalt"];
    ui->debugHltCheckBox->setChecked(val == "yes");

    // Debug on illegal instruction
    val = m_options["cpu.debugOnIllegalCmd"];
    ui->debugIllegalCheckBox->setChecked(val == "yes");

    // Color mode
    val = m_options.value("crtRenderer.colorMode", "");
    ui->colorGroupBox->setVisible(m_platformGroup == "apogey" || m_platformGroup == "rk86" || m_platformGroup == "spec" || m_platformGroup == "sp580");
    if (m_platformGroup == "apogey") {
        ui->color1RadioButton->setText(tr("Color"));
        ui->color2RadioButton->setVisible(false);
        ui->bwRadioButton->setChecked(val == "mono");
        ui->color1RadioButton->setChecked(val == "color");
    } else if (m_platformGroup == "rk86") {
        ui->color1RadioButton->setText(tr("Color mode 1 (Tolkalin)"));
        ui->color2RadioButton->setVisible(true);
        ui->color2RadioButton->setText(tr("Color mode 2 (Akimenko)"));
        ui->bwRadioButton->setChecked(val == "mono");
        ui->color1RadioButton->setChecked(val == "color1");
        ui->color2RadioButton->setChecked(val == "color2");
    } else if (m_platformGroup == "spec" || m_platformGroup == "sp580") {
        ui->color1RadioButton->setText(tr("4-color mode"));
        ui->color2RadioButton->setVisible(true);
        ui->color2RadioButton->setText(tr("8-color mode"));
        ui->bwRadioButton->setChecked(val == "mono");
        ui->color1RadioButton->setChecked(val == "4color");
        ui->color2RadioButton->setChecked(val == "8color");
    }

    // Fields mixing
    val = m_options["window.fieldsMixing"];
    ui->mixingOffRadioButton->setChecked(val == "none");
    ui->mixingMixRadioButton->setChecked(val == "mix");
    ui->mixingInterlaceRadioButton->setChecked(val == "interlace");
    ui->mixingScanlineRadioButton->setChecked(val == "scanline");

    // Visible area
    val = m_options.value("crtRenderer.visibleArea", "");
    ui->cropCheckBox->setEnabled(val != "");
    ui->cropCheckBox->setChecked(val == "yes");

    // Alternate font
    val = m_options.value("crtRenderer.altRenderer", "");
    ui->altFontCheckBox->setEnabled(val != "");
    ui->altFontCheckBox->setChecked(val == "yes");

    // WAV channel
    val = m_options["wavReader.channel"];
    if (val == "left")
        ui->wavChannelComboBox->setCurrentIndex(0);
    else if (val == "right")
        ui->wavChannelComboBox->setCurrentIndex(1);
    else //if (val == "mix")
        ui->wavChannelComboBox->setCurrentIndex(2);

    // Speed up factor
    val = m_options["wavReader.speedUpFactor"];
    int suf = val.toInt();
    ui->speedUpSpinBox->setValue(suf);
    ui->speedUpCheckBox->setChecked(suf != 1);

    // Tape redirect
    val = m_options.value("tapeGrp.enabled", "");
    ui->tapeRedirectCheckBox->setVisible(val != "");
    ui->tapeRedirectCheckBox->setChecked(val == "yes");

    // Mute tape
    val = m_options.value("tapeSoundSource.muted", "");
    ui->muteTapeCheckBox->setVisible(val != "");
    ui->muteTapeCheckBox->setChecked(val == "yes");

    // Suppress file opening on reset
    val = m_options.value("tapeInHook.suspendAfterResetForMs", "");
    ui->tapeSuppressOpeningCheckBox->setVisible(val != "");
    int sar = val.toInt();
    ui->tapeSuppressOpeningCheckBox->setChecked(sar > 0);

    // Allow multiblock
    val = m_options.value("loader.allowMultiblock", "");
    ui->tapeMultiblockCheckBox->setVisible(val != "");
    ui->tapeMultiblockCheckBox->setChecked(val == "yes");

    // Keyboard layout
    val = m_options["kbdLayout.layout"];
    ui->qwertyRadioButton->setChecked(val == "qwerty");
    ui->jcukenRadioButton->setChecked(val == "jcuken");
    ui->smartRadioButton->setChecked(val == "smart");

    // Numpad joystick
    val = m_options.value("kbdLayout.numpadJoystick", "");
    ui->numpadJoystickCheckBox->setVisible(val != "");
    ui->numpadJoystickCheckBox->setChecked(val == "yes");

    // Down as numpad 5
    val = m_options.value("kbdLayout.downAsNumpad5", "");
    ui->downAsNumpad5CheckBox->setVisible(val != "");
    ui->downAsNumpad5CheckBox->setChecked(val == "yes");

    // Up as numpad 5
    val = m_options.value("kbdLayout.upAsNumpad5", "");
    ui->upAsNumpad5CheckBox->setVisible(val != "");
    ui->upAsNumpad5CheckBox->setChecked(val == "yes");

    // Keyboard matrix
    val = m_options.value("keyboard.matrix", "");
    ui->kbdTypeGroupBox->setVisible(val != "");
    ui->kbdOriginalRadioButton->setChecked(val == "original");
    ui->kbdRamfosRadioButton->setChecked(val == "ramfos");
    ui->kbdLikRadioButton->setChecked(val == "lik");
    ui->kbdEurekaRadioButton->setChecked(val == "eureka");

    // Fast reset
    val = m_options.value("platform.fastReset", "");
    ui->fastResetCheckBox->setVisible(val != "");
    ui->fastResetCheckBox->setChecked(val == "yes");

    // AY Stereo
    val = m_options.value("psgSoundSource.mixing", "");
    ui->ayStereoCheckBox->setVisible(val != "");
    ui->ayStereoCheckBox->setChecked(val == "stereo");
}


void SettingsDialog::on_volumeSlider_valueChanged(int value)
{
    ui->volumeSpinBox->setValue(value);
    ui->warnLabel->setVisible(value > 6);
}


void SettingsDialog::on_volumeSpinBox_valueChanged(int arg1)
{
    ui->volumeSlider->setValue(arg1);
}


void SettingsDialog::on_presetComboBox_currentIndexChanged(int index)
{
    if (!m_presetComboBoxEventsAllowed)
        return;
    switch (index) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        ui->autoSizeRadioButton->setChecked(true);
        ui->fixedScaleRadioButton->setChecked(true);
        ui->fixedScaleComboBox->setCurrentIndex(index - 1);
        break;
    case 8:
        ui->userSizeRadioButton->setChecked(true);
        ui->stretchRadioButton->setChecked(true);
        break;
    }
}


void SettingsDialog::on_fixedSizeRadioButton_toggled(bool checked)
{
    ui->widthLineEdit->setEnabled(checked);
    ui->heightLineEdit->setEnabled(checked);
    if (checked) {
        ui->stretchRadioButton->setEnabled(true);
        ui->stretchPropIntRadioButton->setEnabled(true);
        adjustPresetComboBoxState();
    }
}


void SettingsDialog::adjustPresetComboBoxState()
{
    m_presetComboBoxEventsAllowed = false;
    if (ui->autoSizeRadioButton->isChecked() &&
            ui->fixedScaleRadioButton->isChecked() &&
            ui->fixedScaleComboBox->currentIndex() == 0)
        ui->presetComboBox->setCurrentIndex(1);
    else if (ui->autoSizeRadioButton->isChecked() &&
             ui->fixedScaleRadioButton->isChecked() &&
             ui->fixedScaleComboBox->currentIndex() == 1)
         ui->presetComboBox->setCurrentIndex(2);
    else if (ui->autoSizeRadioButton->isChecked() &&
             ui->fixedScaleRadioButton->isChecked() &&
             ui->fixedScaleComboBox->currentIndex() == 2)
         ui->presetComboBox->setCurrentIndex(3);
    else if (ui->autoSizeRadioButton->isChecked() &&
             ui->fixedScaleRadioButton->isChecked() &&
             ui->fixedScaleComboBox->currentIndex() == 3)
         ui->presetComboBox->setCurrentIndex(4);
    else if (ui->autoSizeRadioButton->isChecked() &&
             ui->fixedScaleRadioButton->isChecked() &&
             ui->fixedScaleComboBox->currentIndex() == 4)
         ui->presetComboBox->setCurrentIndex(5);
    else if (ui->autoSizeRadioButton->isChecked() &&
             ui->fixedScaleRadioButton->isChecked() &&
             ui->fixedScaleComboBox->currentIndex() == 5)
        ui->presetComboBox->setCurrentIndex(6);
    else if (ui->autoSizeRadioButton->isChecked() &&
             ui->fixedScaleRadioButton->isChecked() &&
             ui->fixedScaleComboBox->currentIndex() == 6)
        ui->presetComboBox->setCurrentIndex(7);
    else if (ui->userSizeRadioButton->isChecked() &&
             ui->stretchRadioButton->isChecked())
         ui->presetComboBox->setCurrentIndex(8);
    else
        ui->presetComboBox->setCurrentIndex(0);
    m_presetComboBoxEventsAllowed = true;
}


void SettingsDialog::on_autoSizeRadioButton_toggled(bool checked)
{
    if (checked) {
        ui->fixedScaleRadioButton->setChecked(true);
        ui->stretchRadioButton->setEnabled(false);
        ui->stretchPropIntRadioButton->setEnabled(false);
        adjustPresetComboBoxState();
    }
}


void SettingsDialog::on_userSizeRadioButton_toggled(bool checked)
{
    if (checked) {
        ui->stretchRadioButton->setEnabled(true);
        ui->stretchPropIntRadioButton->setEnabled(true);
        adjustPresetComboBoxState();
    }
}


void SettingsDialog::on_fixedScaleRadioButton_toggled(bool checked)
{
    ui->fixedScaleComboBox->setEnabled(checked);
    if (checked)
        adjustPresetComboBoxState();
}


void SettingsDialog::on_stretchRadioButton_toggled(bool checked)
{
    if (checked)
        adjustPresetComboBoxState();
}


void SettingsDialog::on_stretchPropIntRadioButton_toggled(bool checked)
{
    if (checked)
        adjustPresetComboBoxState();
}


void SettingsDialog::on_aspectCheckBox_toggled(bool checked)
{
    Q_UNUSED(checked);
    adjustPresetComboBoxState();
}


void SettingsDialog::on_fixedScaleComboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    adjustPresetComboBoxState();
}


void SettingsDialog::on_speedUpCheckBox_toggled(bool checked)
{
    ui->speedUpSpinBox->setEnabled(checked);
}


void SettingsDialog::on_applyPushButton_clicked()
{
    QString val;
    bool rebootFlag = false;

    switch (ui->langComboBox->currentIndex()) {
        case 0:
            val = "system";
            break;
        case 1:
            val = "en";
            break;
        case 2:
            val = "ru";
            break;
        default:
            val = "";
            break;
    }
    if (val != m_options["locale"]) {
        m_options["locale"] = val;
        rebootFlag = true;
    }

    /*switch (ui->driverComboBox->currentIndex()) {
        case 0:
            val = "desktop";
            break;
        case 1:
            val = "es";
            break;
        default:
            val = "";
            break;
    }
    if (val != m_options["glDriver"]) {
        m_options["glDriver"] = val;
        rebootFlag = true;
    }*/

    m_options["showHelp"] = ui->showHelpCheckBox->isChecked() ? "yes" : "no";
    m_options["preserveSize"] = ui->preserveSizeCheckBox->isChecked() ? "yes" : "no";
    m_options["limitFps"] = ui->limitFpsCheckBox->isChecked() ? "true" : "false";
    m_options["maxFps"] = QString::number(ui->fpsSpinBox->value());

    val = ui->vsyncCheckBox->isChecked() ? "yes" : "no";
    if (val != m_options["vsync"]) {
        m_options["vsync"] = val;
        rebootFlag = true;
    }

    val = ui->srComboBox->currentText(); // !!!
    if (val != m_options["sampleRate"]) {
        m_options["sampleRate"] = val;
        rebootFlag = true;
    }

    m_options["emulation.debug8080MnemoUpperCase"] = ui->upper8080checkBox->isChecked() ? "yes" : "no";
    m_options["emulation.debugZ80MnemoUpperCase"] = ui->upperZ80checkBox->isChecked() ? "yes" : "no";
    m_options["emulation.debugForceZ80Mnemonics"] = ui->forceZ80checkBox->isChecked() ? "yes" : "no";
    m_options["emulation.debugSwapF5F9"] = ui->swapF5F9checkBox->isChecked() ? "yes" : "no";
    m_options["emulation.debugResetKeys"] = ui->resetKeysCheckBox->isChecked() ? "yes" : "no";

    val = "";
    if (ui->rkCodePageRadioButton->isChecked())
        val = "rk";
    else if (ui->koi8CodePageRadioButton->isChecked())
        val = "koi8";
    m_options["platform.codePage"] = val;

    val = QString::number(ui->volumeSlider->value());
    m_options["emulation.volume"] = val;

    val = "";
    if (ui->autoSizeRadioButton->isChecked())
        val = "autosize";
    else if (ui->userSizeRadioButton->isChecked())
        val = "resizable";
    else if (ui->fixedSizeRadioButton->isChecked())
        val = "fixed";
    m_options["window.windowStyle"] = val;

    m_options["window.defaultWindowWidth"] = ui->widthLineEdit->text();
    m_options["window.defaultWindowHeight"] = ui->heightLineEdit->text();

    val = "";
    if (ui->fixedScaleRadioButton->isChecked())
        switch (ui->fixedScaleComboBox->currentIndex()) {
        case 0:
            val = "1x";
            break;
        case 1:
            val = "1.5x";
            break;
        case 2:
            val = "2x";
            break;
        case 3:
            val = "2.5x";
            break;
        case 4:
            val = "3x";
            break;
        case 5:
            val = "4x";
            break;
        case 6:
            val = "5x";
            break;
        default:
            break;
        }
    else if (ui->stretchRadioButton->isChecked())
        val = "fit";
    else if (ui->stretchPropIntRadioButton->isChecked())
        val = "bestFit";
    m_options["window.frameScale"] = val;

    val = "";
    if (ui->smoothingNearestRadioButton->isChecked())
        val = "nearest";
    else if (ui->smoothingSharpRadioButton->isChecked())
        val = "sharp";
    else if (ui->smoothingBilinearRadioButton->isChecked())
        val = "bilinear";
    if (val != "")
        m_options["window.smoothing"] = val;

    val = "";
    if (ui->adaptArRadioButton->isChecked())
        val = "no";
    else if (ui->sqrArRadioButton->isChecked())
        val = "yes";
    if (val != "")
        m_options["window.squarePixels"] = val;

    m_options["window.aspectCorrection"] = ui->aspectCheckBox->isChecked() ? "yes" : "no";

    m_options["window.wideScreen"] = ui->wideComboBox->currentIndex() == 1 ? "yes" : ui->wideComboBox->currentIndex() == 2 ? "custom" : "no";

    m_options["window.customScreenFormat"] = ui->customFormatLineEdit->text();

    m_options["crtRenderer.visibleArea"] = ui->cropCheckBox->isChecked() ? "yes" : "no";

    m_options["cpu.debugOnHalt"] = ui->debugHltCheckBox->isChecked() ? "yes" : "no";

    m_options["cpu.debugOnIllegalCmd"] = ui->debugIllegalCheckBox->isChecked() ? "yes" : "no";

    val = "";
    if (ui->colorGroupBox->isVisible()) {
        if (ui->bwRadioButton->isChecked())
            val = "mono";
        else if (ui->color1RadioButton->isChecked()) {
            if (m_platformGroup == "apogey")
                val = "color";
            else if (m_platformGroup == "rk86")
                val = "color1";
            else if (m_platformGroup == "spec" || m_platformGroup == "sp580")
                val = "4color";
        } else if (ui->color2RadioButton->isChecked()) {
            if (m_platformGroup == "rk86")
                val = "color2";
            else if (m_platformGroup == "spec" || m_platformGroup == "sp580")
                val = "8color";
        }
    }
    if (val != "")
        m_options["crtRenderer.colorMode"] = val;

    val = "";
    if (ui->mixingGroupBox->isEnabled()) {
        if (ui->mixingOffRadioButton->isChecked())
            val = "none";
        else if (ui->mixingMixRadioButton->isChecked())
            val = "mix";
        else if (ui->mixingInterlaceRadioButton->isChecked())
            val = "interlace";
        else if (ui->mixingScanlineRadioButton->isChecked())
            val = "scanline";
    }
    if (val != "")
        m_options["window.fieldsMixing"] = val;

    if (ui->altFontCheckBox->isVisible())
        m_options["crtRenderer.altRenderer"] = ui->altFontCheckBox->isChecked() ? "yes" : "no";

    val = "";
    switch (ui->wavChannelComboBox->currentIndex()) {
    case 0:
        val = "left";
        break;
    case 1:
        val = "right";
        break;
    case 2:
        val = "mix";
        break;
    default:
        break;
    }
    if (val != "")
        m_options["wavReader.channel"] = val;

    if (!ui->speedUpCheckBox->isChecked())
        val = "1";
    else
        val = QString::number(ui->speedUpSpinBox->value());
    m_options["wavReader.speedUpFactor"] = val;

    if (ui->tapeRedirectCheckBox->isVisible())
        m_options["tapeGrp.enabled"] = ui->tapeRedirectCheckBox->isChecked() ? "yes" : "no";

    if (ui->muteTapeCheckBox->isVisible())
        m_options["tapeSoundSource.muted"] = ui->muteTapeCheckBox->isChecked() ? "yes" : "no";

    if (ui->tapeSuppressOpeningCheckBox->isVisible())
        m_options["tapeInHook.suspendAfterResetForMs"] = ui->tapeSuppressOpeningCheckBox->isChecked() ? "200" : "0"; // !!! добавить поле ms

    if (ui->tapeMultiblockCheckBox->isVisible())
        m_options["loader.allowMultiblock"] = ui->tapeMultiblockCheckBox->isChecked() ? "yes" : "no";

    if (ui->numpadJoystickCheckBox->isVisible())
        m_options["kbdLayout.numpadJoystick"] = ui->numpadJoystickCheckBox->isChecked() ? "yes" : "no";

    if (ui->downAsNumpad5CheckBox->isVisible())
        m_options["kbdLayout.downAsNumpad5"] = ui->downAsNumpad5CheckBox->isChecked() ? "yes" : "no";

    if (ui->upAsNumpad5CheckBox->isVisible())
        m_options["kbdLayout.upAsNumpad5"] = ui->upAsNumpad5CheckBox->isChecked() ? "yes" : "no";

    if (ui->fastResetCheckBox->isVisible())
        m_options["platform.fastReset"] = ui->fastResetCheckBox->isChecked() ? "yes" : "no";

    if (ui->ayStereoCheckBox->isVisible())
        m_options["psgSoundSource.mixing"] = ui->ayStereoCheckBox->isChecked() ? "stereo" : "mono";

    val = "";
    if (ui->qwertyRadioButton->isChecked())
        val = "qwerty";
    else if (ui->jcukenRadioButton->isChecked())
        val = "jcuken";
    else if (ui->smartRadioButton->isChecked())
        val = "smart";
    m_options["kbdLayout.layout"] = val;

    val = "";
    if (ui->kbdOriginalRadioButton->isChecked())
        val = "original";
    else if (ui->kbdRamfosRadioButton->isChecked())
        val = "ramfos";
    else if (ui->kbdLikRadioButton->isChecked())
        val = "lik";
    else if (ui->kbdEurekaRadioButton->isChecked())
        val = "eureka";
    m_options["keyboard.matrix"] = val;

    saveRunningConfig();
    saveStoredConfig();

    m_mainWindow->updateActions();

    if (rebootFlag) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Emu80: warning"));
        msgBox.setText(tr("Some settings will take effect after emulator restart"));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.addButton(QMessageBox::Ok);
        msgBox.exec();
    }
}


// Применяет текущие настройки из m_options
void SettingsDialog::saveRunningConfig()
{
    foreach (QString option, m_options.keys()) {
        QString value = m_options.value(option);
        if (value != "" && option != "locale" && option != "showHelp" && option != "maxFps" &&
                           option != "limitFps" && option != "sampleRate" && option != "vsync" && option != "preserveSize") {
            setRunningConfigValue(option, value);
        } else if (option == "maxFps") {
            setRunningConfigValue("emulation.maxFps", m_options.value("limitFps") == "true" ? value : 0);
        }
    }
}


// Сохраняет настройки из m_options в ini-файле
void SettingsDialog::saveStoredConfig()
{
    QSettings settings;

    settings.beginGroup(m_platformGroup);
    //foreach (QString option, m_options.keys()) {
    for (const auto& option: m_options.keys()) {
        QString value = m_options.value(option);
        if (option.left(10) != "emulation." /*&& value != ""*/ && option != "locale" && option != "showHelp" &&
                option != "maxFps" && option != "limitFps" && option != "sampleRate" && option != "vsync" && option != "preserveSize")
            settings.setValue(option, value);
    }
    settings.endGroup();

    settings.beginGroup("system");
    foreach (QString option, m_options.keys()) {
        QString value = m_options.value(option);
        if (option.left(10) != "emulation." && value != "" && (option == "locale" || option == "showHelp" || option == "maxFps" ||
                                                               option == "limitFps" || option == "sampleRate" || option == "vsync" || option == "preserveSize"))
            settings.setValue(option, value);
    }
    settings.endGroup();

    settings.beginGroup("common");
    foreach (QString option, m_options.keys()) {
        QString value = m_options.value(option);
        if (option.left(10) == "emulation.")
            settings.setValue(option, value);
    }
    settings.endGroup();
}


void SettingsDialog::resetPlatformOptions()
{
    QSettings settings;

    settings.beginGroup(m_platformGroup);
    settings.remove("");
    settings.endGroup();

    settings.beginGroup("last_files");
    settings.beginGroup(m_platformGroup);
    settings.remove("");
    settings.endGroup();
    settings.endGroup();

    settings.beginGroup("dirs");
    settings.beginGroup(m_platformGroup);
    settings.remove("");
    settings.endGroup();
    settings.endGroup();

    settings.sync();

    m_options.clear();
    m_options = m_initialOptions;
    fillControlValues();
    saveStoredConfig();
    saveRunningConfig();
    m_mainWindow->updateActions();
}


void SettingsDialog::resetAllOptions()
{
    QSettings settings;
    settings.clear();
    settings.sync();

    m_options.clear();
    m_options = m_initialOptions;
    fillControlValues();
    saveStoredConfig();
    saveRunningConfig();
    m_mainWindow->updateActions();
}


void SettingsDialog::on_okPushButton_clicked()
{
    on_applyPushButton_clicked();
    accept();
}


void SettingsDialog::on_limitFpsCheckBox_toggled(bool checked)
{
    ui->fpsSpinBox->setEnabled(checked);
}


QString SettingsDialog::getOptionValue(QString option)
{
    return m_options[option];
}


/*void SettingsDialog::setOptionValue(QString option, QString value)
{
    m_options[option] = value;
}*/


void SettingsDialog::onResetShowHelp()
{
    m_options["showHelp"] = "no";
    ui->showHelpCheckBox->setChecked(false);
    saveConfig();
}

void SettingsDialog::on_smoothingNearestRadioButton_toggled(bool checked)
{
    if (checked)
        adjustPresetComboBoxState();
}

void SettingsDialog::on_smoothingSharpRadioButton_toggled(bool checked)
{
    if (checked)
        adjustPresetComboBoxState();
}

void SettingsDialog::on_smoothingBilinearRadioButton_toggled(bool checked)
{
    if (checked)
        adjustPresetComboBoxState();
}
