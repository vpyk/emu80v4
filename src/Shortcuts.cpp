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

#include "EmuTypes.h"
#include "PalKeys.h"
#include "Shortcuts.h"

SysReq TranslateKeyToSysReq(PalKeyCode key, bool isKeyDown, bool isAltPressed, bool isShiftPressed)
{
    if (isKeyDown && isAltPressed)
        switch (key) {
            case PK_F11:
                return SR_RESET;
            case PK_ENTER:
                return SR_FULLSCREEN;
            case PK_F10:
                return SR_CLOSE;
            case PK_F8:
                return SR_CHCONFIG;
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
                return isShiftPressed ? SR_DISKD : SR_DEBUG;
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
            case PK_V:
                return isShiftPressed ? SR_PASTE : SR_CROPTOVISIBLE;
            case PK_C:
                return isShiftPressed ? SR_DISKC : SR_COLOR;
            case PK_A:
                return SR_DISKA;
            case PK_B:
                return SR_DISKB;
            case PK_E:
                return isShiftPressed ? SR_OPENRAMDISK2 : SR_OPENRAMDISK;
            case PK_O:
                return isShiftPressed ? SR_SAVERAMDISK2AS : SR_SAVERAMDISKAS;
            case PK_1:
                return SR_1X;
            case PK_2:
                return SR_2X;
            case PK_3:
                return SR_3X;
            case PK_4:
                return SR_4X;
            case PK_5:
                return SR_5X;
            case PK_8:
                return SR_1_5X;
            case PK_9:
                return SR_2_5X;
            case PK_0:
                return SR_FIT;
            case PK_M:
                return isShiftPressed ? SR_MAXIMIZE : SR_MUTE;
            case PK_S:
                return SR_SMOOTHING;
            case PK_R:
                return SR_ASPECTCORRECTION;
            case PK_N:
                return SR_WIDESCREEN;
            case PK_P:
                return isShiftPressed ? SR_PRNCAPTURE : SR_PAUSE;
            case PK_H:
                return isShiftPressed ? SR_HDD : SR_SCREENSHOT;
            case PK_U:
                return SR_FASTRESET;
            case PK_INS:
                return isShiftPressed ? SR_COPYTXT : SR_NONE;
            case PK_T:
                return SR_TAPEHOOK;
            case PK_PGUP:
                return SR_SPEEDSTEPUP;
            case PK_PGDN:
                return SR_SPEEDSTEPDOWN;
            case PK_UP:
                return SR_SPEEDSTEPUPFINE;
            case PK_DOWN:
                return SR_SPEEDSTEPDOWNFINE;
            case PK_HOME:
                return SR_SPEEDSTEPNORMAL;
            case PK_END:
                return SR_FULLTHROTTLE;
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
