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

// Ppi8255Circuit.h

// Реализация класса обвязки контроллера программируемого перефирийного интерфейса КР580ВВ55 (i8255), заголовочный файл

#ifndef PPI8255CIRCUIT_H
#define PPI8255CIRCUIT_H

#include "EmuObjects.h"


// Обвязка ВВ55, базовый класс
class Ppi8255Circuit : public EmuObject
{
    public:
        virtual uint8_t getPortA() {return 0xFF;};
        virtual uint8_t getPortB() {return 0xFF;};
        virtual uint8_t getPortC() {return 0xFF;};
        virtual void setPortA(uint8_t) {};
        virtual void setPortB(uint8_t) {};
        virtual void setPortC(uint8_t) {};
        virtual void setPortAMode(bool) {};
        virtual void setPortBMode(bool) {};
        virtual void setPortCLoMode(bool) {};
        virtual void setPortCHiMode(bool) {};
};


#endif // PPI8255CIRCUIT_H

