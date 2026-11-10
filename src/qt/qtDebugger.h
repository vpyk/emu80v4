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

#ifndef QTDEBUGGER_H
#define QTDEBUGGER_H

#include "../Debugger.h"

class Platform;
class DbgWindow;


// The Qt debugger UI of a platform: the adapter between the platform and the debugger
// window. The platform owns it and deletes it before its debug session.
class QtDebugger : public IDebugger
{
public:
    QtDebugger(Platform* platform);
    ~QtDebugger();

    void startDebug() override;
    void refreshDebug() override;
    void update() override;
    bool isOpen() override;

private:
    DbgWindow* m_window;
};


#endif // QTDEBUGGER_H
