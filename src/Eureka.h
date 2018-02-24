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

#ifndef EUREKA_H
#define EUREKA_H


#include "Specialist.h"


class EurekaPpi8255Circuit : public SpecPpi8255Circuit
{
    public:
        EurekaPpi8255Circuit(std::string romDiskName);
        ~EurekaPpi8255Circuit();
        void setPortA(uint8_t value) override;
        void setPortC(uint8_t value) override;
        uint8_t getPortB() override;
    private:
        SpecRomDisk* m_diskCircuit;
};


#endif // APOGEY_H
