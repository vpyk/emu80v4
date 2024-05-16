/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2021-2024
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

#ifndef KORVET_H
#define KORVET_H

#include "PlatformCore.h"
#include "CrtRenderer.h"
#include "Ppi8255Circuit.h"
#include "Keyboard.h"
#include "Pit8253Sound.h"
#include "CpuWaits.h"


class Covox;
class Fdc1793;
class Pic8259;


class KorvetAddrSpace : public AddressableDevice
{
    public:
        KorvetAddrSpace(std::string fileName);
        virtual ~KorvetAddrSpace();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        void reset()  override {m_memCfg = 0;}

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        void setPage(int pageNum, AddressableDevice* page);
        void setMemCfg(uint8_t cfg) {m_memCfg = cfg;}

        static EmuObject* create(const EmuValuesList& parameters) {return new KorvetAddrSpace(parameters[0].asString());}

    private:
        uint8_t* m_memMap = nullptr;
        AddressableDevice* m_pages[9];
        int m_memCfg = 0;
};


class KorvetAddrSpaceSelector : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void attachKorvetAddrSpace(KorvetAddrSpace* addrSpace) {m_korvetAddrSpace = addrSpace;}

        void writeByte(int, uint8_t value) override;
        uint8_t readByte(int)  override {return 0xff;}

        static EmuObject* create(const EmuValuesList&) {return new KorvetAddrSpaceSelector();}

    private:
        KorvetAddrSpace* m_korvetAddrSpace = nullptr;
};


class KorvetPpi8255Circuit;

class KorvetCore : public PlatformCore
{
    public:
        //KorvetCore();
        //virtual ~KorvetCore();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void draw() override;
        void vrtc(bool isActive) override;
        void hrtc(bool, int) override;
        void inte(bool isActive) override;
        void timer(int id, bool isActive) override;

        void int4(bool isActive);
        void int7();

        void attachCrtRenderer(CrtRenderer* crtRenderer);

        static EmuObject* create(const EmuValuesList&) {return new KorvetCore();}

    private:
        CrtRenderer* m_crtRenderer = nullptr;
        KorvetPpi8255Circuit* m_ppiCircuit = nullptr;
        Pit8253* m_pit = nullptr;
        Pic8259* m_pic = nullptr;
        bool m_curVrtc = true;
        bool m_curHrtc = false;
};


class KorvetRenderer;

class KorvetGraphicsAdapter : public AddressableDevice
{
    public:
        KorvetGraphicsAdapter();
        virtual ~KorvetGraphicsAdapter();

        void writeByte(int, uint8_t value) override;
        uint8_t readByte(int addr) override;
        //void reset() override;

        inline void setColorRegisterValue(uint8_t value) {m_colorRegisterValue = value;}
        inline void setRwPage(int page) {m_rwPage = page;}

        static EmuObject* create(const EmuValuesList&) {return new KorvetGraphicsAdapter();}

        friend KorvetRenderer;

    private:
        uint8_t m_colorRegisterValue = 0;
        uint8_t m_rwPage = 0;
        uint8_t* m_planes[3];
};


class KorvetTextAdapter : public AddressableDevice
{
    public:
        KorvetTextAdapter();
        virtual ~KorvetTextAdapter();

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;
        //void reset() override;

        void setAttrMask(int attrMask) {m_attrMask = attrMask;}
        inline bool getAttr() {return m_curAttr;}

        static EmuObject* create(const EmuValuesList&) {return new KorvetTextAdapter();}

        friend KorvetRenderer;

    private:
        int m_attrMask = 0;
        uint8_t m_curAttr = 0;
        uint8_t* m_symbols;
        uint8_t* m_attrs; // 0xFF if inverted else 0
};


class KorvetRenderer : public CrtRenderer, public IActive
{
    public:
        KorvetRenderer();
        virtual ~KorvetRenderer();

        void renderFrame() override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;
        std::string getDebugInfo() override;

        const char* getTextScreen() override;

        // derived from CrtRenderer
        void toggleCropping() override;
        void toggleColorMode() override;

        // derived from ActiveDevice
        void operate() override;

        void setFontFile(std::string fontFileName);

        void setDisplayPage(int page);
        inline void setLutValue(int reg, int value) {m_lut[reg] = value;}
        void setFont(int fontNo) {m_fontNo = fontNo;}
        void setWideChr(bool wideChr) {m_wideChr = wideChr;}

        void attachGraphicsAdapter(KorvetGraphicsAdapter* adapter) {m_graphicsAdapter = adapter;}
        void attachTextAdapter(KorvetTextAdapter* adapter) {m_textAdapter = adapter;}

