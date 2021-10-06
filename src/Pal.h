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

// Platform Abstraction Layer

#ifdef PAL_QT
#include "qt/qtPal.h"
#endif // PAL_QT

#ifdef PAL_WX
#include "wx/wxPal.h"
#endif // PAL_WX

#ifdef PAL_SDL
#include "sdl/sdlPal.h"
#endif // PAL_SDL

#ifdef PAL_LITE
#include "lite/litePal.h"
#endif // PAL_LITE

#ifdef PAL_WASM
#include "wasm/wasmPal.h"
#endif // PAL_WASM

    #ifdef PAL_QT
    #ifdef __WIN32__
        #define EXE_NAME "Emu80qt.exe"
    #else
        #define EXE_NAME "Emu80qt"
    #endif
#else
    #ifdef __WIN32__
        #define EXE_NAME "Emu80.exe"
    #else
        #define EXE_NAME "Emu80"
    #endif
#endif

bool palInit(int& argc, char** argv);
void palQuit();
void palIdle();

