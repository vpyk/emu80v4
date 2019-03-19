/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2017
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

#ifndef PALKEYS_H
#define PALKEYS_H

enum PalKeyCode
{
    PK_NONE,

    PK_A,
    PK_B,
    PK_C,
    PK_D,
    PK_E,
    PK_F,
    PK_G,
    PK_H,
    PK_I,
    PK_J,
    PK_K,
    PK_L,
    PK_M,
    PK_N,
    PK_O,
    PK_P,
    PK_Q,
    PK_R,
    PK_S,
    PK_T,
    PK_U,
    PK_V,
    PK_W,
    PK_X,
    PK_Y,
    PK_Z,

    PK_1,
    PK_2,
    PK_3,
    PK_4,
    PK_5,
    PK_6,
    PK_7,
    PK_8,
    PK_9,
    PK_0,

    PK_ENTER,
    PK_ESC,
    PK_BSP,
    PK_TAB,
    PK_SPACE,

    PK_MINUS,
    PK_EQU,
    PK_LBRACKET,
    PK_RBRACKET,
    PK_BSLASH,
    PK_SEMICOLON,
    PK_APOSTROPHE,
    PK_TILDE,
    PK_COMMA,
    PK_PERIOD,
    PK_SLASH,

    PK_CAPSLOCK,

    PK_F1,
    PK_F2,
    PK_F3,
    PK_F4,
    PK_F5,
    PK_F6,
    PK_F7,
    PK_F8,
    PK_F9,
    PK_F10,
    PK_F11,
    PK_F12,

    PK_PRSCR,
    PK_SCRLOCK,
    PK_PAUSEBRK,

    PK_INS,
    PK_HOME,
    PK_PGUP,
    PK_DEL,
    PK_END,
    PK_PGDN,
    PK_RIGHT,
    PK_LEFT,
    PK_DOWN,
    PK_UP,

    PK_NUMLOCK,
    PK_KP_DIV,
    PK_KP_MUL,
    PK_KP_MINUS,
    PK_KP_PLUS,
    PK_KP_ENTER,
    PK_KP_1,
    PK_KP_2,
    PK_KP_3,
    PK_KP_4,
    PK_KP_5,
    PK_KP_6,
    PK_KP_7,
    PK_KP_8,
    PK_KP_9,
    PK_KP_0,
    PK_KP_PERIOD,

	PK_LSHIFT,
	PK_RSHIFT,
	PK_LCTRL,
	PK_RCTRL,
	PK_LALT,
	PK_RALT,
	PK_LWIN,
	PK_RWIN,
	PK_MENU,

    PK_JOY_UP,
    PK_JOY_DN,
    PK_JOY_LEFT,
    PK_JOY_RIGHT
};


enum PalMouseKey
{
    PM_LEFT_CLICK,
    PM_LEFT_DBLCLICK,
    PM_WHEEL_UP,
    PM_WHEEL_DOWN
};


#endif // PALKEYS_H
