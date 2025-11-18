/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2025
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

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "KbdLayout.h"


class Keyboard : public EmuObject
{
    public:
        void reset() override {if (m_keysResetAllowed) resetKeys();}

        virtual void resetKeys() = 0;
        virtual void processKey(EmuKey /*key*/, bool /*isPressed*/) {};
        virtual bool processKeyCode(int /*keyCode*/) {return false;} // returns true if processing is done on this stage

        void enableKeysReset() {m_keysResetAllowed = true;}
        void disableKeysReset() {m_keysResetAllowed = false;}

    private:
        bool m_keysResetAllowed = true;
};

#endif  // KEYBOARD_H

