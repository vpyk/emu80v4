/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2023
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

// Pit8253Sound.h

#ifndef PIT8253SOUND_H
#define PIT8253SOUND_H

#include "SoundMixer.h"

class Pit8253;


class Pit8253SoundSource : public SoundSource
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        int calcValue() override;

        void attachPit(Pit8253* pit);
        virtual void tuneupPit() {}

        static EmuObject* create(const EmuValuesList&) {return new Pit8253SoundSource();}

    protected:
        Pit8253* m_pit = nullptr;
};


class RkPit8253SoundSource : public Pit8253SoundSource
{
    public:
        int calcValue() override;
        void tuneupPit() override;
        void reset() override;

        static EmuObject* create(const EmuValuesList&) {return new RkPit8253SoundSource();}
};


#endif // PIT8253SOUND_H
