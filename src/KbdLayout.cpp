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

#include "KbdLayout.h"

using namespace std;

EmuKey KbdLayout::translateKey(PalKeyCode keyCode)
{
    return m_isJcuken ? translateKeyJcuken(keyCode) : translateKeyQwerty(keyCode);
}


bool KbdLayout::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "layout") {
        if (values[0].asString() == "qwerty") {
            setQwertyMode();
            return true;
        } else if (values[0].asString() == "jcuken") {
            setJcukenMode();
            return true;
        } else
            return false;
    }
    return false;
}


string KbdLayout::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "layout") {
        if (m_isJcuken)
            return "jcuken";
        else
            return "qwerty";
    }

    return "";
}


EmuKey RkKbdLayout::translateKeyQwerty(PalKeyCode keyCode)
{
    switch (keyCode) {
        case PK_0:
            return EK_0;
        case PK_1:
            return EK_1;
        case PK_2:
            return EK_2;
        case PK_3:
            return EK_3;
        case PK_4:
            return EK_4;
        case PK_5:
            return EK_5;
        case PK_6:
            return EK_6;
        case PK_7:
            return EK_7;
        case PK_8:
            return EK_8;
        case PK_9:
            return EK_9;

        case PK_A:
            return EK_A;
        case PK_B:
            return EK_B;
        case PK_C:
            return EK_C;
        case PK_D:
            return EK_D;
        case PK_E:
            return EK_E;
        case PK_F:
            return EK_F;
        case PK_G:
            return EK_G;
        case PK_H:
            return EK_H;
        case PK_I:
            return EK_I;
        case PK_J:
            return EK_J;
        case PK_K:
            return EK_K;
        case PK_L:
            return EK_L;
        case PK_M:
            return EK_M;
        case PK_N:
            return EK_N;
        case PK_O:
            return EK_O;
        case PK_P:
            return EK_P;
        case PK_Q:
            return EK_Q;
        case PK_R:
            return EK_R;
        case PK_S:
            return EK_S;
        case PK_T:
            return EK_T;
        case PK_U:
            return EK_U;
        case PK_V:
            return EK_V;
        case PK_W:
            return EK_W;
        case PK_X:
            return EK_X;
        case PK_Y:
            return EK_Y;
        case PK_Z:
            return EK_Z;

        case PK_F1:
            return EK_F1;
        case PK_F2:
            return EK_F2;
        case PK_F3:
            return EK_F3;
        case PK_F4:
            return EK_F4;
        case PK_F5:
            return EK_F5;

        case PK_LBRACKET:
            return EK_LBRACKET;
        case PK_RBRACKET:
            return EK_RBRACKET;
        case PK_SLASH:
            return EK_SLASH;
        case PK_BSLASH:
            return EK_BKSLASH;
        case PK_HOME:
            return EK_HOME;

        case PK_SEMICOLON:
            return EK_SEMICOLON;
        case PK_EQU:
            return EK_COLON;
        case PK_MINUS:
            return EK_MINUS;
        case PK_TAB:
            return EK_TAB;

        case PK_ENTER:
            return EK_CR;
        case PK_UP:
        case PK_KP_8:
            return EK_UP;
        case PK_DOWN:
        case PK_KP_2:
            return EK_DOWN;
        case PK_LEFT:
        case PK_KP_4:
            return EK_LEFT;
        case PK_RIGHT:
        case PK_KP_6:
            return EK_RIGHT;
        case PK_COMMA:
            return EK_COMMA;
        case PK_APOSTROPHE:
            return EK_CARET;
        case PK_TILDE:
            return EK_AT;
        case PK_PERIOD:
            return EK_PERIOD;
        case PK_SPACE:
            return EK_SPACE;
        case PK_BSP:
            return EK_BSP;

        case PK_ESC:
            return EK_ESC;
        case PK_PGUP:
            return EK_CLEAR;
        case PK_PGDN:
            return EK_LF;

        case PK_LSHIFT:
        case PK_RSHIFT:
            return EK_SHIFT;
        case PK_LCTRL:
            return EK_CTRL;
        case PK_RCTRL:
            return EK_LANG;

        // Клавиши Специалиста
        case PK_F6:
            return EK_F6;
        case PK_F7:
            return EK_F7;
        case PK_F8:
            return EK_F8;
        case PK_F9:
            return EK_F9;
        case PK_F10:
            return EK_F10;
        case PK_F11:
            return EK_F11;


        default:
            return EK_NONE;
    }
}


