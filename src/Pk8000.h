/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2018-2021
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

#ifndef PK8000_H
#define PK8000_H

#include "PlatformCore.h"
#include "Ppi8255Circuit.h"
#include "CrtRenderer.h"
#include "FileLoader.h"
#include "Keyboard.h"
#include "CpuWaits.h"

class AddrSpaceMapper;
class Ram;
class Fdc1793;
class GeneralSoundSource;
class Pk8000CpuWaits;


class Pk8000Renderer : public CrtRenderer, public IActive
{
    public:
        Pk8000Renderer();
        ~Pk8000Renderer();

        void renderFrame() override;

        // derived from EmuObject
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        // derived from CrtRenderer
        void toggleColorMode() override;
        void toggleCropping() override;

        // derived from ActiveDevice
        void operate() override;

        void attachScreenMemoryBank(int bankN, Ram* screenMemoryBank);
        void setScreenBank(unsigned bank);
        void setMode(unsigned mode);
        void advance();
        void setFgBgColors(unsigned fgColor, unsigned bgColor);
        void setTextBufferBase(uint16_t base) {m_txtBase = base;}
        void setSymGenBufferBase(uint16_t base);
        void setGraphicsBufferBase(uint16_t base) {m_grBase = base;}
        void setColorBufferBase(uint16_t base) {m_colBase = base;}
        void setBlanking(bool blanking);
        void setColorReg(unsigned addr, uint8_t value);
        uint8_t getColorReg(unsigned addr);
        bool isBorderWide() {return m_wideBorder;}

        static EmuObject* create(const EmuValuesList&) {return new Pk8000Renderer();}

    private:
        const uint32_t c_pk8000ColorPalette[16] = {
            0x000000, 0x000000, 0x00A800, 0x00FF00,
            0x0000A8, 0x0000FF, 0x00A8A8, 0x00FFFF,
            0xA80000, 0xFF0000, 0xA8A800, 0xFFFF00,
            0xA800A8, 0xFF00FF, 0xA8A8A8, 0xFFFFFF
        };

        const uint32_t c_pk8000BwPalette[16] = {
            0x000000, 0x222222, 0x444444, 0x656565,
            0x878787, 0xA9A9A9, 0xCBCBCB, 0xEDEDED,
            0x000000, 0x242424, 0x494949, 0x6D6D6D,
            0x929292, 0xB6B6B6, 0xDBDBDB, 0xFFFFFF
        };

        enum BlankingState {
            BS_NORMAL,
            BS_BLANK,
            BS_WRITE
        };

        const uint8_t* m_screenMemoryBanks[4];
        Ram* m_screenMemoryRamBanks[4];
        Pk8000CpuWaits* m_waits = nullptr;
        unsigned m_bank = 0;
        unsigned m_mode = 0;
        bool m_wideBorder = true;
        uint16_t m_txtBase = 0;
        uint16_t m_sgBase = 0;
        uint16_t m_grBase = 0;
        uint16_t m_colBase = 0;
        uint32_t m_fgColor = 0xC0C0C0;
        uint32_t m_bgColor = 0x000000;
        uint8_t m_colorRegs[32];
        bool m_showBorder = false;
        bool m_colorMode = true;
        bool m_blanking = false;
        bool m_activeArea = false;
        unsigned m_ticksPerPixel;
        uint16_t m_nextLineSgBase = 0;
        const uint32_t* m_palette = c_pk8000ColorPalette;

        uint64_t m_ticksPerScanLineActiveArea;   // тактов на активную часть скан-линии
        uint64_t m_ticksPerScanLineSideBorder;   // тактов на боковой бордюр скан-линии
        int m_pixelsPerOutInstruction = 30;

        int m_curLine = 0;
        int m_offsetX = 0;
        int m_offsetY = 0;
        uint32_t* m_frameBuf;
        void prepareFrame();
        void renderLine(int nLine);

        void setColorMode(bool colorMode);

        uint64_t m_curScanlineClock;
        int m_curScanlinePixel = 0;
        int m_curBlankingPixel = 0;
        uint32_t m_bgScanlinePixels[320];
        uint32_t m_fgScanlinePixels[320];
        BlankingState m_blankingPixels[320];
};


// Ports A0h-BFh
class Pk8000Mode1ColorMem : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        inline void attachCrtRenderer(Pk8000Renderer* renderer) {m_renderer = renderer;}

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int) override;

        static EmuObject* create(const EmuValuesList&) {return new Pk8000Mode1ColorMem();}

    private:
        Pk8000Renderer* m_renderer = nullptr;
};


