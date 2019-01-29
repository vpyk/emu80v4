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

#ifndef PPIATAADAPTER_H
#define PPIATAADAPTER_H


#include "Ppi8255Circuit.h"

class AtaDrive;

class PpiAtaAdapter : public Ppi8255Circuit
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void attachAtaDrive(AtaDrive* ataDrive) {m_ataDrive = ataDrive;}

        // derived from Ppi8255Circuit
        void setPortA(uint8_t value) override; // port 50h
        void setPortB(uint8_t value) override; // port 51h
        void setPortC(uint8_t value) override; // port 52h
        uint8_t getPortB() override; // port 51h
        uint8_t getPortC() override; // port 52h

        static EmuObject* create(const EmuValuesList&) {return new PpiAtaAdapter();}

    private:
        AtaDrive* m_ataDrive = nullptr;

        uint8_t m_portAValue = 0;
        uint8_t m_portBValue = 0;
        uint8_t m_portCValue = 0;
};


#endif // PPIATAADAPTER_H