        static EmuObject* create(const EmuValuesList&) {return new KorvetRenderer();}

    private:
        const uint32_t c_korvetColorPalette[16] = {
            0x000000, 0x0000C0, 0x00C000, 0x00C0C0,
            0xC00000, 0xC000C0, 0xC0C000, 0xC0C0C0,
            0x404040, 0x4040FF, 0x40FF40, 0x40FFFF,
            0xFF4040, 0xFF40FF, 0xFFFF40, 0xFFFFFF
        };

        const uint32_t c_korvetBwPalette[16] = {
            0x000000, 0x111111, 0x222222, 0x333333,
            0x424242, 0x545454, 0x656565, 0x767676,
            0x898989, 0x9A9A9A, 0xABABAB, 0xBDBDBD,
            0xCCCCCC, 0xDDDDDD, 0xEEEEEE, 0xFFFFFF
        };

        const wchar_t* c_korvetSymbols =
            L" ☺☻█████◘█PpLl♫☼►◄↕‼¶§■↨↑↓→←    "
            L" !\"#$%&'()*+,-./0123456789:;<=>?"
            L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
            L"`abcdefghijklmnopqrstuvwxyz{|}~"
            L"································"
            L"································"
            L"юабцдефгхийклмнопярстужвьызшэщчъ"
            L"ЮАБЦДЕФГХИЙКЛМНОПЯРСТУЖВЬЫЗШЭЩЧЪ"
            L" ☺☻█████◘█PpLl♫☼►◄↕‼¶§■↨↑↓→←    "
            L" !\"#$%&'()*+,-./0123456789:;<=>?"
            L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
            L"`abcdefghijklmnopqrstuvwxyz{|}~"
            L"░▒▓│┤╡╢╖╕╣║╗╝╜╛┐"
            L"└┴┬├─┼╞╟╚╔╩╦╠═╬╧"
            L"╨Ё╥╙╘╒╓╫╪┘┌█▄▌▐▀"
            L"АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"
            L"абвгдежзийклмнопрстуфхцчшщъыьэюя"
            L"≡ё≥≤⌠⌡÷≈°•·√ⁿ²■ ";

        uint8_t* m_font = nullptr;
        int m_fontNo = 0;

        KorvetGraphicsAdapter* m_graphicsAdapter = nullptr;
        KorvetTextAdapter* m_textAdapter = nullptr;
        int m_displayPage = 0;
        bool m_wideChr = false;

        int m_lut[16];
        const uint32_t* m_palette= c_korvetColorPalette;

        bool m_showBorder = false;
        bool m_colorMode = true;

        int m_curLine = 0;
        uint32_t* m_frameBuf;

        void setColorMode(bool colorMode);

        void prepareFrame();
        void renderLine(int nLine);
};


class KorvetFddMotor : public ActiveDevice
{
public:
    KorvetFddMotor();

    void operate() override;
    bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

    void on();

    static EmuObject* create(const EmuValuesList&) {return new KorvetFddMotor();}

private:
    KorvetCore* m_core;
};


class KorvetPpi8255Circuit : public Ppi8255Circuit
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        uint8_t getPortA() override;
        void setPortB(uint8_t value) override;
        void setPortC(uint8_t value) override;

        void setVbl(bool vbl) {m_vbl = vbl;}

        static EmuObject* create(const EmuValuesList&) {return new KorvetPpi8255Circuit();}

    private:
        KorvetRenderer* m_crtRenderer = nullptr;
        KorvetGraphicsAdapter* m_graphicsAdapter = nullptr;
        KorvetTextAdapter* m_textAdapter = nullptr;
        Fdc1793* m_fdc;
        KorvetFddMotor* m_motor;
        bool m_motorBit = false;
        int m_addr = 0;
        bool m_vbl = true;
};


class KorvetPit8253SoundSource;

class KorvetPpi8255Circuit2 : public Ppi8255Circuit
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        void setPortA(uint8_t value) override;
        void setPortC(uint8_t value) override;

        static EmuObject* create(const EmuValuesList&) {return new KorvetPpi8255Circuit2();}

    private:
        Covox* m_covox = nullptr;
        KorvetPit8253SoundSource* m_pitSoundSource = nullptr;
        uint8_t m_printerData = 0;
        bool m_printerStrobe = false;
};


class KorvetColorRegister : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int) override {return 0xff;}

        static EmuObject* create(const EmuValuesList&) {return new KorvetColorRegister();}

    private:
        KorvetGraphicsAdapter* m_graphicsAdapter = nullptr;
};


