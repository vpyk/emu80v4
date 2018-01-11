/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2017
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

// Pit8253.cpp

// Реализация программируемого интервального таймера КР580ВИ53


#include "Emulation.h"
#include "Pit8253.h"

Pit8253Counter::Pit8253Counter(Pit8253* pit, int number)
{
    //g_emulation->registerDevice(this);
    m_pit = pit;
    m_number = number;
    m_prevClock = g_emulation->getCurClock();
    m_isCounting = false;
    m_gate = true;
    m_out = false;
    m_counterInitValue = 0xffff;
    m_counter = 0xffff;
    m_mode = 0;
}


void Pit8253Counter::operateForTicks(int ticks)
{
    if (!m_gate) {
        if (m_out)
            m_tempSumOut += ticks;
        return;
    }

    switch (m_mode) {
        case 0:
            //m_tempSumOut = 0;
            if (m_isCounting && !m_out) {
                if (ticks >= m_counter) {
                    m_tempSumOut += (ticks - m_counter);
                    m_isCounting = false;
                    m_out = true;
                    ++m_sumOutTicks;
                }
            } else
                m_tempSumOut += ticks;
            m_counter = (m_counter - ticks) & 0xffff;
            break;
        case 3:
            {
                //m_tempSumOut = 0;

                if (!m_isCounting)
                    m_tempSumOut += ticks;

                int hiPeriod = (m_counterInitValue + 1) / 2;
                int loPeriod = m_counterInitValue / 2;

                int fullCycles = ticks / m_counterInitValue;
                if (m_isCounting) {
                    m_tempSumOut += fullCycles * hiPeriod;
                    m_sumOutTicks += fullCycles;
                }
                ticks -= fullCycles * m_counterInitValue;

                int counter = m_out ? (m_counter + 1) / 2 : m_counter / 2;

                int last = counter;
                int curPeriod = m_out ? hiPeriod : loPeriod;
                int nextPeriod = m_out ? loPeriod : hiPeriod;

                if (ticks < last) {
                    counter -= ticks;
                    if (m_isCounting && m_out)
                        m_tempSumOut += ticks;
                } else if (ticks < last + nextPeriod) {
                    counter = nextPeriod - (ticks - last);
                    if (m_isCounting) {
                        if (m_out)
                            m_tempSumOut += last;
                        else
                            m_tempSumOut += (ticks - last);
                    }
                    m_out = !m_out;
                    if (m_out)
                        ++m_sumOutTicks;
                } else { // if (ticks >= last + nextPeriod)
                    counter = curPeriod - (ticks - last - nextPeriod);
                    if (m_isCounting) {
                        if (m_out)
                            m_tempSumOut += (ticks - nextPeriod);
                        else
                            m_tempSumOut += nextPeriod;
                    }
                }
                m_counter = counter * 2;
                if (m_counter > m_counterInitValue)
                    --m_counter;
            }
            break;
        case 1:
        case 2:
        case 4:
        case 5:
        default:
            if (m_out)
                m_tempSumOut += ticks;
            break;
    }
}


void Pit8253Counter::updateState()
{
    int dt = g_emulation->getCurClock() - m_prevClock;

    if (m_prevFastClock >= m_kDiv * 1024) {
        m_prevFastClock -= m_kDiv * 1024;
    } 

    const uint32_t curFastClock = m_prevFastClock + dt;
    int ticks = curFastClock / m_kDiv - m_prevFastClock / m_kDiv;

    uint64_t curClock = g_emulation->getCurClock();

    if (m_out || !m_isCounting)
        m_tempAddOutClocks -= (m_prevFastClock % m_kDiv);
        //m_tempAddOutClocks += (m_kDiv - m_prevClock % m_kDiv) % m_kDiv;

    operateForTicks(ticks);

    if (m_out || !m_isCounting)
        m_tempAddOutClocks += curFastClock % m_kDiv;

//    m_avgOut = 0;
//    if (curClock != m_prevClock)
//        m_avgOut = m_tempSumOut * 9 * 4096 / (curClock - m_prevClock + addClock);

    m_prevClock = curClock;
    m_prevFastClock = curFastClock;
}


int Pit8253Counter::getAvgOut()
{
    uint64_t curClock = g_emulation->getCurClock();
    m_avgOut = 0;
    if (curClock != m_sampleClock) {
        uint32_t dt = curClock - m_sampleClock;
        m_avgOut = (m_tempSumOut * m_kDiv + m_tempAddOutClocks) * 4096 / dt;
        //m_avgOut = m_tempSumOut * 4096 / (curClock / 9 - m_sampleClock / 9);
    }
    return m_avgOut;
}

void Pit8253Counter::resetStats()
{
    m_avgOut = 0;
    m_sumOutTicks = 0;
    m_tempSumOut = 0;
    m_tempAddOutClocks = 0;
    m_prevClock = g_emulation->getCurClock();
    m_prevFastClock = 0;
    m_sampleClock = m_prevClock;
}


void Pit8253Counter::setMode(int mode)
{
    if (!m_extClockMode)
        updateState();

    m_mode = mode;

    switch (m_mode) {
        case 0:
            m_out = false;
            m_isCounting = false;
            break;
        case 3:
            m_isCounting = false;
            m_out = true;
            break;
        case 1:
        case 2:
        case 4:
        case 5:
            // not implemented yet
            m_isCounting = false;
            m_out = true;
            break;
        default:
            break;
    }
}


