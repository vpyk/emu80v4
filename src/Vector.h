/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2019-2020
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

#ifndef VECTOR_H
#define VECTOR_H

#include "PlatformCore.h"
#include "Ppi8255Circuit.h"
#include "CrtRenderer.h"
#include "FileLoader.h"
#include "Keyboard.h"
#include "KbdLayout.h"
#include "CpuWaits.h"

class Ram;
class Rom;
class Fdc1793;
class GeneralSoundSource;
class Cpu8080Compatible;
class Covox;


class VectorRenderer : public CrtRenderer, public IActive
{
    public:
        VectorRenderer();
        ~VectorRenderer();

        void renderFrame() override;

        // derived from EmuObject
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;
        std::string getDebugInfo() override;

        // derived from CrtRenderer
        void toggleCropping() override;
        void toggleColorMode() override;
        void prepareDebugScreen() override;

        // derived from ActiveDevice
        void operate() override;

        void attachMemory(Ram* memory);

        void setBorderColor(uint8_t color);
        void set512pxMode(bool mode512);
        void setLineOffset(uint8_t lineOffset);
        void setPaletteColor(uint8_t color);
        void vidMemWriteNotify();

        static EmuObject* create(const EmuValuesList&) {return new VectorRenderer();}

    private:
        const uint8_t c_bwMap[256] = {
            0, 13, 20, 34, 42, 56, 63, 76, 20, 34, 41, 54, 63, 76, 83, 97,
            42, 56, 63, 76, 85, 98, 105, 118, 63, 76, 83, 97, 105, 118, 126, 139,
            82, 96, 103, 116, 125, 138, 145, 158, 103, 116, 123, 137, 145, 158, 166, 179,
            125, 138, 145, 158, 167, 180, 187, 201, 145, 158, 166, 179, 187, 201, 208, 221,
            13, 27, 34, 47, 56, 69, 76, 89, 34, 47, 54, 68, 76, 89, 97, 110,
            56, 69, 76, 89, 98, 111, 118, 132, 76, 89, 97, 110, 118, 132, 139, 152,
            96, 109, 116, 129, 138, 151, 158, 172, 116, 129, 137, 150, 158, 172, 179, 192,
            138, 151, 158, 172, 180, 194, 201, 214, 158, 172, 179, 192, 201, 214, 221, 235,
            20, 34, 41, 54, 63, 76, 83, 97, 41, 54, 61, 75, 83, 97, 104, 117,
            63, 76, 83, 97, 105, 118, 126, 139, 83, 97, 104, 117, 126, 139, 146, 159,
            103, 116, 123, 137, 145, 158, 166, 179, 123, 137, 144, 157, 166, 179, 186, 199,
            145, 158, 166, 179, 187, 201, 208, 221, 166, 179, 186, 199, 208, 221, 228, 242,
            34, 47, 54, 68, 76, 89, 97, 110, 54, 68, 75, 88, 97, 110, 117, 130,
            76, 89, 97, 110, 118, 132, 139, 152, 97, 110, 117, 130, 139, 152, 159, 173,
            116, 129, 137, 150, 158, 172, 179, 192, 137, 150, 157, 170, 179, 192, 199, 213,
            158, 172, 179, 192, 201, 214, 221, 235, 179, 192, 199, 213, 221, 235, 242, 255
            // .02-version
            /*0, 17, 22, 40, 37, 54, 59, 76, 21, 38, 43, 60, 57, 75, 80, 97,
            40, 58, 63, 80, 77, 94, 100, 117, 61, 78, 84, 101, 98, 115, 120, 138,
            81, 98, 103, 121, 118, 135, 140, 157, 102, 119, 124, 141, 138, 156, 161, 178,
            121, 138, 144, 161, 158, 175, 180, 198, 142, 159, 164, 182, 179, 196, 201, 218,
            16, 33, 38, 56, 53, 70, 75, 92, 37, 54, 59, 76, 73, 91, 96, 113,
            56, 73, 79, 96, 93, 110, 115, 133, 77, 94, 99, 117, 114, 131, 136, 153,
            97, 114, 119, 136, 133, 151, 156, 173, 117, 135, 140, 157, 154, 171, 177, 194,
            137, 154, 160, 177, 174, 191, 196, 214, 158, 175, 180, 198, 195, 212, 217, 234,
            21, 38, 43, 60, 57, 75, 80, 97, 41, 59, 64, 81, 78, 95, 101, 118,
            61, 78, 84, 101, 98, 115, 120, 138, 82, 99, 104, 122, 119, 136, 141, 158,
            102, 119, 124, 141, 138, 156, 161, 178, 122, 140, 145, 162, 159, 176, 182, 199,
            142, 159, 164, 182, 179, 196, 201, 218, 163, 180, 185, 202, 199, 217, 222, 239,
            37, 54, 59, 76, 73, 91, 96, 113, 57, 75, 80, 97, 94, 111, 117, 134,
            77, 94, 99, 117, 114, 131, 136, 153, 98, 115, 120, 137, 134, 152, 157, 174,
            117, 135, 140, 157, 154, 171, 177, 194, 138, 155, 161, 178, 175, 192, 197, 215,
            158, 175, 180, 198, 195, 212, 217, 234, 179, 196, 201, 218, 215, 233, 238, 255*/
        };

