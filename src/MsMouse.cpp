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

// MsMouse.cpp

// Microsoft serial mouse

#include <sstream>
#include <algorithm>

#include "Globals.h"
#include "Emulation.h"
#include "MsMouse.h"


MsMouse::MsMouse()
{
    m_relative = true;
    useActivityTimeout(1000);
    pause();
    EmuObject::setFrequency(1200); // bit time
}


void MsMouse::reset()
{
    MouseDevice::reset();

    // m_rts is kept: it is set by the serial port, and the reset order of objects is undefined
    // m_rxRdy and m_rxRdyConnected are kept for the same reason
    m_powered = false;
    m_queueLen = m_queuePos = 0;
    m_unread = m_reader = false;
    pause();
    setRts(m_rts);
}


void MsMouse::initConnections()
{
    MouseDevice::initConnections();
    REG_INPUT("rts", MsMouse::setRts);
    REG_INPUT("rxrdy", MsMouse::setRxRdy);
    REG_OUTPUT("data", m_dataOutput);
}


void MsMouse::setRts(bool val)
{
    m_rts = val;
    if (val == m_powered)
        return;
    m_powered = val;
    m_unread = m_reader = false;

    if (val) {
        // power on: the earlier movement and presses are dropped, the identifier goes first
        m_accX = m_accY = 0;
        takeButtons();
        m_lastButtons = 0;
        m_queue[0] = 'M';
        m_queueLen = 1;
        m_queuePos = 0;
        resume();
        syncronize();
        m_curClock += m_kDiv * c_bitsPerByte;
    } else {
        m_queueLen = m_queuePos = 0;
        pause();
    }
}


void MsMouse::setRxRdy(bool val)
{
    m_rxRdyConnected = true;
    bool fall = m_rxRdy && !val;
    m_rxRdy = val;

    // the serial port reset also drops rxrdy, but then RTS goes off too and m_unread is cleared
    if (fall && m_powered && m_unread) {
        m_unread = false;
        m_reader = true;
    }
}


bool MsMouse::isMouseActive()
{
    if (!m_powered)
        return false;
    if (!m_rxRdyConnected)
        return true;

    if (m_reader && m_unread && activityTimeout()) {
        uint64_t timeout = uint64_t(g_emulation->getFrequency()) * activityTimeout() / 1000;
        if (g_emulation->getCurClock() - m_unreadClock >= timeout)
            m_reader = false;
    }
    return m_reader;
}


// Called once per byte time while powered
void MsMouse::operate()
{
    m_curClock += m_kDiv * c_bitsPerByte;

    if (m_queuePos == m_queueLen)
        makePacket();
    if (m_queuePos < m_queueLen) {
        if (!m_unread) {
            m_unread = true;
            m_unreadClock = g_emulation->getCurClock();
        }
        m_dataOutput->setValue(m_queue[m_queuePos++]);
    }
}


static int clampAbs(int value, int limit)
{
    return std::max(-limit, std::min(value, limit));
}


void MsMouse::onMove(int dx, int dy)
{
    m_accX = clampAbs(m_accX + dx, moveLimitX());
    m_accY = clampAbs(m_accY - dy, moveLimitY());
}


static uint8_t takeDelta(int& acc)
{
    int value = std::max(-128, std::min(acc, 127));
    acc -= value;
    return uint8_t(value);
}


// Builds a packet if there is something to report
void MsMouse::makePacket()
{
    int buttons = takeButtons() & 3;
    if (!m_accX && !m_accY && buttons == m_lastButtons)
        return;
    m_lastButtons = buttons;

    uint8_t x = takeDelta(m_accX);
    uint8_t y = takeDelta(m_accY);

    m_queue[0] = 0x40 | (buttons & 1 ? 0x20 : 0) | (buttons & 2 ? 0x10 : 0) | ((y >> 4) & 0x0C) | (x >> 6);
    m_queue[1] = x & 0x3F;
    m_queue[2] = y & 0x3F;
    m_queueLen = 3;
    m_queuePos = 0;
}


std::string MsMouse::getMouseInfoExt()
{
    std::ostringstream oss;
    oss << "rts=" << m_rts << " reader=" << m_reader << " unread=" << m_unread
        << " acc=" << m_accX << "," << m_accY
        << " queue=" << m_queuePos << "/" << m_queueLen;
    return oss.str();
}
