/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2024
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

#ifndef GLOBALS_H
#define GLOBALS_H

#include "Version.h"

#ifdef PAL_QT
    #define TARGET "/qt"
#elif defined PAL_WX
    #define TARGET ""
#else
    #ifdef PAL_WASM
        #define TARGET "/wasm"
    #else
        #define TARGET "/lite"
    #endif
#endif

#define VERSION VER_STR TARGET


#define MAX_SND_AMP 16383

class Emulation;
extern Emulation* g_emulation;

#endif // GLOBALS_H