        uint32_t* m_frameBuf;
        const uint8_t* m_screenMemory;

        bool m_showBorder = false;
        bool m_colorMode = true;

        uint8_t m_lineOffset = 0xFF;
        uint8_t m_latchedLineOffset = 0xFF;
        bool m_lineOffsetIsLatched = false;
        uint8_t m_borderColor = 0;
        bool m_mode512px = false;
        bool m_mode512pxLatched = false;
        uint32_t m_colorPalette[16];
        uint32_t m_bwPalette[16];
        const uint32_t* m_palette= m_colorPalette;
        int m_lastColor = 0;

        unsigned m_ticksPerPixel;

        uint64_t m_curScanlineClock;

        uint64_t m_curFrameClock;
        int m_curFramePixel;

        void setColorMode(bool colorMode);
        void prepareFrame();
        void renderLine(int nLine, int firstPx, int LastPx);
        //void advance();
        void advanceTo(uint64_t clocks);
};


class VectorCore : public PlatformCore
{
    public:
        VectorCore();
        //virtual ~VectorCore();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void draw() override;
        void reset() override;
        void vrtc(bool isActive) override;
        void inte(bool isActive) override;

        void attachCrtRenderer(VectorRenderer* crtRenderer);

        static EmuObject* create(const EmuValuesList&) {return new VectorCore();}
    private:
        VectorRenderer* m_crtRenderer = nullptr;
        bool m_intReq = false;
        bool m_intsEnabled = false;
};


class VectorAddrSpace : public AddressableDevice
{
    public:
        //VectorAddrSpace(std::string romFileName);
        //~VectorAddrSpace() override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        void reset() override;

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        void attachRam(AddressableDevice* mem) {m_mainMemory = mem;}
        void attachRom(Rom* rom) {m_rom = rom;}
        void attachRamDisk(AddressableDevice* ramDisk) {m_ramDisk = ramDisk;}
        void attachCrtRenderer(VectorRenderer* crtRenderer) {m_crtRenderer = crtRenderer;};
        void enableRom() {m_romEnabled = true;}
        void disableRom() {m_romEnabled = false;}
        void ramDiskControl(int inRamPagesMask, bool stackEnabled, int inRamPage, int stackPage);

        static EmuObject* create(const EmuValuesList&) {return new VectorAddrSpace();}

    private:
        AddressableDevice* m_mainMemory = nullptr;
        Rom* m_rom = nullptr;
        AddressableDevice* m_ramDisk = nullptr;
        Cpu8080Compatible* m_cpu = nullptr;
        VectorRenderer* m_crtRenderer = nullptr;

        bool m_romEnabled = true;
        int m_inRamPagesMask = 0;
        bool m_stackDiskEnabled = false;
        int m_inRamDiskPage = 0;
        int m_stackDiskPage = 0;
};


class VectorFileLoader : public FileLoader
{
public:
    //VectorFileLoader();
    bool loadFile(const std::string& fileName, bool run = false) override;

    static EmuObject* create(const EmuValuesList&) {return new VectorFileLoader();}
};


class VectorKeyboard : public Keyboard
{
    public:
        VectorKeyboard();

        void resetKeys() override;
        void processKey(EmuKey key, bool isPressed) override;

        void setMatrixMask(uint8_t mask) {m_mask = ~mask;}
        uint8_t getMatrixData();
        uint8_t getCtrlKeys() {return ~m_ctrlKeys;}

        static EmuObject* create(const EmuValuesList&) {return new VectorKeyboard();}

    private:

