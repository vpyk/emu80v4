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

// RkPpi8255Circuit.h

#ifndef RKPPI8255CIRCUIT_H
#define RKPPI8255CIRCUIT_H

#include "Ppi8255Circuit.h"

class GeneralSoundSource;
class RkKeyboard;


// Обвязка основного ВВ55 в РК86, Апогее и Орионе
class RkPpi8255Circuit : public Ppi8255Circuit
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        uint8_t getPortA() override;
        uint8_t getPortB() override;
        uint8_t getPortC() override;
        void setPortA(uint8_t value) override;
        void setPortB(uint8_t) override {}
        void setPortC(uint8_t value) override;

        // Подключение объекта - клавиатуры типа РК86
        /*virtual */void attachRkKeyboard(RkKeyboard* kbd);

        static EmuObject* create(const EmuValuesList&) {return new RkPpi8255Circuit();}

    protected:
        // Источник звука - вывод на магнитофон
        GeneralSoundSource* m_tapeSoundSource;

        // Клавиатура типа РК86
        RkKeyboard* m_kbd = nullptr;
};


#endif // RKPPI8255CIRCUIT_H


