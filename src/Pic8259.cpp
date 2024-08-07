﻿/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2021-2024
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

// Pic8259.cpp

// Реализация контроллера прерываний КР580ВН59

#include <sstream>
#include <bitset>

#include "Globals.h"
#include "Pic8259.h"
#include "Cpu.h"
#include "Emulation.h"

using namespace std;


void Pic8259::writeByte(int addr, uint8_t value)
{
    addr &= 0x01;

    if (!m_curIcwIndex) {
        if (addr) {
            // OCW1
            m_imr = value;
        } else if (value & 0x10) {
            // ICW1
            m_curInServiceLevel = 8;
            m_curRequestLevel = 8;
            m_imr = 0;
            m_irr = 0;
            m_levelMode = value & 8;
            m_addrInterval = value & 4 ? 4 : 8;
            m_highestPrio = 0;
            m_curIcwIndex = 1;
            m_autoEoi = false;
            m_rotateOnAeoi = false;
            m_totalIcws = 2;
            if (!(value & 2))
                m_totalIcws = value & 1 ? 4 : 3;
            m_isrPageAddr = (m_isrPageAddr & 0xFF00) | (value & 0xE0);
            if (m_addrInterval == 8)
                m_isrPageAddr &= ~0x20;
        } else if (value & 0x08) {
            // OCW3
            if (value & 2)
                m_readIsrFlag = value & 1;
            m_pollMode = value & 4;
            if (value & 0x40)
                m_specialMask = value & 0x20;
        } else {
            // OCW2
            int level = (value & 0x40) ? (value & 7) : m_curInServiceLevel;
            switch (value >> 5) {
            case 1: // EOI
            case 3: // EOI spec.
                eoi(level);
                break;
            case 5: // EOI + rot
            case 7: // EOI spec. + rot
                eoi(level);
                m_highestPrio = (level + 1) & 7;
                break;
            case 6: // spec. prio
                m_highestPrio = level;
                break;
            case 0:
            case 4:
                // clear/set rot. on AEOI
                m_rotateOnAeoi = value & 4;
                break;
            default: // 2 - no op
                break;
            }
        }
    } else {
        if (m_curIcwIndex == 1)
            // ICW2
            m_isrPageAddr = (m_isrPageAddr & 0x00FF) | (value << 8);
        else if (m_curIcwIndex == 3)
            // ICW4
            m_autoEoi = value & 2;

        if (++m_curIcwIndex == m_totalIcws)
            m_curIcwIndex = 0;
    }
}



uint8_t Pic8259::readByte(int addr)
{
    addr &= 1;

    if (addr == 1)
        return m_imr;

    if (!m_pollMode)
        return m_readIsrFlag ? m_isr : m_irr;
    else {
        m_pollMode = false;
        int ret = m_curInServiceLevel == 8 ? 0 : m_curInServiceLevel | 0x80; // todo: implement poll mode!
        //eoi(m_curLevel);
        return ret;
    }
}


void Pic8259::irq(int vect, bool state)
{
    uint8_t mask = 1 << vect;

    if (state) {
        if (m_levelMode || !(m_irs & mask)) {
            m_irr |= mask;
            if (!(m_imr & mask)) {
                serviceInt();
            }
        }
        m_irs |= mask;
    } else {
        m_irs &= ~mask;
        m_irr &= ~mask;
    }
}


void Pic8259::inte(bool active)
{
    m_inte = active;
    if (active && (m_irr & ~m_imr)) {
        serviceInt();
    }
}


bool Pic8259::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "cpu") {
        m_cpu = static_cast<Cpu8080Compatible*>(g_emulation->findObject(values[0].asString()));
        return true;
    } else if (propertyName == "addrSpace") {
        m_as = static_cast<AddressableDevice*>(g_emulation->findObject(values[0].asString()));
        return true;
    }

    return false;
}


void Pic8259::serviceInt()
{
    updateCurLevels();

    if (m_curInServiceLevel == 8 || (((m_curRequestLevel + m_highestPrio) & 7) < ((m_curInServiceLevel + m_highestPrio) & 7)) || m_specialMask)
    {
        if (m_inte) {
            m_isr |= (1 << m_curRequestLevel);
            m_irr &= ~(1 << m_curRequestLevel);
            updateCurLevels();
            m_cpu->intCall(m_isrPageAddr + m_curInServiceLevel * m_addrInterval);
            if (m_autoEoi) {
                eoi(m_curInServiceLevel);
            }
        }
    }
}


void Pic8259::updateCurLevels()
{
    m_curInServiceLevel = 8;
    m_curRequestLevel = 8;

    uint8_t maskedIrr = m_irr & ~m_imr;
    int level = m_highestPrio;
    for (int i = 0; i < 8; i++)
    {
        uint8_t mask = 1 << level;
        if (!m_curIcwIndex && maskedIrr & mask) {
            m_curRequestLevel = level;
            break;
        }
        level = (level + 1) & 7;
    }

    level = m_highestPrio;
    for (int i = 0; i < 8; i++)
    {
        uint8_t mask = 1 << level;
        if (!m_curIcwIndex && m_isr & mask) {
            m_curInServiceLevel = level;
            break;
        }
        level = (level + 1) & 7;
    }
}


void Pic8259::eoi(int level)
{
    if (level == 8) // no interrupt in progress
        return;

    m_isr &= ~(1 << level);
    if (m_rotateOnAeoi)
        m_highestPrio = (level + 1) & 7;

    if (m_irr & ~m_imr)
        serviceInt();
    else
        updateCurLevels();
}


string Pic8259::getDebugInfo()
{
   stringstream ss;
   ss << "PIC i8259:\n";
   //ss << "LEV 76543210\n";
   ss << "IRR " << bitset<8>(m_irr) << "\n";
   ss << "IMR " << bitset<8>(m_imr) << "\n";
   ss << "ISR " << bitset<8>(m_isr) << "\n";
   return ss.str();
}
