/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2024
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

#include <codecvt>
#include <locale>

#include "Globals.h"
#include "Emulation.h"
#include "Platform.h"
#include "KbdTapper.h"


using namespace std;

KbdTapper::KbdTapper()
{
    pause();
    ActiveDevice::setFrequency(1000); // 1 ms

    //typeText("D,FF\n");
}


void KbdTapper::reset()
{
    stop();
}


void KbdTapper::typeText(std::string str)
{
    if (m_typing) {
        stop();
        return;
    }

    //wstring_convert<codecvt_utf8<wchar_t>> conversion;
    wstring_convert<codecvt_utf8<wchar_t, 0x10ffff, little_endian>, wchar_t> conversion; // workaround for MinGW
    m_string = conversion.from_bytes(str);

    if (m_string.empty())
        return;

    m_pos = 0;
    m_keyPressed = false;
    m_typing = true;

    syncronize();
    resume();

    //sendNextKey();
    scheduleKey();

    g_emulation->setTemporarySpeedUpFactor(c_speedUpFactor);
}


void KbdTapper::stop()
{
    if (m_typing && m_keyPressed)
        sendNextKey(); // release button if pressed;

    m_string.clear();
    m_pos = 0;
    m_keyPressed = false;
    m_typing = false;

    pause();

    g_emulation->setTemporarySpeedUpFactor(0);
}


void KbdTapper::sendNextKey()
{
    unsigned key = m_string[m_pos];

    PalKeyCode palKey = key != 0x0A ? PK_NONE : PK_ENTER;

    m_platform->processKey(palKey, !m_keyPressed, key);
    m_keyPressed = !m_keyPressed;

    if (!m_keyPressed) {
        m_pos++;
        if (m_pos == m_string.size())
            stop();
    }
}


void KbdTapper::operate()
{
    sendNextKey();

    if (m_typing)
        scheduleKey();
}


void KbdTapper::scheduleKey()
{
    unsigned key = m_string[m_pos];

    int delay;
    if (m_keyPressed)
        delay = m_pressTime;
    else
        delay = m_releaseTime;

    if (key == 0x0A)
        delay += m_crDelay;

    m_curClock += m_kDiv * delay;
}


bool KbdTapper::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "pressTime") {
        if (values[0].isInt()) {
            m_pressTime = values[0].asInt();
            return true;
        }
    } else if (propertyName == "releaseTime") {
        if (values[0].isInt()) {
            m_releaseTime = values[0].asInt();
        return true;
        }
    } else if (propertyName == "crDelay") {
        if (values[0].isInt()) {
            m_crDelay = values[0].asInt();
        return true;
        }
    }

    return false;
}


string KbdTapper::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "pasting")
        res = m_typing ? "yes" : "no";

    return res;
}
