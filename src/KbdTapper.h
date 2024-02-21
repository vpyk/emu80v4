/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2024
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

#ifndef KBDTAPPER_H
#define KBDTAPPER_H

#include "EmuObjects.h"

const int c_speedUpFactor = 8;

class KbdTapper : public ActiveDevice
{
    public:
        KbdTapper();
        void operate() override;
        void reset() override;
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        void setDelayMs(uint64_t delay);

        void typeText(std::string str);

        static EmuObject* create(const EmuValuesList&) {return new KbdTapper();}

    private:
        int m_pressTime = 80;   // key down time in ms
        int m_releaseTime = 80; // key up in ms
        int m_crDelay = 250;    // CR delay in ms

        std::wstring m_string;
        unsigned m_pos = 0;

        bool m_typing = false;
        bool m_keyPressed = false;

        void sendNextKey();
        void stop();

        void scheduleKey();
};


#endif  // KBDTAPPER_H