// Port 88h
class Pk8000ColorSelector : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        inline void attachCrtRenderer(Pk8000Renderer* renderer) {m_renderer = renderer;}

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int) override {return m_value;}

        static EmuObject* create(const EmuValuesList&) {return new Pk8000ColorSelector();}

    private:
        Pk8000Renderer* m_renderer = nullptr;
        uint8_t m_value = 0;
};


// Port 90h
class Pk8000TxtBufSelector : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        inline void attachCrtRenderer(Pk8000Renderer* renderer) {m_renderer = renderer;}

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int) override {return m_value;}

        static EmuObject* create(const EmuValuesList&) {return new Pk8000TxtBufSelector();}

    private:
        Pk8000Renderer* m_renderer = nullptr;
        uint8_t m_value = 0;
};


// Port 91h
class Pk8000SymGenBufSelector : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        inline void attachCrtRenderer(Pk8000Renderer* renderer) {m_renderer = renderer;}

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int) override {return m_value;}

        static EmuObject* create(const EmuValuesList&) {return new Pk8000SymGenBufSelector();}

    private:
        Pk8000Renderer* m_renderer = nullptr;
        uint8_t m_value = 0;
};


// Port 92h
class Pk8000GrBufSelector : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        inline void attachCrtRenderer(Pk8000Renderer* renderer) {m_renderer = renderer;}

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int) override {return m_value;}

        static EmuObject* create(const EmuValuesList&) {return new Pk8000GrBufSelector();}

    private:
        Pk8000Renderer* m_renderer = nullptr;
        uint8_t m_value = 0;
};


// Port 93h
class Pk8000ColBufSelector : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        inline void attachCrtRenderer(Pk8000Renderer* renderer) {m_renderer = renderer;}

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int) override {return m_value;}

        static EmuObject* create(const EmuValuesList&) {return new Pk8000ColBufSelector();}

    private:
        Pk8000Renderer* m_renderer = nullptr;
        uint8_t m_value = 0;
};


class Pk8000Core : public PlatformCore
{
    public:
        Pk8000Core();
        virtual ~Pk8000Core();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void draw() override;
        void reset() override;
        void vrtc(bool isActive) override;
        void inte(bool isActive) override;

        void attachCrtRenderer(Pk8000Renderer* crtRenderer);

        static EmuObject* create(const EmuValuesList&) {return new Pk8000Core();}
    private:
        Pk8000Renderer* m_crtRenderer = nullptr;
        bool m_intReq = false;
};


class Pk8000FileLoader : public FileLoader
{
public:
    Pk8000FileLoader() {m_multiblockAvailable = true;}
    bool loadFile(const std::string& fileName, bool run = false) override;

    static EmuObject* create(const EmuValuesList&) {return new Pk8000FileLoader();}
};


class Pk8000Keyboard : public Keyboard
{
    public:
        Pk8000Keyboard();

        void resetKeys() override;
        void processKey(EmuKey key, bool isPressed) override;

        void setMatrixRowNo(uint8_t row);
        uint8_t getMatrixRowState();
        uint8_t getJoystickState() {return m_joystickKeys;}

        static EmuObject* create(const EmuValuesList&) {return new Pk8000Keyboard();}

    private:

        const EmuKey m_keyMatrix[10][8] = {
            { EK_0,        EK_1,       EK_2,        EK_3,     EK_4,      EK_5,        EK_6,         EK_7     },
            { EK_8,        EK_9,       EK_COMMA,    EK_MINUS, EK_PERIOD, EK_COLON,    EK_SEMICOLON, EK_SLASH },
            { EK_LBRACKET, EK_BKSLASH, EK_RBRACKET, EK_CARET, EK_UNDSCR, EK_AT,       EK_A,         EK_B     },
            { EK_C,        EK_D,       EK_E,        EK_F,     EK_G,      EK_H,        EK_I,         EK_J     },
            { EK_K,        EK_L,       EK_M,        EK_N,     EK_O,      EK_P,        EK_Q,         EK_R     },
            { EK_S,        EK_T,       EK_U,        EK_V,     EK_W,      EK_X,        EK_Y,         EK_Z     },
            { EK_SHIFT,    EK_CTRL,    EK_GRAPH,    EK_LANG,  EK_FIX,    EK_F1,       EK_F2,        EK_F3    },
            { EK_F4,       EK_F5,      EK_ESC,      EK_TAB,   EK_STOP,   EK_BSP,      EK_SEL,       EK_CR    },
            { EK_SPACE,    EK_CLEAR,   EK_INS,      EK_DEL,   EK_LEFT,   EK_UP,       EK_DOWN,      EK_RIGHT },
            { EK_SHOME,    EK_SEND,    EK_MENU,     EK_HOME,  EK_END,    EK_PEND,     EK_PHOME,     EK_SPACE }
        };

