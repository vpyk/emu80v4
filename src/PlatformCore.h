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

// PlatformCore.h

#ifndef PLATFORMCORE_H
#define PLATFORMCORE_H


#include "EmuObjects.h"

class KbdLayout;
class Keyboard;
class EmuWindow;


class PlatformCore : public EmuObject
{
    public:
        PlatformCore();
        virtual ~PlatformCore();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void attachWindow(EmuWindow* win);
        void attachKeyboard(Keyboard* kbd);
        void attachKbdLayout(KbdLayout* lt);

        virtual void hrtc(bool, int) {}
        virtual void vrtc(bool) {}
        virtual void inte(bool) {}
        virtual void tapeOut(bool isActive) {m_tapeOut = isActive;}

        virtual bool getTapeOut() {return m_tapeOut;}

        virtual void draw() {}

    protected:
        EmuWindow* m_window = nullptr;

    private:
        bool m_tapeOut;
};


#endif  // PLATFORMCORE_H
