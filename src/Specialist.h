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

#ifndef SPECIALIST_H
#define SPECIALIST_H

#include "EmuObjects.h"
#include "Memory.h"
#include "CrtRenderer.h"
#include "PlatformCore.h"
#include "Keyboard.h"
#include "Ppi8255Circuit.h"
#include "FileLoader.h"

class GeneralSoundSource;
class Fdc1793;
class AddrSpaceMapper;


class SpecMxMemPageSelector : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        inline void attachAddrSpaceMapper(AddrSpaceMapper* addrSpaceMapper) {m_addrSpaceMapper = addrSpaceMapper;};

        void reset() override;

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int) override {return 0xff;};

    private:
        AddrSpaceMapper* m_addrSpaceMapper = nullptr;
        bool m_onePageMode = false;
};


class SpecVideoRam : public Ram
{
    public:
        SpecVideoRam(int memSize);
        virtual ~SpecVideoRam();

        void writeByte(int addr, uint8_t value) override;
        void reset() override;
        uint8_t* getColorDataPtr() {return m_colorBuf;};

        void setCurColor(uint8_t color);

    private:
        int m_memSize;
        uint8_t m_color = 0;
        uint8_t* m_colorBuf = nullptr;
};


class SpecMxColorRegister : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // Подключение видео-ОЗУ для записи цвета
        void attachVideoRam(SpecVideoRam* videoRam) {m_videoRam = videoRam;};

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int) override {return 0xff;};

    private:
        SpecVideoRam* m_videoRam = nullptr;
};


class SpecRenderer : public CrtRenderer
{
    enum SpecColorMode {
        SCM_MONO,
        SCM_4COLOR,
        SCM_8COLOR,
        SCM_MX // 16-color MX mode
    };

    const uint32_t spec4ColorPalette[4] = {
        0xFFFFFF, 0xFF0000, 0x00FF00, 0x0000FF,
    };

    const uint32_t spec8ColorPalette[8] = {
        0xFFFFFF, 0xFFFF00, 0xFF00FF, 0xFF0000,
        0x00FFFF, 0x00FF00, 0x0000FF, 0x000000
    };

    const uint32_t spec16ColorPalette[16] = {
        0x000000, 0x0000C0, 0x00C000, 0x00C0C0,
        0xC00000, 0xC000C0, 0xC0C000, 0xC0C0C0,
        0x808080, 0x0000FF, 0x00FF00, 0x00FFFF,
        0xFF0000, 0xFF00FF, 0xFFFF00, 0xFFFFFF
    };

    public:
        SpecRenderer();
        void renderFrame() override;

        void toggleColorMode() override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        inline void attachScreenMemory(SpecVideoRam* videoMemory) {m_screenMemory = videoMemory->getDataPtr(); m_colorMemory = videoMemory->getColorDataPtr();};

    private:
        const uint8_t* m_screenMemory = nullptr;
        const uint8_t* m_colorMemory = nullptr;

        SpecColorMode m_colorMode = SCM_8COLOR;
};


class SpecCore : public PlatformCore
{
    public:
        //SpecCore();
        //virtual ~SpecCore();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void draw() override;

        void attachCrtRenderer(SpecRenderer* crtRenderer);

    private:
        SpecRenderer* m_crtRenderer = nullptr;
};


class SpecMxFddControlRegisters : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        inline void attachFdc1793(Fdc1793* fdc) {m_fdc = fdc;};

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int) override {return 0xff;};

    private:
        Fdc1793* m_fdc = nullptr;
};


class SpecKeyboard : public Keyboard
{
    public:
        SpecKeyboard();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        void resetKeys() override;
        void processKey(EmuKey key, bool isPressed) override;

        void setVMatrixMask(uint16_t mask);
        uint8_t getVMatrixData();
        void setHMatrixMask(uint8_t mask);
        uint16_t getHMatrixData();
        bool getShift();

    private:
        const EmuKey m_keyMatrix[12][6] = {
            { EK_CR,    EK_BSP,   EK_PERIOD,  EK_COLON,    EK_MINUS,     EK_CLEAR },
            { EK_LF,    EK_SLASH, EK_BKSLASH, EK_H,        EK_0,         EK_F11   },
            { EK_RIGHT, EK_COMMA, EK_V,       EK_Z,        EK_9,         EK_F10   },
            { EK_CTRL,  EK_AT,    EK_D,       EK_RBRACKET, EK_8,         EK_F9    },
            { EK_LEFT,  EK_B,     EK_L,       EK_LBRACKET, EK_7,         EK_F8    },
            { EK_SPACE, EK_X,     EK_O,       EK_G,        EK_6,         EK_F7    },
            { EK_ESC,   EK_T,     EK_R,       EK_N,        EK_5,         EK_F6    },
            { EK_TAB,   EK_I,     EK_P,       EK_E,        EK_4,         EK_F5    },
            { EK_DOWN,  EK_M,     EK_A,       EK_K,        EK_3,         EK_F4    },
            { EK_UP,    EK_S,     EK_W,       EK_U,        EK_2,         EK_F3    },
            { EK_HOME,  EK_CARET, EK_Y,       EK_C,        EK_1,         EK_F2    },
            { EK_LANG,  EK_Q,     EK_F,       EK_J,        EK_SEMICOLON, EK_F1    }
        };

