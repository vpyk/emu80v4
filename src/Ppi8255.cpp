/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2022
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

// Ppi8255.cpp

// Реализация контроллера программируемого перефирийного интерфейса КР580ВВ55

#include "Globals.h"
#include "Ppi8255.h"
#include "Ppi8255Circuit.h"
#include "Emulation.h"

using namespace std;

Ppi8255::Ppi8255()
{
    Ppi8255::reset();
}


void Ppi8255::reset()
{
    if (m_noReset)
        return;

    m_portA = 0;
    m_portB = 0;
    m_portC = 0;
    m_chAMode = PCM_IN;
    m_chBMode = PCM_IN;
    m_chCHiMode = PCM_IN;
    m_chCLoMode = PCM_IN;
    if (m_ppiCircuit) {
        m_ppiCircuit->setPortAMode(true);
        m_ppiCircuit->setPortBMode(true);
        m_ppiCircuit->setPortCLoMode(true);
        m_ppiCircuit->setPortCHiMode(true);
    }
}



void Ppi8255::writeByte(int addr, uint8_t value)
{
    addr &= 0x03;
    switch (addr) {
        case 0:
            if (m_chAMode == PCM_OUT) {
                m_portA = value;
                if (m_ppiCircuit)
                    m_ppiCircuit->setPortA(value);
            }
            break;
        case 1:
            if (m_chBMode == PCM_OUT) {
                m_portB = value;
                if (m_ppiCircuit)
                    m_ppiCircuit->setPortB(value);
            }
            break;
        case 2:
            m_portC = value;
            if (m_chCHiMode != PCM_OUT)
                m_portC &= 0x0F;
            if (m_chCLoMode != PCM_OUT)
                m_portC &= 0xF0;
            if (m_ppiCircuit && (m_chCLoMode == PCM_OUT || m_chCHiMode == PCM_OUT))
                    m_ppiCircuit->setPortC(m_portC);
            break;
        default: //ctrl reg
            if (!(value & 0x80)) {
                int bit = (value & 0x0e) >> 1;
                if ((bit < 4 && m_chCLoMode == PCM_OUT) || (bit >= 4 && m_chCHiMode == PCM_OUT)) {
                    uint8_t mask = ~(1 << bit);
                    m_portC &= mask;
                    m_portC |= ((value & 1) << bit);
                    if (m_ppiCircuit)
                        m_ppiCircuit->setPortC(m_portC);
                }
            } else if (!(value & 0x40)) {
                // установка режима, недвунаправленный режим
                m_portA = 0x0;
                m_portB = 0x0;
                m_portC = 0x0;

                m_chAMode = value & 0x10 ? PCM_IN : PCM_OUT;
                m_chBMode = value & 0x02 ? PCM_IN : PCM_OUT;
                m_chCHiMode = value & 0x08 ? PCM_IN : PCM_OUT;
                m_chCLoMode = value &0x01 ? PCM_IN : PCM_OUT;
                // режимы 0 и 1 пока не различаются

                if (m_ppiCircuit) {
                    m_ppiCircuit->setPortAMode(m_chAMode == PCM_IN);
                    m_ppiCircuit->setPortA(0);

                    m_ppiCircuit->setPortBMode(m_chBMode == PCM_IN);
                    m_ppiCircuit->setPortB(0);

                    m_ppiCircuit->setPortCLoMode(m_chCLoMode == PCM_IN);
                    m_ppiCircuit->setPortCHiMode(m_chCHiMode == PCM_IN);
                    m_ppiCircuit->setPortC(0);
                }
            }  else {
                // двунаправленный режим, пока просто сброс значений регистров
                m_portA = 0x0;
                m_portB = 0x0;
                m_portC = 0x0;
            }

    }
}



uint8_t Ppi8255::readByte(int addr)
{
    addr &= 3;
    switch (addr) {
        case 0:
            if (m_chAMode == PCM_IN)
                m_portA = m_ppiCircuit ? m_ppiCircuit->getPortA() : 0;
            return m_portA;
        case 1:
            if (m_chBMode == PCM_IN)
                m_portB = m_ppiCircuit ? m_ppiCircuit->getPortB() : 0;
            return m_portB;
        case 2:
            if (m_chCLoMode == PCM_IN || m_chCHiMode == PCM_IN) {
                uint8_t portC = m_ppiCircuit ? m_ppiCircuit->getPortC() : 0;
                if (m_chCLoMode == PCM_IN)
                    m_portC = (m_portC & 0xf0) | (portC & 0x0f);
                if (m_chCHiMode == PCM_IN)
                    m_portC = (m_portC & 0x0f) | (portC & 0xf0);
            }
            return m_portC;

        default:
            // ctrl reg
            return 0xFF; // undefined
    }
}



void Ppi8255::attachPpi8255Circuit(Ppi8255Circuit* circuit)
{
    m_ppiCircuit = circuit;
}



bool Ppi8255::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "circuit") {
        attachPpi8255Circuit(static_cast<Ppi8255Circuit*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "noReset") {
        if (values[0].asString() == "yes") {
            m_noReset = true;
            return true;
        } else if (values[0].asString() == "no") {
            m_noReset = false;
            return true;
        }
        return false;
    }

    return false;
}
