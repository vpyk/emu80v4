/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2020-2024
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

#ifndef LVOV_H
#define LVOV_H

#include "Memory.h"
#include "CrtRenderer.h"
#include "PlatformCore.h"
#include "Ppi8255Circuit.h"
#include "Keyboard.h"
#include "CpuWaits.h"
#include "FileLoader.h"

class GeneralSoundSource;
class AddrSpaceMapper;


class LvovRenderer : public CrtRenderer, public IActive
{
    /*const uint32_t lvovPalette[4] = {
        0x000000, 0x0000FF, 0x00FF00, 0xFF0000,
    };*/

    const uint32_t lvovPalette[8] = {
        0x000000, 0x0000FF, 0x00FF00, 0x00FFFF, 0xFF0000, 0xFF00FF, 0xFFFF00, 0xFFFFFF
    };

    const uint32_t lvovBwPalette[4] = {
        0x000000, 0x555555, 0xAAAAAA, 0xFFFFFF
    };

public:
        LvovRenderer();
        void renderFrame() override;

        void toggleColorMode() override;
        void toggleCropping() override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        // derived from ActiveDevice
        void operate() override;

        inline void attachScreenMemory(Ram* videoMemory) {m_screenMemory = videoMemory->getDataPtr();}
        inline void setPaletteByte(uint8_t palette) {m_paletteByte = palette;}

        static EmuObject* create(const EmuValuesList&) {return new LvovRenderer();}

    private:
        const uint8_t* m_screenMemory = nullptr;
        bool m_showBorder = false;
        uint8_t m_paletteByte = 0;
        bool m_colorMode = true;

};


class LvovCore : public PlatformCore
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void draw() override;

        void attachCrtRenderer(CrtRenderer* crtRenderer);

        static EmuObject* create(const EmuValuesList&) {return new LvovCore();}

    private:
        CrtRenderer* m_crtRenderer = nullptr;
};


class LvovKeyboard : public Keyboard
{
    public:
        LvovKeyboard();

        void resetKeys() override;
        void processKey(EmuKey key, bool isPressed) override;

        void setMatrix1Mask(uint8_t mask);
        void setMatrix2Mask(uint8_t mask);
        uint8_t getMatrix1Data();
        uint8_t getMatrix2Data();

        static EmuObject* create(const EmuValuesList&) {return new LvovKeyboard();}

    private:
        const EmuKey m_keyMatrix1[8][8] = {
            { EK_6,         EK_7,        EK_8,        EK_GT,    EK_TAB,    EK_MINUS,   EK_0,     EK_9     },
            { EK_G,         EK_LBRACKET, EK_RBRACKET, EK_CR,    EK_LF,     EK_COLON,   EK_H,     EK_Z     },
            { EK_R,         EK_O,        EK_L,        EK_BSP,   EK_PERIOD, EK_BKSLASH, EK_V,     EK_D     },
            { EK_SPACE,     EK_B,        EK_AT,       EK_VR,    EK_UNDSCR, EK_LAT,     EK_SLASH, EK_COMMA },
            { EK_CLEAR,     EK_FG,       EK_FB,       EK_5,     EK_4,      EK_3,       EK_2,     EK_1     },
            { EK_NONE,      EK_NONE,     EK_J,        EK_N,     EK_E,      EK_K,       EK_U,     EK_C     },
            { EK_SEMICOLON, EK_RUS,      EK_CTRL,     EK_P,     EK_A,      EK_W,       EK_Y,     EK_F     },
            { EK_SHIFT,     EK_Q,        EK_CARET,    EK_X,     EK_T,      EK_I,       EK_M,     EK_S     }
        };

        const EmuKey m_keyMatrix2[4][4] = {
            { EK_PRN,   EK_CD, EK_SPK,  EK_FR, },
            { EK_SCR,   EK_F0, EK_F1,   EK_F2, },
            { EK_HOME,  EK_F5, EK_F4,   EK_F3, },
            { EK_RIGHT, EK_UP, EK_LEFT, EK_DOWN }
        };

        uint8_t m_keys1[8];
        uint16_t m_mask1;

        uint8_t m_keys2[4];
        uint8_t m_mask2;
};


// Обвязка основного ВВ55 в Специалисте
class LvovPpi8255Circuit1 : public Ppi8255Circuit
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        uint8_t getPortC() override;
        void setPortA(uint8_t value) override;
        void setPortB(uint8_t value) override;
        void setPortC(uint8_t value) override;

        // attach LvovRenderer
        void attachRenderer(LvovRenderer* renderer) {m_renderer = renderer;}

        void attachAddrSpaceMapper(AddrSpaceMapper* addrSpaceMapper) {m_addrSpaceMapper = addrSpaceMapper;}

        static EmuObject* create(const EmuValuesList&) {return new LvovPpi8255Circuit1();}

    private:
        // Lvov renderer for palette setting
        LvovRenderer* m_renderer = nullptr;

        // sound source - tape output
        GeneralSoundSource* m_tapeSoundSource = nullptr;

        // sound source - internal beeper
        GeneralSoundSource* m_beepSoundSource = nullptr;

        // sound related bits
        bool m_pb7 = false;
        bool m_pc0 = false;

        uint8_t m_printerData = 0;
        bool m_printerStrobe = true;
        bool m_printerReady = true;

        // address space mapper to switch memory pages
        AddrSpaceMapper* m_addrSpaceMapper = nullptr;
};


class LvovPpi8255Circuit2 : public Ppi8255Circuit
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        uint8_t getPortB() override;
        uint8_t getPortC() override;
        void setPortA(uint8_t value) override;
        void setPortC(uint8_t value) override;

        void attachLvovKeyboard(LvovKeyboard* kbd) {m_kbd = kbd;}

        static EmuObject* create(const EmuValuesList&) {return new LvovPpi8255Circuit2();}

    private:
        LvovKeyboard* m_kbd = nullptr;
};


class LvovKbdLayout : public KbdLayout
{
    public:
        LvovKbdLayout() {m_separateRusLat = true;}

        static EmuObject* create(const EmuValuesList&) {return new LvovKbdLayout();}

    protected:
        EmuKey translateKey(PalKeyCode keyCode) override;
        EmuKey translateUnicodeKey(unsigned unicodeKey, PalKeyCode key, bool& shift, bool& lang) override;
};


class LvovCpuWaits : public CpuWaits
{
public:
    int getCpuWaitStates(int memTag, int opcode, int normalClocks) override;

    static EmuObject* create(const EmuValuesList&) {return new LvovCpuWaits();}
};


class LvovCpuCycleWaits : public CpuCycleWaits
{
public:
    int getCpuCycleWaitStates(int memTag, bool write) override;

    static EmuObject* create(const EmuValuesList&) {return new LvovCpuCycleWaits();}

private:
    int m_curWaits = 0;
    int m_curWriteWaits = 0;
};


class LvovFileLoader : public FileLoader
{
    public:
        LvovFileLoader() {m_multiblockAvailable = true;}

        bool loadFile(const std::string& fileName, bool run = false) override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        static EmuObject* create(const EmuValuesList&) {return new LvovFileLoader();}

    private:
        AddressableDevice* m_video = nullptr;
        AddressableDevice* m_io = nullptr;

        int m_fileSize;
        int m_fullSize;
        std::string m_fileName;
        uint8_t* m_buf;
        uint8_t* m_ptr;

        bool loadBinary(bool run);
        bool loadBasic(bool run);
        void loadDump(bool run);

        void setupMultiblock();
};


#endif // LVOV_H