void Pit8253Counter::setHalfOfCounter()
{
    //updateState();

    switch (m_mode) {
        case 0:
            m_isCounting = false;
        case 3:
            break;
        case 1:
        case 2:
        case 4:
        case 5:
            // not implemented yet
            break;
    }
}


void Pit8253Counter::setCounter(uint16_t counter)
{
    if (!m_extClockMode)
        updateState();

    m_counterInitValue = counter;

    if (m_counterInitValue == 0)
        m_counterInitValue = 0x10000;

    switch (m_mode) {
        case 0:
            m_counter = m_counterInitValue;
            m_isCounting = true;
            m_out = false;
            break;
        case 3:
            if (!m_isCounting)
                m_counter = m_counterInitValue;
            m_isCounting = true;
            break;
        case 1:
        case 2:
        case 4:
        case 5:
        default:
            // not implemented yet
            break;
    }
}


void Pit8253Counter::setGate(bool gate)
{
    if (gate == m_gate)
        return;

    if (!m_extClockMode)
        updateState();

    m_gate = gate;
    switch (m_mode) {
        case 0:
            m_isCounting = gate;
            break;
        case 3:
            m_isCounting = gate;
            m_out = !gate;
            if (gate)
                m_counter = m_counterInitValue;
            break;
        default:
            break;
    }
}


bool Pit8253Counter::getOut()
{
    switch (m_mode) {
        case 0:
            return m_out;
        case 3:
            return (m_isCounting && m_out) || !m_gate;
        default:
            return true;
    }
}


Pit8253::Pit8253()
{
    for (int i = 0; i < 3; i++) {
        m_counters[i] = new Pit8253Counter(this, i);
        m_counters[i]->m_kDiv = m_kDiv;
    }
    reset();
}


Pit8253::~Pit8253()
{
    for (int i = 0; i < 3; i++)
        delete m_counters[i];
}


void Pit8253::setFrequency(int64_t freq)
{
    EmuObject::setFrequency(freq);
    for (int i = 0; i < 3; i++)
        m_counters[i]->m_kDiv = m_kDiv;
}


void Pit8253::reset()
{
    for (int i = 0; i < 3; i++) {
        m_latches[i] = 0;
    }
}


void Pit8253::updateState()
{
    for (int i = 0; i < 3; i++)
        m_counters[i]->updateState();
}


bool Pit8253::getOut(int counter)
{
    return m_counters[counter]->getOut();
}


void Pit8253::writeByte(int addr, uint8_t value)
{
    addr &= 0x3;

    if (addr == 0x03) {
        // CSW
        int counterNum = (value & 0xC0) >> 6;
        if (counterNum == 3)
            return; // некорректный номер счетчика
        uint8_t loadMode = (value & 0x30) >> 4;
        uint8_t counterMode = (value & 0x0E) >> 1;
        if (counterMode == 6 || counterMode == 7)
            counterMode &= 3;
        if (loadMode == PRLM_LATCH) {
            // команда защелкивания
            m_latched[counterNum] = true;
            m_latches[counterNum] = m_counters[counterNum]->m_counter;
            m_waitingHi[counterNum] = false;
        } else {
            // установка режима счетчика
            m_latches[counterNum] = 0;
            m_rlModes[counterNum] = (PitReadLoadMode)loadMode;
//            bPITCountBCD[nCnt]=bValue&0x01;
            m_latched[counterNum] = false;
            m_waitingHi[counterNum] = (loadMode == PRLM_HIGHBYTE);
            //m_PITWState[nCnt]=(bLoadMode==rlHighByte)?rwstWaitHigh:rwstWaitLow;
          //m_counters[counterNum]->setCounter(0); //!!!???
            m_counters[counterNum]->setMode(counterMode);
        }

    } else {
        // регистр счетчика
        if (!m_waitingHi[addr]) {
            m_latches[addr] = (m_latches[addr] & 0xff00) | value;
            if (m_rlModes[addr] == PRLM_WORD) {
                m_waitingHi[addr] = true;
                m_counters[addr]->setHalfOfCounter();
            }
            else
                m_counters[addr]->setCounter(m_latches[addr]);
        }
        else { // if m_rlStates[addr] = PRLS_WAITHIGH
            m_latches[addr] = (m_latches[addr] & 0xff) | (value << 8);
            //m_waitingHi[addr] = false;
            if (m_rlModes[addr] == PRLM_WORD)
                m_waitingHi[addr] = false;
            m_counters[addr]->setCounter(m_latches[addr]);
        }
    }
}


uint8_t Pit8253::readByte(int addr)
{
    addr &= 0x3;

    if (addr == 0x03) {
        return 0xFF; //!!!
    } else {
        // регистр счетчика
        if (!m_counters[addr]->getExtClockMode())
            m_counters[addr]->updateState();
        uint16_t cntVal = m_latched[addr] ? m_latches[addr] : m_counters[addr]->m_counter;
        uint8_t res = m_waitingHi[addr] ? (cntVal & 0xff00) >> 8 : cntVal & 0xff;

        if (m_rlModes[addr] == PRLM_WORD)
            m_waitingHi[addr] = !m_waitingHi[addr];

        return res;
    }
}