class KorvetLutRegister : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int) override {return 0xff;}

        static EmuObject* create(const EmuValuesList&) {return new KorvetLutRegister();}

    private:
        KorvetRenderer* m_crtRenderer = nullptr;
};


class KorvetKeyboard : public Keyboard
{
    public:
        KorvetKeyboard();

        void resetKeys() override;
        void processKey(EmuKey key, bool isPressed) override;

        void setMatrix1Mask(uint8_t mask) {m_mask1 = mask;}
        void setMatrix2Mask(uint8_t mask) {m_mask2 = mask;}
        uint8_t getMatrix1Data();
        uint8_t getMatrix2Data();

        static EmuObject* create(const EmuValuesList&) {return new KorvetKeyboard();}

    private:
        const EmuKey m_keyMatrix1[8][8] = {
            { EK_AT,    EK_A,     EK_B,     EK_C,         EK_D,       EK_E,        EK_F,      EK_G      },
            { EK_H,     EK_I,     EK_J,     EK_K,         EK_L,       EK_M,        EK_N,      EK_O      },
            { EK_P,     EK_Q,     EK_R,     EK_S,         EK_T,       EK_U,        EK_V,      EK_W      },
            { EK_X,     EK_Y,     EK_Z,     EK_LBRACKET,  EK_BKSLASH, EK_RBRACKET, EK_CARET,  EK_UNDSCR },
            { EK_0,     EK_1,     EK_2,     EK_3,         EK_4,       EK_5,        EK_6,      EK_7      },
            { EK_8,     EK_9,     EK_COLON, EK_SEMICOLON, EK_COMMA,   EK_MINUS,    EK_PERIOD, EK_SLASH  },
            { EK_CR,    EK_CLEAR, EK_STOP,  EK_INS,       EK_DEL,     EK_BSP,      EK_TAB,    EK_SPACE  },
            { EK_SHIFT, EK_LANG,  EK_GRAPH, EK_ESC,       EK_SEL,     EK_CTRL,     EK_FIX,    EK_SHIFT  }
        };

        const EmuKey m_keyMatrix2[3][8] = {
            { EK_PHOME, EK_SHOME, EK_DOWN, EK_SEND, EK_LEFT, EK_MENU, EK_RIGHT, EK_HOME },
            { EK_UP,    EK_END,   EK_NONE, EK_NONE, EK_NONE, EK_NONE, EK_PEND,  EK_NONE },
            { EK_F1,    EK_F2,    EK_F3,   EK_F4,   EK_F5,   EK_NONE, EK_NONE,  EK_NONE }
        };

        uint8_t m_keys1[8];
        uint8_t m_keys2[3];
        uint8_t m_mask1;
        uint8_t m_mask2;
};


class KorvetKbdLayout : public KbdLayout
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        static EmuObject* create(const EmuValuesList&) {return new KorvetKbdLayout();}

    protected:
        EmuKey translateKey(PalKeyCode keyCode) override;
        EmuKey translateUnicodeKey(unsigned unicodeKey, PalKeyCode key, bool& shift, bool& lang) override;
        bool processSpecialKeys(PalKeyCode keyCode) override;

    private:
        bool m_downAsNumpad5 = false;
};


class KorvetKeyboardRegisters : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void writeByte(int, uint8_t) override {}
        uint8_t readByte(int addr) override;

        static EmuObject* create(const EmuValuesList&) {return new KorvetKeyboardRegisters();}

    private:
        KorvetKeyboard* m_keyboard = nullptr;
};


class KorvetPit8253SoundSource : public Pit8253SoundSource
{
    public:
        int calcValue() override;
        void tuneupPit() override;

        void setGate(bool gate);

        static EmuObject* create(const EmuValuesList&) {return new KorvetPit8253SoundSource();}

    private:
        bool m_gate = false;
        int m_sumValue;

        void updateStats();
};


class KorvetCpuCycleWaits : public CpuCycleWaits
{
public:
    int getCpuCycleWaitStates(int memTag, bool write) override;

    static EmuObject* create(const EmuValuesList&) {return new KorvetCpuCycleWaits();}
};


class Psg3910;

class KorvetPpiPsgAdapter : public Ppi8255Circuit
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        uint8_t getPortA() override;
        void setPortA(uint8_t value) override;
        void setPortB(uint8_t value) override;

        static EmuObject* create(const EmuValuesList&) {return new KorvetPpiPsgAdapter();}

    private:
        Psg3910* m_psg = nullptr;
        bool m_strobe = false;
        uint8_t m_read = 0;
        uint8_t m_write = 0;
};


#endif // KORVET_H
