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

// MsMouse.h

// Microsoft serial mouse, header file

#ifndef MSMOUSE_H
#define MSMOUSE_H

#include "EmuObjects.h"
#include "MouseDevice.h"


// Two-button Microsoft serial mouse, platform independent. Connected to a serial port (e.g. Pci8251):
// "data" output -> serial port data input, serial port dtr/rts outputs -> "dtr"/"rts" inputs.
// Line format 1200 baud 7N1: a byte every 9 bit times (7.5 ms), the mouse paces the bytes itself.
// The mouse is powered by DTR and RTS: when both become active it sends the identifier 'M' (4Dh).
// When the mouse moves or the buttons change it sends a 3-byte packet:
//   1 L R Y7 Y6 X7 X6  (bit 6 marks the first byte)
//   0 0 X5 X4 X3 X2 X1 X0
//   0 0 Y5 Y4 Y3 Y2 Y1 Y0
// dX right > 0, dY down > 0, -128..127; the excess goes to the next packets.
// Inputs are logical (true = active), unconnected inputs are considered active.
// The mouse is active while powered.
// Properties (see MouseDevice): relative (default yes), scale (default 1).
class MsMouse : public MouseDevice, public IActive
{
    public:
        MsMouse();

        void reset() override;
        void initConnections() override;
        void operate() override;

        bool isMouseActive() override;

        static EmuObject* create(const EmuValuesList&) {return new MsMouse();}

    protected:
        void onMove(int dx, int dy) override;
        std::string getMouseInfoExt() override;

    private:
        static const int c_bitsPerByte = 9; // start bit, 7 data bits, stop bit

        bool m_dtr = true;
        bool m_rts = true;
        bool m_powered = false;

        uint8_t m_queue[3] = {0, 0, 0};
        int m_queueLen = 0;
        int m_queuePos = 0;

        int m_accX = 0;         // right > 0
        int m_accY = 0;         // down > 0
        int m_lastButtons = 0;

        EmuOutput* m_dataOutput = nullptr;

        void setDtr(bool val);
        void setRts(bool val);
        void updatePower();
        void makePacket();
};

#endif // MSMOUSE_H
