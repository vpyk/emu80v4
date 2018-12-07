/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2018
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

class AddrSpaceMapper;
class Ram;
class GeneralSoundSource;


class Pk8000Renderer : public CrtRenderer
{
    public:
        Pk8000Renderer();
        void renderFrame() override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        //std::string getPropertyStringValue(const std::string& propertyName) override;
        //void toggleColorMode()  override {m_isColorMode = !m_isColorMode;}

        void attachScreenMemoryBank(int bankN, Ram* screenMemoryBank);
        void setScreenBank(unsigned bank);
        void setMode(unsigned mode);
        void setFgColor(unsigned color) {m_fgColor = c_pk8000ColorPalette[color];}
        void setBgColor(unsigned color) {m_bgColor = c_pk8000ColorPalette[color];}
        void setTextBufferBase(uint16_t base) {m_txtBase = base;}
        void setSymGenBufferBase(uint16_t base) {m_sgBase = base;}
        void setGraphicsBufferBase(uint16_t base) {m_grBase = base;}
        void setColorBufferBase(uint16_t base) {m_colBase = base;}

        void setColorReg(unsigned addr, uint8_t value);
        uint8_t getColorReg(unsigned addr);

        static EmuObject* create(const EmuValuesList&) {return new Pk8000Renderer();}

    private:
        const uint32_t c_pk8000ColorPalette[16] = {
            0x000000, 0x000000, 0x00C000, 0x00FF00,
            0x0000C0, 0x0000FF, 0x00C0C0, 0x00FFFF,
            0xC00000, 0xFF0000, 0xC0C000, 0xFFFF00,
            0xC000C0, 0xFF00FF, 0xC0C0C0, 0xFFFFFF
        };

        const uint8_t* m_screenMemoryBanks[4];
        Ram* m_screenMemoryRamBanks[4];
        unsigned m_bank = 0;
        unsigned m_mode = 0;
        uint16_t m_txtBase = 0;
        uint16_t m_sgBase = 0;
        uint16_t m_grBase = 0;
        uint16_t m_colBase = 0;
        uint32_t m_fgColor = 0xC0C0C0;
        uint32_t m_bgColor = 0x000000;
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
        void inte(bool isActive) override;

        void attachCrtRenderer(Pk8000Renderer* crtRenderer);

        static EmuObject* create(const EmuValuesList&) {return new Pk8000Core();}
    private:
        Pk8000Renderer* m_crtRenderer = nullptr;

        //GeneralSoundSource* m_beepSoundSource;
};


class Pk8000FileLoader : public FileLoader
{
public:
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

        static EmuObject* create(const EmuValuesList&) {return new Pk8000Keyboard();}

    private:

        const EmuKey m_keyMatrix[10][8] = {
            { EK_0,        EK_1,       EK_2,        EK_3,     EK_4,      EK_5,        EK_6,         EK_7     },
            { EK_8,        EK_9,       EK_COMMA,    EK_MINUS, EK_PERIOD, EK_COLON,    EK_SEMICOLON, EK_SLASH },
            { EK_LBRACKET, EK_BKSLASH, EK_RBRACKET, EK_CARET, EK_NONE,   EK_AT,       EK_A,         EK_B     },
            { EK_C,        EK_D,       EK_E,        EK_F,     EK_G,      EK_H,        EK_I,         EK_J     },
            { EK_K,        EK_L,       EK_M,        EK_N,     EK_O,      EK_P,        EK_Q,         EK_R     },
            { EK_S,        EK_T,       EK_U,        EK_V,     EK_W,      EK_X,        EK_Y,         EK_Z     },
            { EK_SHIFT,    EK_CTRL,    EK_NONE,     EK_LANG,  EK_NONE,   EK_F1,       EK_F2,        EK_F3    },
            { EK_F4,       EK_F5,      EK_ESC,      EK_TAB,   EK_NONE,   EK_BSP,      EK_NONE,      EK_CR    },
            { EK_SPACE,    EK_CLEAR,   EK_NONE,     EK_NONE,  EK_LEFT,   EK_UP,       EK_DOWN,      EK_RIGHT },
            { EK_HOME,     EK_NONE,    EK_NONE,     EK_NONE,  EK_NONE,   EK_NONE,     EK_NONE,      EK_SPACE }
        };

            /*{ EK_SHIFT,    EK_CTRL,   ?(ГРАФ),     EK_LANG,  ?(ФИКС),   EK_F1,       EK_F2,        EK_F3     },
            { EK_F4,       EK_F5,     EK_ESC,      EK_TAB,   ?(STOP),   EK_BSP,      ?(СЕЛ),       EK_CR     },
            { EK_SPACE,    EK_CLEAR,  ?(INS),      ?(DEL),   ?EK_LEFT,  ?EK_UP,      ?EK_DOWN,     ?EK_RIGHT },
            { ?EK_HOME,    ?EK_END,   ?(MENU),     ?N1,      ?N3,       ?N.,         ?N0,          EK_SPACE  }*/

        uint8_t m_keys[10];
        uint8_t m_rowNo;

};


class Pk8000Ppi8255Circuit1 : public Ppi8255Circuit
{
    public:
        Pk8000Ppi8255Circuit1();
        virtual ~Pk8000Ppi8255Circuit1();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        uint8_t getPortB() override;
        void setPortA(uint8_t value) override;
        void setPortC(uint8_t value) override;

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
        void setPortA(uint8_t value) override;
        //void setPortB(uint8_t value) override;
        //void setPortC(uint8_t value) override;

        void attachAddrSpaceMapper(int bank, AddrSpaceMapper* addrSpaceMapper);

        static EmuObject* create(const EmuValuesList&) {return new Pk8000Ppi8255Circuit2();}

    protected:
        Pk8000Renderer* m_renderer = nullptr;
};


#endif // PK8000_H

