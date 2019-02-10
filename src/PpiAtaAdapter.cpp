/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2018-2019
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

#include "Emulation.h"
#include "PpiAtaAdapter.h"
#include "AtaDrive.h"

using namespace std;


void PpiAtaAdapter::setPortA(uint8_t value)
{
    if (m_ataDrive && (value & 0x80) && !(m_portAValue & 0x80)) {
        //reset
        m_ataDrive->reset();
    return;
    }

    int addr =  value & 7;
    int cs0 = (value & 0x18) >> 3;

    if (cs0 != 1)
        return; // device control and alternate status registers are not implemented

    if ((value & 0x20) && !(m_portAValue & 0x20)) {
        // write
        if (m_ataDrive)
            m_ataDrive->writeReg(addr, addr ? m_portCValue : m_portBValue << 8 | m_portCValue);
    } else if ((value & 0x40) && !(m_portAValue & 0x40)) {
        // read
        if (m_ataDrive) {
            uint16_t val = m_ataDrive->readReg(addr);
            if (!addr)
                m_portBValue = val >> 8;
            m_portCValue = val & 0xFF;
        }
    }

    m_portAValue = value;
}


void PpiAtaAdapter::setPortB(uint8_t value)
{
    m_portBValue = value;
}


void PpiAtaAdapter::setPortC(uint8_t value)
{
    m_portCValue = value;
}


uint8_t PpiAtaAdapter::getPortB()
{
    return m_portBValue;
}


uint8_t PpiAtaAdapter::getPortC()
{
    return m_portCValue;
}


bool PpiAtaAdapter::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "ataDrive") {
        attachAtaDrive(static_cast<AtaDrive*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}
