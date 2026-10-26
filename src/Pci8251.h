/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2026
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

// Pci8251.h

// Implementation of Programmable Communication Interface i8251 (RJ580VV51), header file

#ifndef PCI8251_H
#define PCI8251_H

#include "EmuObjects.h"


// i8251
class Pci8251 : public AddressableDevice
{
    public:
        void initConnections() override;

        void reset() override; // Chip reset

        // derived from AddressableDevice
        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        static EmuObject* create(const EmuValuesList&) {return new Pci8251();}

    private:
        bool m_idle = true; // Set after chip reset, reset after mode instruction
        int m_bitMask = 0xff;

        uint8_t m_outBuffer[2] = {0, 0};
        int m_outBufferLen = 0;
        uint8_t m_inData = 0;
        bool m_inDataWasRead = true;

        // All signals are logical (true = active), inversion to pin levels can be done in config
        // Unconnected inputs are considered active

        // Inputs
        bool m_cts = true;
        bool m_dsr = true;

        // Outputs
        bool m_dtr = false;
        bool m_rts = false;

        // Internal state
        bool m_txen = false;
        bool m_rxe = false;
        bool m_oe = false;

        void setData(uint8_t val);
        void setCts(bool val);
        void setDsr(bool val);

        EmuOutput* m_dataOutput = nullptr;
        EmuOutput* m_dtrOutput = nullptr;
        EmuOutput* m_rtsOutput = nullptr;
        EmuOutput* m_txrdyOutput = nullptr;
        EmuOutput* m_rxrdyOutput = nullptr;

        void internalReset();
        void tryTransmit();
        void updateTxRdy();
        void updateOutputs();
};

#endif // PCI8251_H
