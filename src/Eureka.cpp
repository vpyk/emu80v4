/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2018
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

#include "Eureka.h"


EurekaPpi8255Circuit::EurekaPpi8255Circuit(std::string romDiskName)
{
    m_romDisk = new SpecRomDisk(romDiskName);
}


EurekaPpi8255Circuit::~EurekaPpi8255Circuit()
{
    delete m_romDisk;
}


void EurekaPpi8255Circuit::setPortA(uint8_t value)
{
    SpecPpi8255Circuit::setPortA(value);
    m_romDisk->setPortA(value);
}


void EurekaPpi8255Circuit::setPortC(uint8_t value)
{
    SpecPpi8255Circuit::setPortC(value);
    m_romDisk->setPortC(value);
}


uint8_t EurekaPpi8255Circuit::getPortB()
{
    return SpecPpi8255Circuit::getPortC() && m_romDisk->getPortC();
}
