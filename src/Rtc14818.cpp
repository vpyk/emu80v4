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

// Rtc14818.cpp
// Basic implementation of RTC MC14818A (KR512VI1)

#include <chrono>
#include <ctime>

#include "Rtc14818.h"

using namespace std;

Rtc14818::Rtc14818()
{
    m_curReg = 0;
    for (int i = 0; i < 64; i++)
        m_regs[i] = 0;
    m_regs[0x0D] = 0x80;
}


bool Rtc14818::setProperty(const std::string &propertyName, const EmuValuesList &values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "pokeCmos" && values[0].isInt() && values[1].isInt()) {
        m_regs[values[0].asInt() & 0x3F] = values[1].asInt();
        return true;
    }

    return false;
}


void Rtc14818::writeByte(int addr, uint8_t value)
{
    addr &= 1;

    if (addr == 0) {
        // reg number
        m_curReg = value & 0x3F;
        return;
    }

    // register
    if (m_curReg == 0x0A)
        value &= 0x7F;
    else if (m_curReg == 0x0C)
        value = 0;
    else if (m_curReg == 0x0D)
        value = 0x80;

    if (m_curReg >= 0x0A)
        m_regs[m_curReg] = value;
}


uint8_t Rtc14818::readByte(int addr)
{
    if (addr == 0)
        return 0xFF;

    if (m_curReg < 0x0E)
        setTime();

    return m_regs[m_curReg];
}


void Rtc14818::setTime()
{
    auto now = std::chrono::system_clock::now();
    time_t cNow = std::chrono::system_clock::to_time_t(now);
    tm* localTime = std::localtime(&cNow);

    int year = localTime->tm_year % 100;
    int month = localTime->tm_mon + 1;
    int day = localTime->tm_mday;
    int dayOfWeek = (localTime->tm_wday + 6) % 7 + 1;
    int hour = localTime->tm_hour;
    int minute = localTime->tm_min;
    int second = localTime->tm_sec;

    bool bcd = !(m_regs[0xB] & 4);

    bool ampm = false;
    if (!(m_regs[0xB] & 2)) {
        ampm = hour > 12;
        hour %= 12;
    }

    m_regs[0] = bcd ? toBCD(second) : second;
    m_regs[2] = bcd ? toBCD(minute) : minute;
    m_regs[4] = bcd ? toBCD(hour) : hour;
    if (ampm)
        m_regs[4] |= 0x80;
    m_regs[6] = dayOfWeek;

    m_regs[7] = bcd ? toBCD(day) : day;
    m_regs[8] = bcd ? toBCD(month) : month;
    m_regs[9] = bcd ? toBCD(year) : year;
}


uint8_t Rtc14818::toBCD(int value) const
{
    return ((value / 10) << 4) | (value % 10);
}
