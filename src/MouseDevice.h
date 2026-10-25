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

#ifndef MOUSEDEVICE_H
#define MOUSEDEVICE_H

#include <string>

#include "EmuObjects.h"


// Base class of an emulated mouse. Handles the host and MCP side: absolute and relative (captured)
// host pointer movement, host and MCP buttons, the pointer position on the device screen, common
// properties. A derived class implements the device protocol and gets the movement in its own units.
//
// Device screen coordinates: x 0..width-1 to the right, y 0..height-1 upward.
// Buttons: bit 0 left, bit 1 right, bit 2 middle. Wheel steps: positive away from the user.
//
// Properties:
//   relative        - yes/no, pointer capture in the Qt frontend
//   scale           - device movement units per device screen point, 1..16
//   activityTimeout - ms of emulated time, only for mice using markActivity() (see below)
class MouseDevice : public EmuObject
{
    public:
        void reset() override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        // Host frontend interface.
        // Absolute move: fx/fy are frame pixels, inside = false when the pointer is outside the picture.
        // Returns true if the pointer is over the device screen
        bool hostMouseMove(int fx, int fy, bool inside, int buttons);
        // Relative move (pointer capture): frame pixels, screen direction (dy > 0 - down)
        void hostMouseMoveRelative(int dx, int dy, int buttons);
        void hostMouseWheel(int steps);
        bool isRelative() {return m_relative;}

        // MCP interface: x/y - device screen coordinates, -1 - don't change
        void emuMouseSet(int x, int y, int buttons);
        int emuMouseMaxX() {return m_width - 1;}
        int emuMouseMaxY() {return m_height - 1;}

        // The program uses the mouse: the frontend captures the pointer only while the mouse is active.
        // Default: activity tracked by markActivity()
        virtual bool isMouseActive();

        std::string getMouseInfo();

    protected:
        // Frame pixel -> device screen point (y up, may be outside the screen).
        // Returns false if the point doesn't belong to the device screen (e.g. border).
        // Default: the frame is the device screen
        virtual bool frameToMouse(int fx, int fy, int& x, int& y);

        // Movement in device units (scale applied, y up); called only while the mouse is active
        virtual void onMove(int /*dx*/, int /*dy*/) {}
        // Wheel steps; called only while the mouse is active
        virtual void onWheel(int /*steps*/) {}
        // Device specific part of getMouseInfo()
        virtual std::string getMouseInfoExt() {return "";}

        void setScreenSize(int width, int height);

        int pointerX() {return m_x;}
        int pointerY() {return m_y;}

        // Current buttons: host | MCP
        int buttons() {return m_hostButtons | m_emuButtons;}
        // Current buttons plus the ones pressed since the previous call (a short click isn't lost)
        int takeButtons();

        // Activity tracking for mice without an enable bit: the mouse is active while the program
        // accesses it, activityTimeout ms of emulated time since the last markActivity()
        void useActivityTimeout(int defaultTimeout);
        void markActivity();

        bool m_relative = false;
        int m_scale = 1;

    private:
        int m_width = 512;
        int m_height = 256;

        int m_x = 256;              // pointer on the device screen
        int m_y = 128;

        bool m_haveHost = false;    // last absolute host position is known
        int m_hostX = 0;            // in device screen coordinates, not limited by the screen
        int m_hostY = 0;

        int m_hostButtons = 0;
        int m_emuButtons = 0;       // set via MCP, held until MCP changes them
        int m_pressedSinceTake = 0;

        bool m_activityTimeoutUsed = false;
        int m_activityTimeout = 0;
        bool m_activitySeen = false;
        uint64_t m_lastActivityClock = 0;

        void setButtons(int hostButtons, int emuButtons);
        void setPointer(int x, int y);
        void reportMove(int dx, int dy);
};

#endif // MOUSEDEVICE_H
