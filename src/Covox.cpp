/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2020-2021
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

// Covox.cpp

#include "Emulation.h"

//#include "Pal.h"
#include "Covox.h"

using namespace std;


Covox::Covox(int bits)
{
    m_bits = bits;
}


void Covox::setValue(int value)
{
    updateStats();
    m_curValue = value;
}


// Обновляет внутренние счетчики, вызывается перед установкой нового значения либо перед получением текущего
void Covox::updateStats()
{
    uint64_t curClock = g_emulation->getCurClock();

    int clocks = curClock - m_prevClock;
    m_sumVal += clocks * m_curValue;

    m_prevClock = curClock;
}

// Получение текущего значения
int Covox::calcValue()
{
    updateStats();

    int res = 0;

    uint64_t ticks = g_emulation->getCurClock() - m_initClock;
    if (ticks)
        res = int64_t(m_sumVal) * MAX_SND_AMP / ticks >> (m_bits - 1);
    m_sumVal = 0;
    m_initClock = g_emulation->getCurClock();

    return res * m_ampFactor;
}
