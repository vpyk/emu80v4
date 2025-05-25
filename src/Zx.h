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

#ifndef ZX_H
#define ZX_H

#include "AddrSpace.h"
#include "CrtRenderer.h"
#include "PlatformCore.h"
#include "Keyboard.h"
#include "FileLoader.h"
#include "CpuHook.h"
//#include "CpuWaits.h"

class GeneralSoundSource;
class CpuZ80;

enum ZxModel {
    ZM_48K,
    ZM_128K,
    ZM_PENTAGON
};

class ZxRenderer : public CrtRenderer, public IActive
{
    const uint32_t zxPalette[16] = {
        0x000000, 0x0000c0, 0xc00000, 0xc000c0, 0x00c000, 0x00c0c0, 0xc0c000, 0xc0c0c0,
        0x000000, 0x0000ff, 0xff0000, 0xff00ff, 0x00ff00, 0x00ffff, 0xffff00, 0xffffff
    };

public:
        ZxRenderer();
        ~ZxRenderer();

        void renderFrame() override;

        void toggleColorMode() override;
        void toggleCropping() override;

        void initConnections() override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;
        std::string getDebugInfo() override;

        // derived from ActiveDevice
        void operate() override;

        void setBorderColor(uint8_t color);
        void setScreenPage(int screenPage);

        static EmuObject* create(const EmuValuesList&) {return new ZxRenderer();}

    private:
        CpuZ80* m_cpu = 0;

        const uint8_t* m_screenMemory[2] = {nullptr, nullptr};
        int m_screenPage = 0;
        bool m_showBorder = false;
        bool m_colorMode = true;
        uint8_t m_borderColor = 0;
        int m_flashCnt = 0;

        ZxModel m_model = ZM_128K;
        int m_lineTStates = 228;
        int m_linePixels = 456;
        int m_visibleScanLine = 63;
        int m_scanLines = 311;
        int m_bias = 0;
        int m_vOffset = 23;

        EmuOutput* m_intOutput = nullptr;

        unsigned m_ticksPerTState;
        uint64_t m_curFrameClock = 0;
        int m_curScanLine = 0;
        int m_curFrameTState = 0;

        bool m_intActive = false;

        uint32_t* m_fullFrame = nullptr;

        void advanceTo(uint64_t clocks);
        void drawLine(int line, int fromTState, int toTState);

        void setModel(ZxModel model);
};


class ZxCore : public PlatformCore
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void initConnections() override;

        void draw() override;

        void attachCrtRenderer(CrtRenderer* crtRenderer);

        static EmuObject* create(const EmuValuesList&) {return new ZxCore();}

    private:
        CrtRenderer* m_crtRenderer = nullptr;
        bool m_intReq = false;
        void setInt(bool state);

        void setZxModel(ZxModel model);
};


class Psg3910;
class Fdc1793;
class Register;

class ZxPorts : public AddressableDevice
{
public:
    void reset() override;

    void initConnections() override;

    void writeByte(int addr, uint8_t value) override;
    uint8_t readByte(int addr) override;

    bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

    static EmuObject* create(const EmuValuesList&) {return new ZxPorts();}

private:
    int m_kbdMatrixData = 0;

    Psg3910* m_ay[2] = {nullptr, nullptr};
    int m_curAy = 0;

    Fdc1793* m_fdc = nullptr;
    Register* m_fddRegister = nullptr;
    bool m_bdiActive = false;

    EmuOutput* m_kbdMaskOutput = nullptr;
    EmuOutput* m_portFEOutput = nullptr;
    EmuOutput* m_port7FFDOutput = nullptr;

    bool m_128kMode = true;

    void setKbdMatrixData(int data);
    void setBdiActive(int active) {m_bdiActive = active;}
};


class CpuZ80;

class ZxBdiAddrSpace : public AddrSpace
{
public:
    ZxBdiAddrSpace() : m_bdiActive(false) {}

    void initConnections() override;

    void reset() override;
    uint8_t readByte(int addr) override;

    bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

    static EmuObject* create(const EmuValuesList&) {return new ZxBdiAddrSpace();}

private:
    CpuZ80* m_cpu = nullptr;

    bool m_bdiActive = false;

    EmuOutput* m_bdiActiveOutput = nullptr;
};


class ZxKeyboard : public Keyboard
{
    public:
        ZxKeyboard();

        void resetKeys() override;
        void processKey(EmuKey key, bool isPressed) override;

        void initConnections() override;

        static EmuObject* create(const EmuValuesList&) {return new ZxKeyboard();}

    private:
        const EmuKey m_keyMatrix[8][5] = {
            { EK_SHIFT, EK_Z,    EK_X,  EK_C,  EK_V},
            { EK_A,     EK_S,    EK_D,  EK_F,  EK_G},
            { EK_Q,     EK_W,    EK_E,  EK_R,  EK_T},
            { EK_1,     EK_2,    EK_3,  EK_4,  EK_5},
            { EK_0,     EK_9,    EK_8,  EK_7,  EK_6},
            { EK_P,     EK_O,    EK_I,  EK_U,  EK_Y},
            { EK_CR,    EK_L,    EK_K,  EK_J,  EK_H},
            { EK_SPACE, EK_CTRL, EK_M,  EK_N,  EK_B}
        };

        uint8_t m_keys[8];
        uint8_t m_mask;

        EmuOutput* m_dataOutput = nullptr;
        void setMatrixMask(uint8_t mask);
};


class ZxKbdLayout : public KbdLayout
{
    public:
        ZxKbdLayout() {m_separateRusLat = true;}

        static EmuObject* create(const EmuValuesList&) {return new ZxKbdLayout();}

    protected:
        EmuKey translateKey(PalKeyCode keyCode) override;
        EmuKey translateUnicodeKey(unsigned unicodeKey, PalKeyCode key, bool& shift, bool& lang, bool& ctrl) override;
        bool translateKeyEx(PalKeyCode keyCode, EmuKey& key1, EmuKey& key2) override;
};


class ZxTapeInHook : public CpuHook
{
public:
    ZxTapeInHook(uint16_t addr) : CpuHook(addr) {}

    bool hookProc() override;

    static EmuObject* create(const EmuValuesList& parameters) {return parameters[0].isInt() ? new ZxTapeInHook(parameters[0].asInt()) : nullptr;}
};


class ZxTapeOutHook : public CpuHook
{
public:
    ZxTapeOutHook(uint16_t addr) : CpuHook(addr) {}

    bool hookProc() override;

    static EmuObject* create(const EmuValuesList& parameters) {return parameters[0].isInt() ? new ZxTapeOutHook(parameters[0].asInt()) : nullptr;}
};


class ZxFileLoader : public FileLoader
{
    public:
        ZxFileLoader() {m_multiblockAvailable = true;}

        bool loadFile(const std::string& fileName, bool run = false) override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        static EmuObject* create(const EmuValuesList&) {return new ZxFileLoader();}
};


/*class ZxCpuWaits : public CpuWaits
{
public:
    void initConnections() override;

    int getCpuWaitStates(int, int, int normalClocks) override;

    static EmuObject* create(const EmuValuesList&) {return new ZxCpuWaits();}

private:
    uint64_t m_lastIntTime = 0;

    void setInt(int);
};*/


#endif // ZX_H
