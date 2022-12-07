/*
 *  bashkiria-2m for Emu80 v. 4.x
 *  © Dmitry Tselikov <bashkiria-2m.narod.ru>, 2022
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

#ifndef BASHKIRIA_H
#define BASHKIRIA_H

#include "Memory.h"
#include "CrtRenderer.h"
#include "PlatformCore.h"
#include "Ppi8255Circuit.h"
#include "Pic8259.h"
#include "Pit8253.h"
#include "Pit8253Sound.h"
#include "Keyboard.h"
#include "Fdc1793.h"

class GeneralSoundSource;
class AddrSpaceMapper;


class Bashkiria_2M_Core : public PlatformCore
{
    public:
        void draw() override;
        void vrtc(bool isActive) override;
        void inte(bool isActive) override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria_2M_Core();}

    private:
        CrtRenderer* m_crtRenderer = nullptr;
        Pic8259* m_pic = nullptr;
};


class Bashkiria_2M_Renderer : public CrtRenderer, public IActive
{
    public:
        Bashkiria_2M_Renderer();
        void renderFrame() override;

        void toggleColorMode() override;
        void toggleCropping() override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        inline void attachScreenMemory(Ram* videoMemory) {if(videoMemory) m_screenMemory = videoMemory->getDataPtr();}
        inline void setScroll(uint8_t value) {m_scroll = value;}
        inline void setVideoPage(uint8_t value) {m_page = value;}
        void setPalette(int addr, uint8_t value);

        // derived from IActive
        void operate() override;

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria_2M_Renderer();}

    private:
        const uint8_t* m_screenMemory = nullptr;
        int m_line = 0, m_scrollAct = 0;
        uint8_t m_scroll = 0;
        uint8_t m_page = 0;
        bool m_colorMode = true;
        bool m_showBorder = false;
        bool m_showBorderChanged = true;
        uint32_t m_palette[2][4] = {
            {0x000000,0xB4B4B4,0x5A5A5A,0xFFFFFF},
            {0x000000,0x00FF00,0x0000FF,0xFF0000}
        };
};


class Bashkiria_2M_Palette : public AddressableDevice
{
    public:
        void writeByte(int addr, uint8_t value) override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria_2M_Palette();}

    private:
        Bashkiria_2M_Renderer* m_renderer = nullptr;
};


class Bashkiria_2M_Ppi8255Circuit1 : public Ppi8255Circuit
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        void setPortA(uint8_t value) override;
        void setPortB(uint8_t value) override;
        void setPortC(uint8_t value) override;

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria_2M_Ppi8255Circuit1();}

    private:
        // renderer for scroll setting
        Bashkiria_2M_Renderer* m_renderer = nullptr;

        uint8_t m_printerData = 0;
        bool m_printerStrobe = true;

        // address space mapper to switch memory pages
        AddrSpaceMapper* m_addrSpaceMapper = nullptr;
};


class Bashkiria_2M_Ppi8255Circuit2 : public Ppi8255Circuit
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        uint8_t getPortA() override;
        void setPortB(uint8_t value) override;
        void setPortC(uint8_t value) override;

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria_2M_Ppi8255Circuit2();}

    private:
        Fdc1793* m_fdc = nullptr;
};


class Bashkiria_2M_PitIrqWatchdog : public AddressableDevice, public IActive
{
    public:
        // derived from AddressableDevice
        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from IActive
        void operate() override;

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria_2M_PitIrqWatchdog();}

    private:
        Pit8253* m_pit = nullptr;
        Pic8259* m_pic = nullptr;
};


class Bashkiria_2M_Keyboard : public Keyboard
{
    public:
        Bashkiria_2M_Keyboard();

        void resetKeys() override;
        void processKey(EmuKey key, bool isPressed) override;

        uint8_t getMatrixData(int mask);

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria_2M_Keyboard();}

    private:
        const EmuKey m_keyMatrix[11][8] = {
            { EK_U,        EK_A,         EK_T,     EK_L,   EK_W,   EK_F6,      EK_F,     EK_F7       },
            { EK_J,        EK_Y,         EK_V,     EK_K,   EK_R,   EK_Q,       EK_B,     EK_LBRACKET },
            { EK_D,        EK_F8,        EK_E,     EK_N,   EK_C,   EK_H,       EK_Z,     EK_G        },
            { EK_RBRACKET, EK_X,         EK_O,     EK_F9,  EK_I,   EK_P,       EK_S,     EK_M        },
            { EK_8,        EK_7,         EK_6,     EK_5,   EK_4,   EK_3,       EK_2,     EK_MINUS    },
            { EK_COMMA,    EK_SEMICOLON, EK_COLON, EK_AT,  EK_1,   EK_BKSLASH, EK_0,     EK_9        },
            { EK_SPACE,    EK_TAB,       EK_BSP,   EK_DEL, EK_INS, EK_STOP,    EK_CLEAR, EK_CR       },
            { EK_SHIFT,    EK_FIX,       EK_CTRL,  EK_SEL, EK_ESC, EK_MENU,    EK_LANG,  EK_LF       },

            { EK_NP_7,   EK_NP_6,       EK_NP_5,    EK_NP_4,   EK_NP_3,  EK_NP_2,  EK_NP_1,  EK_NP_0 },
            { EK_SLASH,  EK_NP_PERIOD,  EK_CARET,   EK_PERIOD, EK_NONE,  EK_NONE,  EK_NP_9,  EK_NP_8 },
            { EK_NONE,   EK_NONE,       EK_NONE,    EK_F5,     EK_F4,    EK_F3,    EK_F2,    EK_F1   }
        };

        uint8_t m_keys[11];
};


class Bashkiria_2M_KbdLayout : public KbdLayout
{
    public:

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria_2M_KbdLayout();}

    protected:
        EmuKey translateKey(PalKeyCode keyCode) override;
        EmuKey translateUnicodeKey(unsigned unicodeKey, PalKeyCode key, bool& shift, bool& lang) override;
};


class Bashkiria_2M_KbdMem : public AddressableDevice
{
    public:
        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria_2M_KbdMem();}

    private:
        Bashkiria_2M_Keyboard* m_kbd = nullptr;
};


class Bashkiria_2M_Pit8253SoundSource : public Pit8253SoundSource
{
    public:
        int calcValue() override;

        void setGate(bool gate);

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria_2M_Pit8253SoundSource();}

    private:
};


class Bashkiria_2M_Spi8251 : public AddressableDevice
{
    public:
        void writeByte(int addr, uint8_t value) override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria_2M_Spi8251();}

    private:
        Bashkiria_2M_Pit8253SoundSource* m_snd = nullptr;
};


#endif // BASHKIRIA_H
