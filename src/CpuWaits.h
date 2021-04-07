/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2019-2021
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

#ifndef CPUWAITS_H
#define CPUWAITS_H

#include "EmuObjects.h"

// Базовый класс тактов ожидания процессора (на команду)
class CpuWaits : public EmuObject
{
    public:
        virtual int getCpuWaitStates(int memTag, int opcode, int normalClocks) = 0;
};


// Базовый класс тактов ожидания процессора (на обращение к шине)
class CpuCycleWaits : public EmuObject
{
    public:
        virtual int getCpuCycleWaitStates(int memTag, bool write) = 0;
};

#endif //CPUWAITS_H
