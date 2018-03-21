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

#include "KbdLayout.h"
#include "Keyboard.h"
#include "Platform.h"


using namespace std;


void KbdLayout::resetKeys()
{
    m_platform->getKeyboard()->resetKeys();
    m_shiftPressed = false;
    m_lastNonUnicodeKey = EK_NONE;
}


void KbdLayout::processKey(PalKeyCode keyCode, bool isPressed, unsigned unicodeKey)
{
    Keyboard* kbd = m_platform->getKeyboard();

    EmuKey emuKey;

    switch (m_mode) {
        case KLM_QWERTY:
            emuKey = translateKeyQwerty(keyCode);
            kbd->processKey(emuKey, isPressed);
            break;
        case KLM_JCUKEN:
            emuKey = translateKeyJcuken(keyCode);
            kbd->processKey(emuKey, isPressed);
            break;
        case KLM_SMART:
            bool shift;
            emuKey = translateKeySmart(unicodeKey, shift);
            if (emuKey == EK_NONE) {
                emuKey = translateKeyQwerty(keyCode);
                if (emuKey == EK_SHIFT)
                    m_shiftPressed = isPressed;
                kbd->processKey(emuKey, isPressed);
                if (emuKey != EK_SHIFT) // SDL issue, see below
                    m_lastNonUnicodeKey = emuKey;
            } else {
                // Workaround for SDL: unocode and ordinary codes go separately
                if (keyCode == PK_NONE && m_lastNonUnicodeKey != EK_NONE)
                    kbd->processKey(m_lastNonUnicodeKey, false);
                m_lastNonUnicodeKey = EK_NONE;

                if (shift != m_shiftPressed)
                    kbd->processKey(EK_SHIFT, shift == isPressed);
                kbd->processKey(emuKey, isPressed);
            }

            break;
        default:
            emuKey = EK_NONE; // normally this never occurs
    }

}


