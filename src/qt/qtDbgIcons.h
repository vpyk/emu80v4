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

#ifndef QTDBGICONS_H
#define QTDBGICONS_H

#include <QIcon>


enum DbgIconKind {
    DI_RUN,
    DI_BREAK,
    DI_STEP_INTO,
    DI_STEP_OVER,
    DI_STEP_OUT,
    DI_RUN_TO_CURSOR,
    DI_SKIP
};


// Icons of the debugger actions, drawn in code: sharp at any scale, colors from
// the current application palette (light or dark theme)
QIcon dbgIcon(DbgIconKind kind);


#endif // QTDBGICONS_H
