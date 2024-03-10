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

#include "Globals.h"
#include "KbdLayout.h"
#include "Keyboard.h"
#include "Platform.h"
#include "Emulation.h"


using namespace std;


void KbdLayout::resetKeys()
{
    m_platform->getKeyboard()->resetKeys();
    m_shiftPressed = false;
    m_lastNonUnicodeKey = EK_NONE;
    m_lastPalKeyPressedCode = PK_NONE;
    m_shiftSet.clear();
    m_langSet.clear();
}


void KbdLayout::processKey(PalKeyCode keyCode, bool isPressed, unsigned unicodeKey)
{
    if (isPressed && processSpecialKeys(keyCode))
        return;

    Keyboard* kbd = m_platform->getKeyboard();

    EmuKey emuKey;

    switch (m_mode) {
        case KLM_QWERTY:
        case KLM_JCUKEN:
            emuKey = translateKey(keyCode);
            kbd->processKey(emuKey, isPressed);
            break;
        case KLM_SMART:
            bool shift;
            bool lang;
            emuKey = translateUnicodeKey(unicodeKey, keyCode != PK_NONE ? keyCode : m_lastPalKeyPressedCode, shift, lang);
            if (emuKey == EK_NONE) {
                emuKey = translateKey(keyCode);
                if (emuKey == EK_SHIFT)
                    m_shiftPressed = isPressed;
                else if (emuKey == EK_LANG)
                    m_langPressed = isPressed;
                kbd->processKey(emuKey, isPressed);
                if (isPressed && emuKey != EK_SHIFT && emuKey != EK_LANG) {// SDL issue, see below
                    m_lastNonUnicodeKey = emuKey;
                    m_lastPalKeyPressedCode = keyCode;
                }
            } else {
                // Workaround for SDL: unicode and ordinary codes go separately
                if (keyCode == PK_NONE && m_lastNonUnicodeKey != EK_NONE)
                    kbd->processKey(m_lastNonUnicodeKey, false);
                m_lastNonUnicodeKey = EK_NONE;
                m_lastPalKeyPressedCode = PK_NONE;

                int s1 = m_shiftSet.size();
                if (shift && isPressed)
                    m_shiftSet.insert(emuKey);
                else if (shift && !isPressed)
                    m_shiftSet.erase(emuKey);
                int s2 = m_shiftSet.size();

                if (m_shiftPressed && !shift)
                    kbd->processKey(EK_SHIFT, !isPressed);
                else if (s1 == 0 && s2 > 0)
                    kbd->processKey(EK_SHIFT, true);
                else if (s1 > 0 && s2 == 0)
                    kbd->processKey(EK_SHIFT, false);

                s1 = m_langSet.size();
                if (lang && isPressed)
                    m_langSet.insert(emuKey);
                else if (lang && !isPressed)
                    m_langSet.erase(emuKey);
                lang = !m_langSet.empty();
                s2 = m_langSet.size();

                if (m_separateRusLat) {
                    // Lvov etc.
                    kbd->processKey(EK_RUS, lang && isPressed);
                    if (m_prevLang && !lang)
                        kbd->processKey(EK_LAT, lang != isPressed);
                    if (!isPressed)
                        m_prevLang = lang;
                } else {
                    // Pk8000 etc.
                    if (s1 == 0 && s2 > 0)
                        kbd->processKey(EK_LANG, true);
                    else if (s1 > 0 && s2 == 0)
                        kbd->processKey(EK_LANG, false);
                }
                if (!m_helper || !isPressed)
                    kbd->processKey(emuKey, isPressed);
                else // delayed key press
                    m_helper->enqueueKeyPress(emuKey);
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
    } else if (propertyName == "helper") {
        m_helper = static_cast<KbdLayoutHelper*>(g_emulation->findObject(values[0].asString()));
        return true;
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


EmuKey KbdLayout::translateCommonKeys(PalKeyCode keyCode)
{
    EmuKey key = m_mode == KLM_JCUKEN ? translateCommonKeysJcuken(keyCode) : translateCommonKeysQwerty(keyCode);
    if (key != EK_NONE)
        return key;

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

    case PK_SLASH:
        return EK_SLASH;

    case PK_SEMICOLON:
        return EK_SEMICOLON;
    case PK_EQU:
        return EK_COLON;
    case PK_MINUS:
        return EK_MINUS;
    case PK_TAB:
        return EK_TAB;

    case PK_PERIOD:
        return EK_PERIOD;
    case PK_SPACE:
        return EK_SPACE;
    case PK_BSP:
        return EK_BSP;

    case PK_ENTER:
    case PK_KP_ENTER:
        return EK_CR;
    case PK_ESC:
        return EK_ESC;
    case PK_UP:
        return EK_UP;
    case PK_KP_8:
        return m_numpadJoystick ? EK_JS_UP : EK_UP;
    case PK_DOWN:
        return EK_DOWN;
    case PK_KP_2:
        return m_numpadJoystick ? EK_JS_DOWN : EK_DOWN;
    case PK_LEFT:
        return EK_LEFT;
    case PK_KP_4:
        return m_numpadJoystick ? EK_JS_LEFT : EK_LEFT;
    case PK_RIGHT:
        return EK_RIGHT;
    case PK_KP_6:
        return m_numpadJoystick ? EK_JS_RIGHT : EK_RIGHT;
    case PK_KP_5:
        return m_numpadJoystick ? EK_JS_BTN2 : EK_NONE;
    case PK_COMMA:
        return EK_COMMA;

    case PK_LSHIFT:
    case PK_RSHIFT:
        return EK_SHIFT;

    case PK_KP_0:
        return m_numpadJoystick ? EK_JS_BTN2 : EK_LANG;

    case PK_RCTRL:
    case PK_INS:
        return EK_LANG;

    default:
        return EK_NONE;
    }
}

EmuKey KbdLayout::translateCommonKeysQwerty(PalKeyCode keyCode)
{
    switch (keyCode) {
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

    case PK_LBRACKET:
        return EK_LBRACKET;
    case PK_RBRACKET:
        return EK_RBRACKET;
    case PK_BSLASH:
        return EK_BKSLASH;
    case PK_APOSTROPHE:
        return EK_CARET;
    case PK_TILDE:
        return EK_AT;

    default:
        return EK_NONE;
    }
}


EmuKey KbdLayout::translateCommonKeysJcuken(PalKeyCode keyCode)
{
    switch (keyCode) {
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

    default:
        return EK_NONE;
    }
}


EmuKey KbdLayout::translateCommonUnicodeKeys(unsigned unicodeKey, bool& shift, bool& lang)
{
    EmuKey key;
    shift = false;
    lang = false;

    if (unicodeKey >= L'A' && unicodeKey <= L'Z') {
        key = (EmuKey)((int)EK_A + (unicodeKey - L'A'));
        shift = true;
    } else if (unicodeKey >= L'a' && unicodeKey <= L'z')
        key = (EmuKey)((int)EK_A + (unicodeKey - L'a'));
    else switch (unicodeKey) {
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
            shift = true;
            break;
        case L',':
            key = EK_COMMA;
            break;
        case L'/':
            key = EK_SLASH;
            break;
        case L' ':
            key = EK_SPACE;
            break;
        //case L'_':
        //    key = EK_BSP;
        //    break;

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
        case L'|':
            key = EK_BKSLASH;
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

        // Cyrillic letters (upper case)
        case L'А':
            key = EK_A;
            lang = true;
            shift = true;
            break;
        case L'Б':
            key = EK_B;
            lang = true;
            shift = true;
            break;
        case L'В':
            key = EK_W;
            lang = true;
            shift = true;
            break;
        case L'Г':
            key = EK_G;
            lang = true;
            shift = true;
            break;
        case L'Д':
            key = EK_D;
            lang = true;
            shift = true;
            break;
        case L'Е':
            key = EK_E;
            lang = true;
            shift = true;
            break;
        case L'Ж':
            key = EK_V;
            lang = true;
            shift = true;
            break;
        case L'З':
            key = EK_Z;
            lang = true;
            shift = true;
            break;
        case L'И':
            key = EK_I;
            lang = true;
            shift = true;
            break;
        case L'Й':
            key = EK_J;
            lang = true;
            shift = true;
            break;
        case L'К':
            key = EK_K;
            lang = true;
            shift = true;
            break;
        case L'Л':
            key = EK_L;
            lang = true;
            shift = true;
            break;
        case L'М':
            key = EK_M;
            lang = true;
            shift = true;
            break;
        case L'Н':
            key = EK_N;
            lang = true;
            shift = true;
            break;
        case L'О':
            key = EK_O;
            lang = true;
            shift = true;
            break;
        case L'П':
            key = EK_P;
            lang = true;
            shift = true;
            break;
        case L'Р':
            key = EK_R;
            lang = true;
            shift = true;
            break;
        case L'С':
            key = EK_S;
            lang = true;
            shift = true;
            break;
        case L'Т':
            key = EK_T;
            lang = true;
            shift = true;
            break;
        case L'У':
            key = EK_U;
            lang = true;
            shift = true;
            break;
        case L'Ф':
            key = EK_F;
            lang = true;
            shift = true;
            break;
        case L'Х':
            key = EK_H;
            lang = true;
            shift = true;
            break;
        case L'Ц':
            key = EK_C;
            lang = true;
            shift = true;
            break;
        case L'Ч':
            key = EK_CARET;
            lang = true;
            shift = true;
            break;
        case L'Ш':
            key = EK_LBRACKET;
            lang = true;
            shift = true;
            break;
        case L'Щ':
            key = EK_RBRACKET;
            lang = true;
            shift = true;
            break;
        // Ъъ For Mikrosha only
        case L'Ъ':
            key = EK_BSP;
            lang = true;
            shift = true;
            break;
        case L'Ы':
            key = EK_Y;
            lang = true;
            shift = true;
            break;
        case L'Ь':
            key = EK_X;
            lang = true;
            shift = true;
            break;
        case L'Э':
            key = EK_BKSLASH;
            lang = true;
            shift = true;
            break;
        case L'Ю':
            key = EK_AT;
            lang = true;
            shift = true;
            break;
        case L'Я':
            key = EK_Q;
            lang = true;
            shift = true;
            break;

       // Cyrillic letters (upper case)
       case L'а':
           key = EK_A;
           lang = true;
           break;
       case L'б':
           key = EK_B;
           lang = true;
           break;
       case L'в':
           key = EK_W;
           lang = true;
           break;
       case L'г':
           key = EK_G;
           lang = true;
           break;
       case L'д':
           key = EK_D;
           lang = true;
           break;
       case L'е':
           key = EK_E;
           lang = true;
           break;
       case L'ж':
           key = EK_V;
           lang = true;
           break;
       case L'з':
           key = EK_Z;
           lang = true;
           break;
       case L'и':
           key = EK_I;
           lang = true;
           break;
       case L'й':
           key = EK_J;
           lang = true;
           break;
       case L'к':
           key = EK_K;
           lang = true;
           break;
       case L'л':
           key = EK_L;
           lang = true;
           break;
       case L'м':
           key = EK_M;
           lang = true;
           break;
       case L'н':
           key = EK_N;
           lang = true;
           break;
       case L'о':
           key = EK_O;
           lang = true;
           break;
       case L'п':
           key = EK_P;
           lang = true;
           break;
       case L'р':
           key = EK_R;
           lang = true;
           break;
       case L'с':
           key = EK_S;
           lang = true;
           break;
       case L'т':
           key = EK_T;
           lang = true;
           break;
       case L'у':
           key = EK_U;
           lang = true;
           break;
       case L'ф':
           key = EK_F;
           lang = true;
           break;
       case L'х':
           key = EK_H;
           lang = true;
           break;
       case L'ц':
           key = EK_C;
           lang = true;
           break;
       case L'ч':
           key = EK_CARET;
           lang = true;
           break;
       case L'ш':
           key = EK_LBRACKET;
           lang = true;
           break;
       case L'щ':
           key = EK_RBRACKET;
           lang = true;
           break;
       // Ъъ For Mikrosha only
       case L'ъ':
           key = EK_BSP;
           lang = true;
           break;
       case L'ы':
           key = EK_Y;
           lang = true;
           break;
       case L'ь':
           key = EK_X;
           lang = true;
           break;
       case L'э':
           key = EK_BKSLASH;
           lang = true;
           break;
       case L'ю':
           key = EK_AT;
           lang = true;
           break;
       case L'я':
           key = EK_Q;
           lang = true;
           break;

        default:
            key = EK_NONE;
    }
    return key;
}


EmuKey RkKbdLayout::translateKey(PalKeyCode keyCode)
{
    EmuKey key = translateCommonKeys(keyCode);
    if (key != EK_NONE)
        return key;

    switch (keyCode) {
        case PK_HOME:
            return EK_HOME;

        case PK_PGUP:
            return EK_CLEAR;
        case PK_PGDN:
            return EK_LF;

        case PK_LCTRL:
            return EK_CTRL;

        default:
            return EK_NONE;
    }
}


EmuKey RkKbdLayout::translateUnicodeKey(unsigned unicodeKey, PalKeyCode, bool& shift, bool& lang)
{
    if (unicodeKey == L'_') {
        lang = false;
        shift = true;
        return EK_BSP;
    }

    EmuKey key = translateCommonUnicodeKeys(unicodeKey, shift, lang);
    if (key != EK_NONE) {
        if (key >= EK_A && key <= EK_Z)
            shift = lang;
        if (lang &&
            (key == EK_LBRACKET || key == EK_RBRACKET || key == EK_BKSLASH || key == EK_CARET || key == EK_AT))
            shift = lang;
        else if (key == EK_AT)
            shift = !shift;
    }
    lang = false;
    return key;
}


EmuKey KrKbdLayout::translateKey(PalKeyCode keyCode)
{
    switch (keyCode) {
    case PK_F8:
    case PK_MENU:
        return EK_VR;
    case PK_F9:
        return EK_UNDSCR;
    case PK_KP_0:
        return EK_NP_0;
    case PK_KP_1:
        return EK_NP_1;
    case PK_KP_2:
        return EK_NP_2;
    case PK_KP_3:
        return EK_NP_3;
    case PK_KP_4:
        return EK_NP_4;
    case PK_KP_5:
        return EK_NP_5;
    case PK_KP_6:
        return EK_NP_6;
    case PK_KP_7:
        return EK_NP_7;
    case PK_KP_8:
        return EK_NP_8;
    case PK_KP_9:
        return EK_NP_9;
    case PK_KP_PERIOD:
        return EK_NP_PERIOD;
    case PK_KP_ENTER:
        return EK_NP_CR;
    case PK_KP_DIV:
        return EK_NP_COMMA;
    case PK_KP_PLUS:
        return EK_NP_PLUSMUL;
    case PK_KP_MINUS:
        return EK_NP_MINUSDIV;
    default:
        break;
    }

    EmuKey key = RkKbdLayout::translateKey(keyCode);
    if (key != EK_NONE)
        return key;

    switch (keyCode) {
    case PK_DEL:
        return EK_RUS;
    default:
        return EK_NONE;
    }
 return key;
}


EmuKey KrKbdLayout::translateUnicodeKey(unsigned unicodeKey, PalKeyCode keyCode, bool& shift, bool& lang)
{
    /*if (keyCode == PK_KP_PLUS || keyCode == PK_KP_MINUS || keyCode == PK_KP_DIV)
        return EK_NONE;*/

    if (unicodeKey == L'_')
        return EK_UNDSCR;

    return RkKbdLayout::translateUnicodeKey(unicodeKey, keyCode, shift, lang);
}


KbdLayoutHelper::KbdLayoutHelper()
{
    pause();
    ActiveDevice::setFrequency(1000000); // 1 µs
}


void KbdLayoutHelper::enqueueKeyPress(EmuKey key)
{
    m_key = key;
    resume();
    syncronize();
    m_curClock += m_kDiv * m_delay;
}


void KbdLayoutHelper::operate()
{
    Keyboard* kbd = m_platform->getKeyboard();
    kbd->processKey(m_key, true);
    pause();
}


bool KbdLayoutHelper::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "delay") {
        if (values[0].isInt()) {
            m_delay = values[0].asInt();
            return true;
        }
    }
    return false;
}
