/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2021
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

#ifndef RKKEYBOARD_H
#define RKKEYBOARD_H

#include "Keyboard.h"


class RkKeyboard : public Keyboard
{
    public:
        RkKeyboard();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void resetKeys() override;
        void processKey(EmuKey key, bool isPressed) override;

        void setMatrixMask(uint8_t mask);
        uint8_t getMatrixData();
        uint8_t getCtrlKeys();

        static EmuObject* create(const EmuValuesList&) {return new RkKeyboard();}

    private:

    enum RkKbdMatrix {
        RKM_RK = 0,
        RKM_PARTNER,
        RKM_MIKROSHA,
        RKM_MIKRO80
    };

        const EmuKey m_keyMatrix[8][8] = {
            { EK_HOME, EK_CLEAR, EK_ESC,   EK_F1,        EK_F2,      EK_F3,       EK_F4,     EK_F5    },
            { EK_TAB,  EK_LF,    EK_CR,    EK_BSP,       EK_LEFT,    EK_UP,       EK_RIGHT,  EK_DOWN  },
            { EK_0,    EK_1,     EK_2,     EK_3,         EK_4,       EK_5,        EK_6,      EK_7     },
            { EK_8,    EK_9,     EK_COLON, EK_SEMICOLON, EK_COMMA,   EK_MINUS,    EK_PERIOD, EK_SLASH },
            { EK_AT,   EK_A,     EK_B,     EK_C,         EK_D,       EK_E,        EK_F,      EK_G     },
            { EK_H,    EK_I,     EK_J,     EK_K,         EK_L,       EK_M,        EK_N,      EK_O     },
            { EK_P,    EK_Q,     EK_R,     EK_S,         EK_T,       EK_U,        EK_V,      EK_W     },
            { EK_X,    EK_Y,     EK_Z,     EK_LBRACKET,  EK_BKSLASH, EK_RBRACKET, EK_CARET,  EK_SPACE }
        };

        const EmuKey m_KeyMatrixMikrosha[8][8] = {
            { EK_SPACE, EK_ESC,  EK_TAB,     EK_LF,        EK_CR,      EK_CLEAR,    EK_LEFT,   EK_RIGHT },
            { EK_UP,    EK_DOWN, EK_HOME,    EK_F1,        EK_F2,      EK_F3,       EK_F4,     EK_F5    },
            { EK_0,     EK_1,    EK_2,       EK_3,         EK_4,       EK_5,        EK_6,      EK_7     },
            { EK_8,     EK_9,    EK_COLON,   EK_SEMICOLON, EK_COMMA,   EK_MINUS,    EK_PERIOD, EK_SLASH },
            { EK_AT,    EK_A,    EK_B,       EK_C,         EK_D,       EK_E,        EK_F,      EK_G     },
            { EK_H,     EK_I,    EK_J,       EK_K,         EK_L,       EK_M,        EK_N,      EK_O     },
            { EK_P,     EK_Q,    EK_R,       EK_S,         EK_T,       EK_U,        EK_V,      EK_W     },
            { EK_X,     EK_Y,    EK_Z,       EK_LBRACKET,  EK_BKSLASH, EK_RBRACKET, EK_CARET,  EK_BSP   }
        };

        const EmuKey m_keyMatrixMikro80[8][8] = {
            { EK_0,      EK_1,         EK_2,       EK_3,        EK_4,         EK_5,     EK_6,     EK_NONE },
            { EK_7,      EK_8,         EK_9,       EK_COLON,    EK_SEMICOLON, EK_COMMA, EK_MINUS, EK_NONE },
            { EK_PERIOD, EK_SLASH,     EK_AT,      EK_A,        EK_B,         EK_C,     EK_D,     EK_NONE },
            { EK_E,      EK_F,         EK_G,       EK_H,        EK_I,         EK_J,     EK_K,     EK_NONE },
            { EK_L,      EK_M,         EK_N,       EK_O,        EK_P,         EK_Q,     EK_R,     EK_NONE },
            { EK_S,      EK_T,         EK_U,       EK_V,        EK_W,         EK_X,     EK_Y,     EK_NONE },
            { EK_Z,      EK_LBRACKET,  EK_BKSLASH, EK_RBRACKET, EK_CARET,     EK_BSP,   EK_SPACE, EK_NONE },
            { EK_RIGHT,  EK_LEFT,      EK_UP,      EK_DOWN,     EK_CR,        EK_CLEAR, EK_HOME,  EK_NONE }
        };

