/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2019-2022
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

#include <QCloseEvent>

#include "qtHelpDialog.h"
#include "ui_qtHelpDialog.h"


HelpDialog* HelpDialog::m_instance = nullptr;


HelpDialog::HelpDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HelpDialog)
{
    ui->setupUi(this);
}


HelpDialog::~HelpDialog()
{
    delete ui;
}


void HelpDialog::closeEvent(QCloseEvent *event)
{
    if (!m_instance->ui->showNextTimeCheckBox->isChecked())
        emit resetShowHelp();
    m_instance = nullptr;
    event->accept();
}


void HelpDialog::reject()
{
    close();
}


HelpDialog* HelpDialog::execute(QString helpFile, bool showHint)
{
    if (helpFile == "")
        return nullptr;

    if (!m_instance) {
        m_instance = new HelpDialog();
    }

    //m_instance->setAttribute(Qt::WA_DeleteOnClose);

    if (helpFile != "")
        m_instance->ui->textBrowser->setSource(QUrl::fromLocalFile(helpFile));
    else
        m_instance->ui->textBrowser->setHtml("<h2>Sorry, currently there is no help for this platrorm</h2>");

    m_instance->ui->showNextTimeCheckBox->setVisible(showHint);
    m_instance->ui->hintLabel->setVisible(showHint);

    m_instance->show();
    m_instance->activateWindow();

    return m_instance;
}


void HelpDialog::activate()
{
    if (m_instance)
        m_instance->activateWindow();
}
