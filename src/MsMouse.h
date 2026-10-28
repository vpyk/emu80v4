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
// "data" output -> serial port data input, serial port rts output -> "rts" input,
// serial port rxrdy output -> "rxrdy" input (optional, see below).
// Line format 1200 baud 7N1: a byte every 9 bit times (7.5 ms), the mouse paces the bytes itself.
// The mouse is powered by RTS: when RTS becomes active it sends the identifier 'M' (4Dh).
// DTR isn't used: e.g. the Korvet ABRIS editor and CP/M BIOS set RTS only.
// When the mouse moves or the buttons change it sends a 3-byte packet:
//   1 L R Y7 Y6 X7 X6  (bit 6 marks the first byte)
//   0 0 X5 X4 X3 X2 X1 X0
//   0 0 Y5 Y4 Y3 Y2 Y1 Y0
// dX right > 0, dY down > 0, -128..127; the excess goes to the next packets.
// Inputs are logical (true = active), unconnected rts is considered active.
// While powered, the mouse always sends its movement. It is active (the pointer can be captured) while
// the program reads the bytes: a falling edge of rxrdy after a byte was sent means the byte was read.
// The mouse becomes inactive when a sent byte stays unread for activityTimeout ms of emulated time,
// e.g. on Korvet CP/M BIOS sets RTS, but nobody reads the mouse. Without rxrdy connected the mouse
// is active while powered.
// Properties (see MouseDevice): relative (default yes), scale (default 1),
// activityTimeout (default 1000, 0 - once read, the mouse stays active while powered).
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
        bool acceptsInput() override {return m_powered;}
        void onMove(int dx, int dy) override;
        std::string getMouseInfoExt() override;

    private:
        static const int c_bitsPerByte = 9; // start bit, 7 data bits, stop bit

        bool m_rts = true;
        bool m_powered = false;

        bool m_rxRdyConnected = false;  // rxrdy input has got a value at least once
        bool m_rxRdy = false;
        bool m_unread = false;          // a sent byte hasn't been read yet
        uint64_t m_unreadClock = 0;     // when the oldest unread byte was sent
        bool m_reader = false;          // the program reads the bytes

        uint8_t m_queue[3] = {0, 0, 0};
        int m_queueLen = 0;
        int m_queuePos = 0;

        int m_accX = 0;         // right > 0
        int m_accY = 0;         // down > 0
        int m_lastButtons = 0;

        EmuOutput* m_dataOutput = nullptr;

        void setRts(bool val);
        void setRxRdy(bool val);
        void makePacket();
};

#endif // MSMOUSE_H
