/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2018-2023
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

#include "Globals.h"
#include "Emulation.h"
#include "PpiAtaAdapter.h"
#include "AtaDrive.h"

using namespace std;


void PpiAtaAdapter::setPortA(uint8_t value)
{
    int addr =  value & 7;
    int cs = (value & 0x18) >> 3;
    bool iow = value & 0x20;
    bool ior = value & 0x40;
    bool reset = value & 0x80;

    m_ataDrive->writeControl(cs, addr, ior, iow, reset);
}


void PpiAtaAdapter::setPortB(uint8_t value)
{
    m_portBWrValue = value;
    m_ataDrive->writeData(m_portBWrValue << 8 | m_portCWrValue);
}


void PpiAtaAdapter::setPortC(uint8_t value)
{
    m_portCWrValue = value;
    m_ataDrive->writeData(m_portBWrValue << 8 | m_portCWrValue);
}


uint8_t PpiAtaAdapter::getPortB()
{
    return m_ataDrive->readData() >> 8;
}


uint8_t PpiAtaAdapter::getPortC()
{
    return m_ataDrive->readData() & 0xFF;
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