        const EmuKey m_keyMatrix[8][8] = {
            { EK_TAB,  EK_LF,    EK_CR,    EK_BSP,       EK_LEFT,    EK_UP,       EK_RIGHT,  EK_DOWN  },
            { EK_HOME, EK_CLEAR, EK_ESC,   EK_F1,        EK_F2,      EK_F3,       EK_F4,     EK_F5    },
            { EK_0,    EK_1,     EK_2,     EK_3,         EK_4,       EK_5,        EK_6,      EK_7     },
            { EK_8,    EK_9,     EK_COLON, EK_SEMICOLON, EK_COMMA,   EK_MINUS,    EK_PERIOD, EK_SLASH },
            { EK_AT,   EK_A,     EK_B,     EK_C,         EK_D,       EK_E,        EK_F,      EK_G     },
            { EK_H,    EK_I,     EK_J,     EK_K,         EK_L,       EK_M,        EK_N,      EK_O     },
            { EK_P,    EK_Q,     EK_R,     EK_S,         EK_T,       EK_U,        EK_V,      EK_W     },
            { EK_X,    EK_Y,     EK_Z,     EK_LBRACKET,  EK_BKSLASH, EK_RBRACKET, EK_CARET,  EK_SPACE }
        };

        const EmuKey m_ctrlKeyMatrix[8] = {
            EK_NONE, EK_NONE, EK_NONE, EK_NONE, EK_NONE, EK_SHIFT, EK_CTRL, EK_LANG
        };

        uint8_t m_keys[8];
        uint8_t m_ctrlKeys;
        uint8_t m_mask;
};


class VectorPpi8255Circuit : public Ppi8255Circuit
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        uint8_t getPortB() override; // port 02
        uint8_t getPortC() override; // port 01
        void setPortA(uint8_t value) override; // port 03
        void setPortB(uint8_t value) override; // port 02
        void setPortC(uint8_t value) override; // port 01

        void attachKeyboard(VectorKeyboard* kbd) {m_kbd = kbd;}
        void attachRenderer(VectorRenderer* renderer) {m_renderer = renderer;}

        static EmuObject* create(const EmuValuesList&) {return new VectorPpi8255Circuit();}

    private:
        // Источник звука - вывод на магнитофон
        GeneralSoundSource* m_tapeSoundSource;

        VectorKeyboard* m_kbd = nullptr;
        VectorRenderer* m_renderer = nullptr;
};


class VectorPpi8255Circuit2 : public Ppi8255Circuit
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        void setPortA(uint8_t value) override; // port 03

        void attachCovox(Covox* covox) {m_covox = covox;}

        static EmuObject* create(const EmuValuesList&) {return new VectorPpi8255Circuit2();}

    private:
        // Источник звука - ковокс
        Covox* m_covox;
};


class VectorColorRegister : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void attachRenderer(VectorRenderer* renderer) {m_renderer = renderer;}

        void writeByte(int addr, uint8_t value) override;
        //uint8_t readByte(int) override;

        static EmuObject* create(const EmuValuesList&) {return new VectorColorRegister();}

    private:
        VectorRenderer* m_renderer = nullptr;
};


class VectorCpuWaits : public CpuWaits
{
public:
    int getCpuWaitStates(int, int, int normalClocks) override;

    static EmuObject* create(const EmuValuesList&) {return new VectorCpuWaits();}
};


class VectorZ80CpuWaits : public CpuWaits
{
public:
    int getCpuWaitStates(int, int opcode, int normalClocks) override;

    static EmuObject* create(const EmuValuesList&) {return new VectorZ80CpuWaits();}
};


class VectorKbdLayout : public RkKbdLayout
{
    public:
        static EmuObject* create(const EmuValuesList&) {return new VectorKbdLayout();}

    protected:
        bool processSpecialKeys(PalKeyCode keyCode) override;
};


class VectorRamDiskSelector : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void attachVectorAddrSpace(VectorAddrSpace* vectorAddrSpace) {m_vectorAddrSpace = vectorAddrSpace;}

        void writeByte(int, uint8_t value) override;
        uint8_t readByte(int)  override {return 0xff;}

        static EmuObject* create(const EmuValuesList&) {return new VectorRamDiskSelector();}

    private:
        VectorAddrSpace* m_vectorAddrSpace = nullptr;
};


class VectorFddControlRegister : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        inline void attachFdc1793(Fdc1793* fdc) {m_fdc = fdc;}

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int)  override {return 0xff;}

        static EmuObject* create(const EmuValuesList&) {return new VectorFddControlRegister();}

    private:
        Fdc1793* m_fdc = nullptr;
};


#endif // VECTOR_H
