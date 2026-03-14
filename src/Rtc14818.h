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

// Rtc14818.h

#ifndef RTC14818_H
#define RTC14818_H

#include <EmuObjects.h>

class Rtc14818 : public AddressableDevice
{
public:
    Rtc14818();

    bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

    void writeByte(int addr, uint8_t value) override;
    uint8_t readByte(int addr)  override;

    static EmuObject* create(const EmuValuesList&) {return new Rtc14818();}

private:
    int m_curReg = 0;
    uint8_t m_regs[64];

    void setTime();
    uint8_t toBCD(int value) const;
};


#endif // RTC14818_H
