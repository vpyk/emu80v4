/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2026
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

#include "qtDebugger.h"
#include "qtDbgWindow.h"


IDebugger* palCreateDebugger(Platform* platform)
{
    return new QtDebugger(platform);
}


QtDebugger::QtDebugger(Platform* platform)
{
    m_window = new DbgWindow(platform);
}


QtDebugger::~QtDebugger()
{
    delete m_window;
}


void QtDebugger::startDebug()
{
    m_window->showStop();
}


void QtDebugger::refreshDebug()
{
    m_window->refreshStop();
}


void QtDebugger::update()
{
    m_window->refreshStop();
}


bool QtDebugger::isOpen()
{
    return m_window->isVisible();
}
