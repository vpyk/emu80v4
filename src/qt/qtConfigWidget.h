/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2019-2026
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

#ifndef CONFIGWIDGET_H
#define CONFIGWIDGET_H

#include <QWidget>
#include <QSettings>
#include <QDir>
#include <QLabel>

class ConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigWidget(QWidget* parent = nullptr);
    ~ConfigWidget();

    virtual void loadConfig() = 0;
    virtual void saveConfig() = 0;
    virtual void setDefaults() = 0;

    virtual void tune() {}

    void tune(const QString& platform);

    static ConfigWidget* create(QString platformName);

protected:
    QSettings* m_settings = nullptr;
    QString m_platform;
    QDir m_baseDir;

    QMap<QString, QString> m_defValues;

    void optBegin();
    void optSave(QString option, QString value);
    QVariant optLoad(QString option);
    void optEnd();

    QString selectFile(QString fileName, bool dirMode, QString title = "Select file", QString filter = "*.*");
    void selectFile(QLabel* label, bool dirMode, QString title = "Select file", QString filter = "*.*");
};


namespace Ui {
    class ApogeyConfigWidget;
    class KorvetConfigWidget;
    class VectorConfigWidget;
    class ZxConfigWidget;
    class SpecConfigWidget;
    class OkeanConfigWidget;
    class PartnerConfigWidget;
    class OrionConfigWidget;
    class SpMx2ConfigWidget;
}

class ApogeyConfigWidget : public ConfigWidget
{
    Q_OBJECT

public:
    explicit ApogeyConfigWidget(QWidget *parent = nullptr);
    //~ApogeyConfigWidget();

    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

    void tune() override;

private slots:
    void onSelectRomDisk();
    void onSelectSdDir();
    void onSelectSdImg();

private:
    Ui::ApogeyConfigWidget *ui;
};


class KorvetConfigWidget : public ConfigWidget
{
    Q_OBJECT

public:
    explicit KorvetConfigWidget(QWidget *parent = nullptr);
    //~ApogeyConfigWidget();

    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

private:
    Ui::KorvetConfigWidget *ui;
};


class VectorConfigWidget : public ConfigWidget
{
    Q_OBJECT

public:
    explicit VectorConfigWidget(QWidget *parent = nullptr);
    //~ApogeyConfigWidget();

    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

private:
    Ui::VectorConfigWidget *ui;
};


class ZxConfigWidget : public ConfigWidget
{
    Q_OBJECT

public:
    explicit ZxConfigWidget(QWidget *parent = nullptr);

    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

private:
    Ui::ZxConfigWidget *ui;
};


class SpecConfigWidget : public ConfigWidget
{
    Q_OBJECT

public:
    explicit SpecConfigWidget(QWidget *parent = nullptr);

    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

private:
    Ui::SpecConfigWidget *ui;
};


class OkeanConfigWidget : public ConfigWidget
{
    Q_OBJECT

public:
    explicit OkeanConfigWidget(QWidget *parent = nullptr);

    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

private:
    Ui::OkeanConfigWidget *ui;
};


class PartnerConfigWidget : public ConfigWidget
{
    Q_OBJECT

public:
    explicit PartnerConfigWidget(QWidget *parent = nullptr);

    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

private:
    Ui::PartnerConfigWidget *ui;
};


class OrionConfigWidget : public ConfigWidget
{
    Q_OBJECT

public:
    explicit OrionConfigWidget(QWidget *parent = nullptr);

    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

private:
    Ui::OrionConfigWidget *ui;
};


class SpMx2ConfigWidget : public ConfigWidget
{
    Q_OBJECT

public:
    explicit SpMx2ConfigWidget(QWidget *parent = nullptr);

    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

private:
    Ui::SpMx2ConfigWidget *ui;
};


#endif // CONFIGWIDGET_H
