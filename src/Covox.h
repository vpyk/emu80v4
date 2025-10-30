/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2025
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

// Covox.h

#ifndef COVOX_H
#define COVOX_H

#include "SoundMixer.h"


// Простой источник звука
class Covox : public SoundSource
{
    public:
        Covox(int bits);

        void initConnections() override;

        // derived from SoundSOurce
        int calcValue() override;

        // Установка текущего значения источника звука
        void setValue(int value);

        static EmuObject* create(const EmuValuesList& parameters) {return new Covox(parameters[0].asInt());}

    private:
        int m_bits;

        int m_curValue = 0;
        uint64_t m_initClock = 0;
        uint64_t m_prevClock = 0;
        int m_sumVal = 0;

        void updateStats();
};


#endif // COVOX_H