        const EmuKey m_ctrlKeyMatrixRk[8] = {
            EK_NONE, EK_NONE, EK_NONE, EK_NONE, EK_NONE, EK_SHIFT, EK_CTRL, EK_LANG
        };

        const EmuKey m_ctrlKeyMatrixPartner[8] = {
            EK_NONE, EK_NONE, EK_NONE, EK_NONE, EK_LANG, EK_SHIFT, EK_CTRL, EK_NONE
        };

        const EmuKey m_ctrlKeyMatrixMikrosha[8] = {
            EK_NONE, EK_NONE, EK_NONE, EK_NONE, EK_NONE, EK_LANG, EK_CTRL, EK_SHIFT
        };

        const EmuKey m_ctrlKeyMatrixMikro80[8] = {
            EK_LANG, EK_CTRL, EK_SHIFT, EK_NONE, EK_NONE, EK_NONE, EK_NONE, EK_NONE
        };

        RkKbdMatrix m_matrixKind = RKM_RK;

        uint8_t m_keys[8];
        uint8_t m_ctrlKeys;
        uint8_t m_mask;

};


class Ms7007Keyboard : public Keyboard
{
    public:
        Ms7007Keyboard();

        void resetKeys() override;
        void processKey(EmuKey key, bool isPressed) override;

        void setMatrixMask(uint8_t mask);
        uint16_t getMatrixData();

        static EmuObject* create(const EmuValuesList&) {return new Ms7007Keyboard();}

    private:

    const EmuKey m_keyMatrix[8][11] = {
            {EK_NP_COMMA,    EK_ESC,       EK_SEMICOLON, EK_F1,    EK_F2,         EK_F3,   EK_4,      EK_F4,    EK_F5,     EK_7,        EK_8        },
            {EK_NP_MINUSDIV, EK_TAB,       EK_J,         EK_1,     EK_2,          EK_3,    EK_E,      EK_5,     EK_6,      EK_LBRACKET, EK_RBRACKET },
            {EK_NONE,        EK_CTRL,      EK_F,         EK_C,     EK_U,          EK_K,    EK_P,      EK_N,     EK_G,      EK_L,        EK_D        },
            {EK_NONE,        EK_GRAPH,     EK_Q,         EK_Y,     EK_W,          EK_A,    EK_I,      EK_R,     EK_O,      EK_B,        EK_AT       },
            {EK_SHIFT,       EK_LANG,      EK_FIX,       EK_CARET, EK_S,          EK_M,    EK_SPACE,  EK_T,     EK_X,      EK_LEFT,     EK_COMMA    },
            {EK_NP_7,        EK_NP_0,      EK_NP_1,      EK_NP_4,  EK_NP_PLUSMUL, EK_BSP,  EK_RIGHT,  EK_DOWN,  EK_PERIOD, EK_BKSLASH,  EK_V        },
            {EK_NP_8,        EK_NP_PERIOD, EK_NP_2,      EK_NP_5,  EK_EXEC,       EK_SET,  EK_CR,     EK_UP,    EK_UNDSCR, EK_H,        EK_Z        },
            {EK_NP_9,        EK_NP_CR,     EK_NP_3,      EK_NP_6,  EK_RESET,      EK_HELP, EK_COLON,  EK_SLASH, EK_MINUS,  EK_0,        EK_9        }
        };

        uint16_t m_keys[8];
        uint8_t m_mask;
};


#endif  // RKKEYBOARD_H