        const EmuKey m_keyMatrixMx[12][6] = {
            { EK_CR,    EK_BSP,   EK_PERIOD,  EK_COLON,    EK_MINUS,     EK_CLEAR },
            { EK_LF,    EK_SLASH, EK_BKSLASH, EK_H,        EK_0,         EK_F9    },
            { EK_RIGHT, EK_COMMA, EK_V,       EK_Z,        EK_9,         EK_F8    },
            { EK_TAB,   EK_AT,    EK_D,       EK_RBRACKET, EK_8,         EK_F7    },
            { EK_LEFT,  EK_B,     EK_L,       EK_LBRACKET, EK_7,         EK_F6    },
            { EK_SPACE, EK_X,     EK_O,       EK_G,        EK_6,         EK_F5    },
            { EK_NONE,  EK_T,     EK_R,       EK_N,        EK_5,         EK_F4    },
            { EK_NONE,  EK_I,     EK_P,       EK_E,        EK_4,         EK_F3    },
            { EK_DOWN,  EK_M,     EK_A,       EK_K,        EK_3,         EK_F2    },
            { EK_UP,    EK_S,     EK_W,       EK_U,        EK_2,         EK_F1    },
            { EK_HOME,  EK_CARET, EK_Y,       EK_C,        EK_1,         EK_F10   },
            { EK_LANG,  EK_Q,     EK_F,       EK_J,        EK_SEMICOLON, EK_ESC   }
        };

        bool m_mxMatrix = false;

        uint8_t m_vKeys[12];
        uint16_t m_vMask;

        uint16_t m_hKeys[6];
        uint8_t m_hMask;

        bool m_shift;
};


// Обвязка основного ВВ55 в Специалисте
class SpecPpi8255Circuit : public Ppi8255Circuit
{
    public:
        SpecPpi8255Circuit();
        virtual ~SpecPpi8255Circuit();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        uint8_t getPortA() override;
        uint8_t getPortB() override;
        uint8_t getPortC() override;
        void setPortA(uint8_t value) override;
        void setPortB(uint8_t value) override;
        void setPortC(uint8_t value) override;
        void setPortCLoMode(bool isInput) override;
        void setPortAMode(bool isInput) override;
        void setPortBMode(bool isInput) override;

        // Подключение объекта - клавиатуры Специалиста
        void attachSpecKeyboard(SpecKeyboard* kbd);

        // Подключение видео-ОЗУ для записи цвета
        void attachVideoRam(SpecVideoRam* videoRam);


    protected:
        // Источник звука - вывод на магнитофон
        GeneralSoundSource* m_tapeSoundSource;

    private:
        // Клавиатура типа РК86
        SpecKeyboard* m_kbd = nullptr;
        SpecVideoRam* m_videoRam = nullptr;

        uint16_t m_kbdMask;

        bool m_portCloInputMode;
        bool m_portAInputMode;
        bool m_portBInputMode;
};


class SpecRomDisk : public Ppi8255Circuit
{
    public:
        SpecRomDisk() {}; // явно не использовать, для производных классов
        SpecRomDisk(std::string romDiskName);
        virtual ~SpecRomDisk();

        uint8_t getPortA() override {return 0xff;};
        uint8_t getPortB() override;
        uint8_t getPortC() override {return 0xff;};
        void setPortA(uint8_t) override;
        void setPortB(uint8_t) override {};
        void setPortC(uint8_t) override;

    protected:
        uint8_t* m_romDisk;
        unsigned m_curAddr = 0;
};


class SpecFileLoader : public FileLoader
{
    public:
        bool loadFile(const std::string& fileName, bool run = false) override;
};


class SpecMxFileLoader : public FileLoader
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        bool loadFile(const std::string& fileName, bool run = false) override;

    private:
        AddressableDevice* m_ramDisk = nullptr;
        AddrSpaceMapper* m_pageMapper = nullptr;
};


#endif // SPECIALIST_H
