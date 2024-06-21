/*
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

// Pic8259.h

#ifndef DMA8259_H
#define DMA8259_H

#include "EmuObjects.h"

class Cpu8080Compatible;

class Pic8259 : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getDebugInfo() override;

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        void irq(int vect, bool active);
        void inte(bool active);

        static EmuObject* create(const EmuValuesList&) {return new Pic8259();}

    private:
        Cpu8080Compatible* m_cpu = nullptr;
        AddressableDevice* m_as = nullptr;

        uint8_t m_irs = 0; // IR inputs

        uint8_t m_imr = 0; // interrupt mask register
        uint8_t m_irr = 0; // interrupt request register
        uint8_t m_isr = 0; // in-service register

        int m_curIcwIndex = 0;       // 1 after ICW1, wait for ICW2, 2 after ICW2
        int m_totalIcws = 2;         // Total icw needed
        int m_addrInterval = 8;      // isr table interval
        uint16_t m_isrPageAddr = 0;  // isr table base address
        bool m_levelMode = false;    // level interrupt mode (instead of  edge)
        int m_curInServiceLevel = 8; // current in-service level
        int m_curRequestLevel = 8;   // current request level
        int m_highestPrio = 0;       // bottom priority level
        bool m_readIsrFlag = false;  // read ISR (true) or IRR (false)
        bool m_pollMode = false;     // poll mode flag
        bool m_specialMask = false;  // specialMask
        bool m_autoEoi = false;      // auto EOI
        bool m_rotateOnAeoi = false; // auto EOI
        bool m_inte = false;         // inte cpu output

        void updateCurLevels();
        void serviceInt();
        void eoi(int level);
};

#endif // DMA8259_H