EmuKey RkKbdLayout::translateKeyJcuken(PalKeyCode keyCode)
{
    switch (keyCode) {
        case PK_0:
            return EK_0;
        case PK_1:
            return EK_1;
        case PK_2:
            return EK_2;
        case PK_3:
            return EK_3;
        case PK_4:
            return EK_4;
        case PK_5:
            return EK_5;
        case PK_6:
            return EK_6;
        case PK_7:
            return EK_7;
        case PK_8:
            return EK_8;
        case PK_9:
            return EK_9;

        case PK_Q:
            return EK_J;
        case PK_W:
            return EK_C;
        case PK_E:
            return EK_U;
        case PK_R:
            return EK_K;
        case PK_T:
            return EK_E;
        case PK_Y:
            return EK_N;
        case PK_U:
            return EK_G;
        case PK_I:
            return EK_LBRACKET;
        case PK_O:
            return EK_RBRACKET;
        case PK_P:
            return EK_Z;
        case PK_LBRACKET:
            return EK_H;

        case PK_A:
            return EK_F;
        case PK_S:
            return EK_Y;
        case PK_D:
            return EK_W;
        case PK_F:
            return EK_A;
        case PK_G:
            return EK_P;
        case PK_H:
            return EK_R;
        case PK_J:
            return EK_O;
        case PK_K:
            return EK_L;
        case PK_L:
            return EK_D;
        case PK_SEMICOLON:
            return EK_V;
        case PK_APOSTROPHE:
            return EK_BKSLASH;

        case PK_Z:
            return EK_Q;
        case PK_X:
            return EK_CARET;
        case PK_C:
            return EK_S;
        case PK_V:
            return EK_M;
        case PK_B:
            return EK_I;
        case PK_N:
            return EK_T;
        case PK_M:
            return EK_X;
        case PK_COMMA:
          return EK_B;
        case PK_PERIOD:
            return EK_AT;

        case PK_F1:
            return EK_F1;
        case PK_F2:
            return EK_F2;
        case PK_F3:
            return EK_F3;
        case PK_F4:
            return EK_F4;
        case PK_F5:
            return EK_F5;

        case PK_RBRACKET:
            return EK_COLON;
        case PK_SLASH:
            return EK_COMMA;
        case PK_BSLASH:
            return EK_PERIOD;
        case PK_HOME:
            return EK_HOME;

        case PK_EQU:
            return EK_COLON;
        case PK_MINUS:
            return EK_MINUS;
        case PK_TAB:
            return EK_TAB;

        case PK_ENTER:
            return EK_CR;
        case PK_UP:
            return EK_UP;
        case PK_DOWN:
            return EK_DOWN;
        case PK_LEFT:
            return EK_LEFT;
        case PK_RIGHT:
            return EK_RIGHT;
        case PK_TILDE:
            return EK_AT;
        case PK_SPACE:
            return EK_SPACE;
        case PK_BSP:
            return EK_BSP;

        case PK_ESC:
            return EK_ESC;
        case PK_PGUP:
            return EK_CLEAR;
        case PK_PGDN:
            return EK_LF;

        case PK_LSHIFT:
        case PK_RSHIFT:
            return EK_SHIFT;
        case PK_LCTRL:
            return EK_CTRL;
        case PK_RCTRL:
            return EK_LANG;

        // Клавиши Специалиста
        case PK_F6:
            return EK_F6;
        case PK_F7:
            return EK_F7;
        case PK_F8:
            return EK_F8;
        case PK_F9:
            return EK_F9;
        case PK_F10:
            return EK_F10;
        case PK_F11:
            return EK_F11;

        default:
            return EK_NONE;
    }
}
