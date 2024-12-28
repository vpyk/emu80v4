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

#ifndef QTSETTINGSDIALOG_H
#define QTSETTINGSDIALOG_H

#include <QDialog>
#include <QString>
#include <QMap>

#include <string>

class MainWindow;

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

    void execute();
    void initConfig();
    void updateConfig();
    void saveConfig();
    void resetPlatformOptions();
    void resetAllOptions();

    QString getOptionValue(QString option);
    //void setOptionValue(QString option, QString value);

public slots:
    void onResetShowHelp();

private slots:

    void on_volumeSpinBox_valueChanged(int arg1);
    void on_presetComboBox_currentIndexChanged(int index);
    void on_fixedSizeRadioButton_toggled(bool checked);
    void on_autoSizeRadioButton_toggled(bool checked);
    void on_userSizeRadioButton_toggled(bool checked);
    void on_fixedScaleRadioButton_toggled(bool checked);
    void on_stretchRadioButton_toggled(bool checked);
    void on_stretchPropIntRadioButton_toggled(bool checked);
    void on_aspectCheckBox_toggled(bool checked);
    void on_applyPushButton_clicked();
    void on_speedUpCheckBox_toggled(bool checked);
    void on_okPushButton_clicked();
    void on_limitFpsCheckBox_toggled(bool checked);
    void on_volumeSlider_valueChanged(int value);
    void on_fixedScaleComboBox_currentIndexChanged(int index);
    void on_smoothingNearestRadioButton_toggled(bool checked);
    void on_smoothingSharpRadioButton_toggled(bool checked);
    void on_smoothingBilinearRadioButton_toggled(bool checked);

private:
    Ui::SettingsDialog *ui;

    MainWindow* m_mainWindow;
    QMap<QString, QString> m_options;
    QMap<QString, QString> m_initialOptions;

    std::string m_platform;
    QString m_platformGroup;

    QString getRunningConfigValue(QString option);
    void setRunningConfigValue(QString option, QString value);
    void loadRunningConfigValue(QString option, bool force = false);

    bool m_presetComboBoxEventsAllowed = true;
    void adjustPresetComboBoxState();

    void clearConfig();
    void readRunningConfig();
    void writeInitialSavedConfig();
    void fillControlValues();
    void loadSavedConfig();
    void saveStoredConfig();
    void saveRunningConfig();
};

#endif // QTSETTINGSDIALOG_H
