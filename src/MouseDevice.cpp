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

#include <sstream>
#include <algorithm>

#include "Globals.h"
#include "Emulation.h"
#include "MouseDevice.h"

using namespace std;


void MouseDevice::reset()
{
    m_x = m_width / 2;
    m_y = m_height / 2;
    m_haveHost = false;
    m_hostButtons = m_emuButtons = 0;
    m_pressedSinceTake = 0;
    m_activitySeen = false;
}


bool MouseDevice::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "relative") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_relative = values[0].asString() == "yes";
            return true;
        }
    } else if (propertyName == "scale") {
        if (values[0].isInt() && values[0].asInt() >= 1 && values[0].asInt() <= 16) {
            m_scale = int(values[0].asInt());
            return true;
        }
    } else if (propertyName == "activityTimeout" && m_activityTimeoutUsed) {
        if (values[0].isInt() && values[0].asInt() >= 0) {
            m_activityTimeout = int(values[0].asInt());
            return true;
        }
    }

    return false;
}


string MouseDevice::getPropertyStringValue(const string& propertyName)
{
    string res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "relative")
        return m_relative ? "yes" : "no";
    else if (propertyName == "scale")
        return to_string(m_scale);
    else if (propertyName == "activityTimeout" && m_activityTimeoutUsed)
        return to_string(m_activityTimeout);

    return "";
}


void MouseDevice::setScreenSize(int width, int height)
{
    m_width = width;
    m_height = height;
    m_x = width / 2;
    m_y = height / 2;
}


bool MouseDevice::frameToMouse(int fx, int fy, int& x, int& y)
{
    x = fx;
    y = m_height - 1 - fy;
    return true;
}


bool MouseDevice::hostMouseMove(int fx, int fy, bool inside, int buttons)
{
    setButtons(buttons, m_emuButtons);

    int x, y;
    if (!inside || !frameToMouse(fx, fy, x, y)) {
        // no jump when the pointer comes back from another place
        m_haveHost = false;
        return false;
    }

    // the movement is taken from the host position, which may be outside the device screen (e.g. border)
    if (m_haveHost)
        reportMove(x - m_hostX, y - m_hostY);
    m_hostX = x;
    m_hostY = y;
    m_haveHost = true;

    setPointer(x, y);
    return true;
}


void MouseDevice::hostMouseMoveRelative(int dx, int dy, int buttons)
{
    setButtons(buttons, m_emuButtons);

    // the host pointer position is unknown now: the next absolute move only syncs it
    m_haveHost = false;

    // there's no host pointer to follow, so scale works as the mouse sensitivity for the pointer too
    reportMove(dx, -dy);
    setPointer(m_x + dx * m_scale, m_y - dy * m_scale);
}


void MouseDevice::hostMouseWheel(int steps)
{
    if (isMouseActive())
        onWheel(steps);
}


void MouseDevice::emuMouseSet(int x, int y, int buttons)
{
    if (buttons >= 0)
        setButtons(m_hostButtons, buttons);

    if (x < 0 || y < 0)
        return;

    // MCP checks the range, this is just a safeguard
    x = min(x, m_width - 1);
    y = min(y, m_height - 1);

    reportMove(x - m_x, y - m_y);
    setPointer(x, y);
}


void MouseDevice::setPointer(int x, int y)
{
    m_x = max(0, min(x, m_width - 1));
    m_y = max(0, min(y, m_height - 1));
}


void MouseDevice::reportMove(int dx, int dy)
{
    if ((dx || dy) && isMouseActive())
        onMove(dx * m_scale, dy * m_scale);
}


// Presses are tracked on the union of host and MCP buttons
void MouseDevice::setButtons(int hostButtons, int emuButtons)
{
    int prev = buttons();
    m_hostButtons = hostButtons & 7;
    m_emuButtons = emuButtons & 7;
    m_pressedSinceTake |= buttons() & ~prev;
}


int MouseDevice::takeButtons()
{
    int res = buttons() | m_pressedSinceTake;
    m_pressedSinceTake = 0;
    return res;
}


void MouseDevice::useActivityTimeout(int defaultTimeout)
{
    m_activityTimeoutUsed = true;
    m_activityTimeout = defaultTimeout;
}


void MouseDevice::markActivity()
{
    m_lastActivityClock = g_emulation->getCurClock();
    m_activitySeen = true;
}


bool MouseDevice::isMouseActive()
{
    if (!m_activitySeen)
        return false;
    if (m_activityTimeout == 0)
        return true;
    uint64_t timeout = uint64_t(g_emulation->getFrequency()) * m_activityTimeout / 1000;
    return g_emulation->getCurClock() - m_lastActivityClock < timeout;
}


string MouseDevice::getMouseInfo()
{
    ostringstream oss;
    oss << "active=" << isMouseActive();
    if (m_activityTimeoutUsed && m_activitySeen)
        oss << " idle_ms=" << (g_emulation->getCurClock() - m_lastActivityClock) * 1000 / g_emulation->getFrequency();
    oss << " relative=" << m_relative << " pointer=" << m_x << "," << m_y
        << " buttons=" << buttons() << " mcp_buttons=" << m_emuButtons;
    string ext = getMouseInfoExt();
    if (!ext.empty())
        oss << " " << ext;
    return oss.str();
}
