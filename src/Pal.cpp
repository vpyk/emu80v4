/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017
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

#include "Pal.h"

bool palInit(int& argc, char** argv)
{
#ifdef PAL_SDL
    if (!palSdlInit())
        return false;
#endif

#ifdef PAL_WX
    if (!palWxInit(argc, argv)) {
 #ifdef PAL_SDL
        palSdlQuit();
 #endif
        return false;
    }
#endif

#ifdef PAL_QT
    if (!palQtInit(argc, argv))
        return false;
#endif

    return true;
}


void palQuit()
{
#ifdef PAL_WX
    palWxQuit();
#endif

#ifdef PAL_SDL
    palSdlQuit();
#endif

#ifdef PAL_QT
    palQtQuit();
#endif
}


void palIdle()
{
#ifdef PAL_WX
    palWxProcessMessages();
#endif // PAL_WX
}
