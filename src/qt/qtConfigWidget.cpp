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

#include "qtConfigWidget.h"

/*ConfigWidget::ConfigWidget()
{

}*/


ConfigWidget* ConfigWidget::create(QString platformName)
{
    return new SampleConfigWidget();
}


/*SampleConfigWidget::SampleConfigWidget(QWidget *parent)
{
    //
}*/


 #include <QPalette>
void SampleConfigWidget::setConfig()
{
    QPalette Pal(palette());
    Pal.setColor(QPalette::Background, Qt::black);
    setAutoFillBackground(true);
    setPalette(Pal);
}


void SampleConfigWidget::getConfig()
{

}