        uint8_t m_keys[10];
        uint8_t m_rowNo;

        uint8_t m_joystickKeys = 0;
};


class Pk8000Ppi8255Circuit1 : public Ppi8255Circuit
{
    public:
        Pk8000Ppi8255Circuit1();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        uint8_t getPortB() override; // port 81h
        void setPortA(uint8_t value) override; // port 80h
        void setPortC(uint8_t value) override; //port 82h

        /*virtual */void attachKeyboard(Pk8000Keyboard* kbd) {m_kbd = kbd;}
        void attachAddrSpaceMapper(int bank, AddrSpaceMapper* addrSpaceMapper);

        static EmuObject* create(const EmuValuesList&) {return new Pk8000Ppi8255Circuit1();}

    protected:
        // Источник звука - вывод на магнитофон
        GeneralSoundSource* m_tapeSoundSource;

        // Источник звука - встроенный резонатор
        GeneralSoundSource* m_beepSoundSource;

        Pk8000Keyboard* m_kbd = nullptr;
        AddrSpaceMapper* m_addrSpaceMappers[4];
};


class Pk8000Ppi8255Circuit2 : public Ppi8255Circuit
{
    public:
        //Pk8000Ppi8255Circuit2() {}
        //virtual ~Pk8000Ppi8255Circuit2();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void attachCrtRenderer(Pk8000Renderer* renderer) {m_renderer = renderer;}

        // derived from Ppi8255Circuit
        void setPortA(uint8_t value) override; // port 84h
        void setPortB(uint8_t value) override; // port 85h
        void setPortC(uint8_t value) override; // port 86h
        uint8_t getPortC() override; // port 86h

        void attachAddrSpaceMapper(int bank, AddrSpaceMapper* addrSpaceMapper);

        static EmuObject* create(const EmuValuesList&) {return new Pk8000Ppi8255Circuit2();}

    protected:
        Pk8000Renderer* m_renderer = nullptr;

    private:
        uint8_t m_printerData = 0;
        bool m_printerStrobe = true;
};


// Port 8Dh
class Pk8000InputRegister : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        uint8_t readByte(int) override;
        void writeByte(int, uint8_t) override {}

        void attachKeyboard(Pk8000Keyboard* kbd) {m_kbd = kbd;}

        static EmuObject* create(const EmuValuesList&) {return new Pk8000InputRegister();}

    private:
        Pk8000Keyboard* m_kbd = nullptr;
};


class Pk8000KbdLayout : public KbdLayout
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        static EmuObject* create(const EmuValuesList&) {return new Pk8000KbdLayout();}

    protected:
        EmuKey translateKey(PalKeyCode keyCode) override;
        EmuKey translateUnicodeKey(unsigned unicodeKey, PalKeyCode key, bool& shift, bool& lang) override;
};


class Pk8000FddControlRegister : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        inline void attachFdc1793(Fdc1793* fdc) {m_fdc = fdc;}

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int)  override {return 0xff;}

        static EmuObject* create(const EmuValuesList&) {return new Pk8000FddControlRegister();}

    private:
        Fdc1793* m_fdc = nullptr;
};


class Pk8000FdcStatusRegisters : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        inline void attachFdc1793(Fdc1793* fdc) {m_fdc = fdc;}

        void writeByte(int addr, uint8_t value)  override {m_bytes[addr & 0x03] = value;}
        uint8_t readByte(int addr) override;

        static EmuObject* create(const EmuValuesList&) {return new Pk8000FdcStatusRegisters();}

    private:
        Fdc1793* m_fdc = nullptr;
        uint8_t m_bytes[4];
};


class Pk8000CpuWaits : public CpuWaits
{
public:
    int getCpuWaitStates(int memTag, int opcode, int normalClocks) override;
    inline void setState(bool scr03activeArea) {m_scr03activeArea = scr03activeArea;}

    static EmuObject* create(const EmuValuesList&) {return new Pk8000CpuWaits();}

private:
    bool m_scr03activeArea = false;
};

#endif // PK8000_H
