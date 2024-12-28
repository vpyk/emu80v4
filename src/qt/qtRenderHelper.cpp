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

#include <QApplication>
#include <QThread>

#include "qtRenderHelper.h"
#include "qtPaintWidget.h"
#include "qtMainWindow.h"

#include "../Pal.h"
#include "../EmuCalls.h"


extern QApplication* getApplication();

RenderHelper* g_renderHelper;


RenderHelper::RenderHelper(QWidget *parent) : QObject(parent)
{
    m_timer = new QTimer();
    m_timer->setInterval(2);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
}


RenderHelper::~RenderHelper()
{
    delete m_timer;
}


void RenderHelper::setPaintWidget(PaintWidget* widget)
{
    m_widget = widget;
}


void RenderHelper::onTimer()
{
    getApplication()->processEvents();

    if (!m_paused)
        emuEmulationCycle();
}


void RenderHelper::start()
{
    m_timer->start();
}


void RenderHelper::pause()
{
    m_paused = true;
    m_timer->stop();
}


void RenderHelper::resume()
{
    m_paused = false;
    m_timer->start();
}


void RenderHelper::addWindow(MainWindow* window)
{
    m_windowList.push_back(window);
}

void RenderHelper::removeWindow(MainWindow* window)
{
    m_windowList.remove(window);
}


void RenderHelper::updateConfig()
{
    for (auto it = m_windowList.begin(); it != m_windowList.end(); it++)
        (*it)->updateConfig();
}
