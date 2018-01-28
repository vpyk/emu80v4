/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2018
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

#include <string>

#include "Pal.h"

#include "Emulation.h"


Emulation* g_emulation = nullptr;

int main (int argc, char** argv)
{
    if (!palInit(argc, argv))
        return 1;

    new Emulation(argc, argv); // g_emulation присваивается в конструкторе

    palStart();
    palExecute();

    delete g_emulation;

    palQuit();

    return 0;
}

