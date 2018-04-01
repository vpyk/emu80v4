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

#include "EmuTypes.h"
#include "PalKeys.h"
#include "Shortcuts.h"

SysReq TranslateKeyToSysReq(PalKeyCode key, bool isKeyDown, bool isAltPressed)
{
    if (isKeyDown && isAltPressed)
        switch (key) {
            case PK_F11:
                return SR_RESET;
            case PK_ENTER:
                return SR_FULLSCREEN;
            case PK_F10:
                return SR_CLOSE;
            case PK_F9:
                return SR_CHPLATFORM;
            case PK_F12:
                return SR_CONFIG;
            case PK_F1:
                return SR_HELP;
            case PK_F3:
                return SR_LOADRUN;
            case PK_L:
                return SR_LOAD;
            case PK_W:
                return SR_LOADWAV;
            case PK_D:
                return SR_DEBUG;
            case PK_X:
                return SR_EXIT;
            case PK_Q:
                return SR_QUERTY;
            case PK_J:
                return SR_JCUKEN;
            case PK_K:
                return SR_SMART;
            case PK_F:
                return SR_FONT;
            case PK_C:
                return SR_COLOR;
            case PK_A:
                return SR_DISKA;
            case PK_B:
                return SR_DISKB;
            case PK_1:
                return SR_1X;
            case PK_2:
                return SR_2X;
            case PK_3:
                return SR_3X;
            case PK_0:
                return SR_FIT;
            case PK_M:
                return SR_MAXIMIZE;
            case PK_R:
                return SR_ASPECTCORRECTION;
            case PK_P:
                return SR_PAUSE;
            case PK_H:
                return SR_SCREENSHOT;
            case PK_U:
                return SR_MUTE;
            default:
                return SR_NONE;
        } else {
            if (key == PK_END)
                return isKeyDown ? SR_SPEEDUP : SR_SPEEDNORMAL;
            if (key == PK_PAUSEBRK && isKeyDown)
                return SR_PAUSE;
            else
                return SR_NONE;
        }
}