bool KbdLayout::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    string s = values[0].asString();

    if (propertyName == "layout") {
        if (values[0].asString() == "qwerty") {
            setQwertyMode();
            return true;
        } else if (values[0].asString() == "jcuken") {
            setJcukenMode();
            return true;
        } else if (values[0].asString() == "smart") {
            setSmartMode();
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
        if (m_mode == KLM_QWERTY)
            return "qwerty";
        else if (m_mode == KLM_JCUKEN)
            return "jcuken";
        else // if (m_mode == KLM_SMART)
            return "smart";
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
        case PK_KP_ENTER:
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
        case PK_KP_ENTER:
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


EmuKey RkKbdLayout::translateKeySmart(unsigned unicodeKey, bool& shift)
{
    EmuKey key;
    shift = false;

    switch (unicodeKey) {
        // Latin letters (lower case)
        case L'A':
        case L'a':
            key = EK_A;
            break;
        case L'B':
        case L'b':
            key = EK_B;
            break;
        case L'C':
        case L'c':
            key = EK_C;
            break;
        case L'D':
        case L'd':
            key = EK_D;
            break;
        case L'E':
        case L'e':
            key = EK_E;
            break;
        case L'F':
        case L'f':
            key = EK_F;
            break;
        case L'G':
        case L'g':
            key = EK_G;
            break;
        case L'H':
        case L'h':
            key = EK_H;
            break;
        case L'I':
        case L'i':
            key = EK_I;
            break;
        case L'J':
        case L'j':
            key = EK_J;
            break;
        case L'K':
        case L'k':
            key = EK_K;
            break;
        case L'L':
        case L'l':
            key = EK_L;
            break;
        case L'M':
        case L'm':
            key = EK_M;
            break;
        case L'N':
        case L'n':
            key = EK_N;
            break;
        case L'O':
        case L'o':
            key = EK_O;
            break;
        case L'P':
        case L'p':
            key = EK_P;
            break;
        case L'Q':
        case L'q':
            key = EK_Q;
            break;
        case L'R':
        case L'r':
            key = EK_R;
            break;
        case L'S':
        case L's':
            key = EK_S;
            break;
        case L'T':
        case L't':
            key = EK_T;
            break;
        case L'U':
        case L'u':
            key = EK_U;
            break;
        case L'V':
        case L'v':
            key = EK_V;
            break;
        case L'W':
        case L'w':
            key = EK_W;
            break;
        case L'X':
        case L'x':
            key = EK_X;
            break;
        case L'Y':
        case L'y':
            key = EK_Y;
            break;
        case L'Z':
        case L'z':
            key = EK_Z;
            break;

        // Lower case symbols
        case L';':
            key = EK_SEMICOLON;
            break;
        case L'-':
            key = EK_MINUS;
            break;
        case L':':
            key = EK_COLON;
            break;
        case L'[':
            key = EK_LBRACKET;
            break;
        case L']':
            key = EK_RBRACKET;
            break;
        case L'\\':
            key = EK_BKSLASH;
            break;
        case L'.':
            key = EK_PERIOD;
            break;
        case L'^':
            key = EK_CARET;
            break;
        case L'@':
            key = EK_AT;
            break;
        case L',':
            key = EK_COMMA;
            break;
        case L'/':
            key = EK_SLASH;
            break;
        case L'_':
            key = EK_BSP;
            break;


        // Cyrillic letters (upper case)
        case L'А':
        case L'а':
            key = EK_A;
            shift = true;
            break;
        case L'Б':
        case L'б':
            key = EK_B;
            shift = true;
            break;
        case L'В':
        case L'в':
            key = EK_W;
            shift = true;
            break;
        case L'Г':
        case L'г':
            key = EK_G;
            shift = true;
            break;
        case L'Д':
        case L'д':
            key = EK_D;
            shift = true;
            break;
        case L'Е':
        case L'е':
            key = EK_E;
            shift = true;
            break;
        case L'Ж':
        case L'ж':
            key = EK_V;
            shift = true;
            break;
        case L'З':
        case L'з':
            key = EK_Z;
            shift = true;
            break;
        case L'И':
        case L'и':
            key = EK_I;
            shift = true;
            break;
        case L'Й':
        case L'й':
            key = EK_J;
            shift = true;
            break;
        case L'К':
        case L'к':
            key = EK_K;
            shift = true;
            break;
        case L'Л':
        case L'л':
            key = EK_L;
            shift = true;
            break;
        case L'М':
        case L'м':
            key = EK_M;
            shift = true;
            break;
        case L'Н':
        case L'н':
            key = EK_N;
            shift = true;
            break;
        case L'О':
        case L'о':
            key = EK_O;
            shift = true;
            break;
        case L'П':
        case L'п':
            key = EK_P;
            shift = true;
            break;
        case L'Р':
        case L'р':
            key = EK_R;
            shift = true;
            break;
        case L'С':
        case L'с':
            key = EK_S;
            shift = true;
            break;
        case L'Т':
        case L'т':
            key = EK_T;
            shift = true;
            break;
        case L'У':
        case L'у':
            key = EK_U;
            shift = true;
            break;
        case L'Ф':
        case L'ф':
            key = EK_F;
            shift = true;
            break;
        case L'Х':
        case L'х':
            key = EK_H;
            shift = true;
            break;
        case L'Ц':
        case L'ц':
            key = EK_C;
            shift = true;
            break;
        case L'Ч':
        case L'ч':
            key = EK_CARET;
            shift = true;
            break;
        case L'Ш':
        case L'ш':
            key = EK_LBRACKET;
            shift = true;
            break;
        case L'Щ':
        case L'щ':
            key = EK_RBRACKET;
            shift = true;
            break;
        // Ъъ For Mikrosha only
        case L'Ъ':
        case L'ъ':
            key = EK_BSP;
            shift = true;
            break;
        case L'Ы':
        case L'ы':
            key = EK_Y;
            shift = true;
            break;
        case L'Ь':
        case L'ь':
            key = EK_X;
            shift = true;
            break;
        case L'Э':
        case L'э':
            key = EK_BKSLASH;
            shift = true;
            break;
        case L'Ю':
        case L'ю':
            key = EK_AT;
            shift = true;
            break;
        case L'Я':
        case L'я':
            key = EK_Q;
            shift = true;
            break;

        // Digits (lower case)
        case L'0':
            key = EK_0;
            break;
        case L'1':
            key = EK_1;
            break;
        case L'2':
            key = EK_2;
            break;
        case L'3':
            key = EK_3;
            break;
        case L'4':
            key = EK_4;
            break;
        case L'5':
            key = EK_5;
            break;
        case L'6':
            key = EK_6;
            break;
        case L'7':
            key = EK_7;
            break;
        case L'8':
            key = EK_8;
            break;
        case L'9':
            key = EK_9;
            break;

        //Upper-case symbols
        case L'+':
            key = EK_SEMICOLON;
            shift = true;
            break;
        case L'!':
            key = EK_1;
            shift = true;
            break;
        case L'\"':
            key = EK_2;
            shift = true;
            break;
        case L'#':
            key = EK_3;
            shift = true;
            break;
        case L'$':
            key = EK_4;
            shift = true;
            break;
        case L'%':
            key = EK_5;
            shift = true;
            break;
        case L'&':
            key = EK_6;
            shift = true;
            break;
        case L'\'':
            key = EK_7;
            shift = true;
            break;
        case L'(':
            key = EK_8;
            shift = true;
            break;
        case L')':
            key = EK_9;
            shift = true;
            break;
        case L'=':
            key = EK_MINUS;
            shift = true;
            break;
        case L'*':
            key = EK_COLON;
            shift = true;
            break;
        case L'>':
            key = EK_PERIOD;
            shift = true;
            break;
        case L'<':
            key = EK_COMMA;
            shift = true;
            break;
        case L'?':
            key = EK_SLASH;
            shift = true;
            break;
        default:
            key = EK_NONE;
    }
    return key;
}
