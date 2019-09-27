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

#ifndef CONFIGWIDGET_H
#define CONFIGWIDGET_H

#include <QWidget>


class ConfigWidget : public QWidget
{
    Q_OBJECT

public:
    //ConfigWidget();

    virtual void setConfig() = 0;
    virtual void getConfig() = 0;

    static ConfigWidget* create(QString platformName);
};


class SampleConfigWidget : public ConfigWidget
{
    Q_OBJECT

public:
    //explicit SampleConfigWidget(QWidget *parent = nullptr);
    //~SampleConfigWidget();

    void setConfig() override;
    void getConfig() override;

private:
    //Ui::SampleConfigWidget *ui;
};




#endif // CONFIGWIDGET_H
