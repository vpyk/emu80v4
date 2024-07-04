/*
 *  bashkiria-2m for Emu80 v. 4.x
 *  © Dmitry Tselikov <bashkiria-2m.narod.ru>, 2022
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


class Bashkiria2mCore : public PlatformCore
{
    public:
        void draw() override;
        void vrtc(bool isActive) override;
        void inte(bool isActive) override;
        void timer(int id, bool isActive) override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria2mCore();}

    private:
        CrtRenderer* m_crtRenderer = nullptr;
        Pic8259* m_pic = nullptr;
        Pit8253* m_pit = nullptr;
};


class Bashkiria2mRenderer : public CrtRenderer, public IActive
{
    public:
        Bashkiria2mRenderer();
        ~Bashkiria2mRenderer();
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

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria2mRenderer();}

    private:
        uint32_t* m_frameBuf;
        const uint8_t* m_screenMemory = nullptr;
        int m_line = 0, m_scrollAct = 0;
        uint8_t m_scroll = 0;
        uint8_t m_page = 0;
        bool m_colorMode = true;
        bool m_showBorder = false;
        uint32_t m_palette[2][4] = {
            {0x000000,0xB4B4B4,0x5A5A5A,0xFFFFFF},
            {0x000000,0x00FF00,0x0000FF,0xFF0000}
        };
};


class Bashkiria2mPalette : public AddressableDevice
{
    public:
        void writeByte(int addr, uint8_t value) override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria2mPalette();}

    private:
        Bashkiria2mRenderer* m_renderer = nullptr;
};


class Bashkiria2mPpi8255Circuit1 : public Ppi8255Circuit
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        void setPortA(uint8_t value) override;
        void setPortB(uint8_t value) override;
        void setPortC(uint8_t value) override;

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria2mPpi8255Circuit1();}

    private:
        // renderer for scroll setting
        Bashkiria2mRenderer* m_renderer = nullptr;

        uint8_t m_printerData = 0;
        bool m_printerStrobe = true;

        // address space mapper to switch memory pages
        AddrSpaceMapper* m_addrSpaceMapper = nullptr;
};


class Bashkiria2mPpi8255Circuit2 : public Ppi8255Circuit
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        uint8_t getPortA() override;
        void setPortB(uint8_t value) override;
        void setPortC(uint8_t value) override;

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria2mPpi8255Circuit2();}

    private:
        Fdc1793* m_fdc = nullptr;
};


class Bashkiria2mKeyboard : public Keyboard
{
    public:
        Bashkiria2mKeyboard();

        void resetKeys() override;
        void processKey(EmuKey key, bool isPressed) override;

        uint8_t getMatrixData(int mask);

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria2mKeyboard();}

    private:
        const EmuKey m_keyMatrix[11][8] = {
            { EK_U,        EK_A,         EK_T,     EK_L,        EK_W,   EK_GRAVE,   EK_F,     EK_TILDE    },
            { EK_J,        EK_Y,         EK_V,     EK_K,        EK_R,   EK_Q,       EK_B,     EK_LBRACE   },
            { EK_D,        EK_LBRACKET,  EK_E,     EK_N,        EK_C,   EK_H,       EK_Z,     EK_G        },
            { EK_RBRACE,   EK_X,         EK_O,     EK_RBRACKET, EK_I,   EK_P,       EK_S,     EK_M        },
            { EK_8,        EK_7,         EK_6,     EK_5,        EK_4,   EK_3,       EK_2,     EK_MINUS    },
            { EK_COMMA,    EK_SEMICOLON, EK_EQU,   EK_YO,       EK_1,   EK_BKSLASH, EK_0,     EK_9        },
            { EK_SPACE,    EK_TAB,       EK_BSP,   EK_DEL,      EK_INS, EK_STOP,    EK_CLEAR, EK_CR       },
            { EK_SHIFT,    EK_FIX,       EK_CTRL,  EK_SEL,      EK_ESC, EK_GRAPH,   EK_LANG,  EK_LF       },

            { EK_SHOME,    EK_RIGHT,     EK_MENU,  EK_LEFT,     EK_END, EK_DOWN,    EK_HOME,  EK_PHOME },
            { EK_SLASH,    EK_PEND,      EK_COLON, EK_PERIOD,   EK_NONE,EK_NONE,    EK_SEND,  EK_UP },
            { EK_NONE,     EK_NONE,      EK_NONE,  EK_F5,       EK_F4,  EK_F3,      EK_F2,    EK_F1   }
        };

        uint8_t m_keys[11];
};


class Bashkiria2mKbdLayout : public KbdLayout
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria2mKbdLayout();}

    protected:
        EmuKey translateKey(PalKeyCode keyCode) override;
        EmuKey translateUnicodeKey(unsigned unicodeKey, PalKeyCode key, bool& shift, bool& lang) override;

    private:
        bool m_upAsNumpad5 = false;
};


class Bashkiria2mKbdMem : public AddressableDevice
{
    public:
        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria2mKbdMem();}

    private:
        Bashkiria2mKeyboard* m_kbd = nullptr;
};


class Bashkiria2mPit8253SoundSource : public Pit8253SoundSource
{
    public:
        int calcValue() override;
        void tuneupPit() override;

        void setGate(bool gate);

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria2mPit8253SoundSource();}

    private:
};


class Bashkiria2mSpi8251 : public AddressableDevice
{
    public:
        void writeByte(int addr, uint8_t value) override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        static EmuObject* create(const EmuValuesList&) {return new Bashkiria2mSpi8251();}

    private:
        Bashkiria2mPit8253SoundSource* m_snd = nullptr;
};


#endif // BASHKIRIA_H
