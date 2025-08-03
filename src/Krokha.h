/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2025
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

#ifndef KROKHA_H
#define KROKHA_H

#include "CrtRenderer.h"
#include "PlatformCore.h"
#include "Keyboard.h"


class KrokhaCore : public PlatformCore
{
public:
    bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

    void draw() override;
    void vrtc(bool isActive) override;
    void inte(bool isActive) override;

    static EmuObject* create(const EmuValuesList&) {return new KrokhaCore();}

private:
    CrtRenderer* m_crtRenderer = nullptr;

    bool m_intReq = false;
};


class KrokhaRenderer : public CrtRenderer, public IActive
{
    public:
        KrokhaRenderer();
        ~KrokhaRenderer();
        void renderFrame() override;

        void toggleColorMode() override;
        void toggleCropping() override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        // derived from ActiveDevice
        void operate() override;

        static EmuObject* create(const EmuValuesList&) {return new KrokhaRenderer();}

    private:
        const uint32_t c_krokhaColorPalette[8] = {
            0xBFBFBF, 0x00FF00, 0xFF0000, 0xFFFF00, 0x0000FF, 0x00FFFF, 0xFF00FF, 0xFFFFFF
        };

        const uint8_t* m_screenMemory = nullptr;
        const uint8_t* m_font = nullptr;

        int m_fontSize = 0;

        uint32_t* m_frameBuf = nullptr;

        bool m_showBorder = false;
        bool m_colorMode = false;

        int m_curLine = 0;

        int m_offsetX = 0;
        int m_offsetY = 0;

        void renderLine(int nLine);
};


class KrokhaJoystick : public Keyboard
{
    public:
        KrokhaJoystick();

        void initConnections() override;

        void resetKeys() override;
        void processKey(EmuKey key, bool isPressed) override;

        static EmuObject* create(const EmuValuesList&) {return new KrokhaJoystick();}

    private:
        uint8_t m_keys;

        EmuOutput* m_keysOutput = nullptr;
};


#endif // KROKHA_H
