/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2025
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

#ifndef OCEAN_H
#define OCEAN_H

//#include "Memory.h"
#include "CrtRenderer.h"
#include "PlatformCore.h"
#include "RkKeyboard.h"
#include "FileLoader.h"

class Fdc1793;
class AddrSpaceMapper;
class SpecMxPit8253SoundSource;
class Pic8259;

class OkeanRenderer : public CrtRenderer, public IActive
{
    public:
        OkeanRenderer();
        ~OkeanRenderer();

        void initConnections() override;

        void renderFrame() override;

        void toggleColorMode() override;
        void toggleCropping() override;
        void prepareDebugScreen() override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;
        //std::string getDebugInfo() override;

        // derived from ActiveDevice
        void operate() override;

        void setMode(bool mode) {m_lowResMode = mode;}
        void setPage(int page) {m_page = page;}
        void setPalette(int palette) {m_palette = palette;}
        void setVscroll(uint8_t vscroll) {m_vscroll = vscroll;}
        void setHscroll(uint8_t hscroll);
        void setBgColor(uint8_t bgColor);

        static EmuObject* create(const EmuValuesList&) {return new OkeanRenderer();}

    private:
        enum class ColorMode {
            Color1,
            Color2,
            GrayScale
        };

        uint32_t m_colorPalette1[8][4] = {
            {0x000000, 0x00ff00, 0x0000ff, 0xff0000},
            {0xffffff, 0x00ff00, 0x0000ff, 0xff0000},
            {0x00ff00, 0x0000ff, 0xff00ff, 0x00ffff},
            {0x000000, 0x00ff00, 0xffff00, 0xffffff},
            {0x000000, 0x00ff00, 0x00ffff, 0xff0000},
            {0x000000, 0xff0000, 0x0000ff, 0x00ffff},
            {0x0000ff, 0xffffff, 0x00ffff, 0xff0000},
            {0x000000, 0x000000, 0x000000, 0x000000}
        };

        uint32_t m_colorPalette2[8][4] = {
            {0x000000, 0xff0000, 0x00ff00, 0x0000ff},
            {0xffffff, 0xff0000, 0x00ff00, 0x0000ff},
            {0xff0000, 0x00ff00, 0x00ffff, 0xffff00},
            {0x000000, 0xff0000, 0xff00ff, 0xffffff},
            {0x000000, 0xff0000, 0xffff00, 0x0000ff},
            {0x000000, 0x0000ff, 0x00ff00, 0xffff00},
            {0x00ff00, 0xffffff, 0xffff00, 0x0000ff},
            {0x000000, 0x000000, 0x000000, 0x000000}
        };

        uint32_t m_colorPaletteGray[8][4] = {
            {0x000000, 0x555555, 0x555555, 0x555555},
            {0xffffff, 0x555555, 0x555555, 0x555555},
            {0x555555, 0x555555, 0x808080, 0x808080},
            {0x000000, 0x555555, 0x808080, 0xffffff},
            {0x000000, 0x555555, 0x808080, 0x555555},
            {0x000000, 0x555555, 0x555555, 0x808080},
            {0x555555, 0xffffff, 0x808080, 0x555555},
            {0x000000, 0x000000, 0x000000, 0x000000}
        };


        /*uint32_t m_monoPalette1[2][8] = {
            {0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000},
            {0xff0000, 0x0000ff, 0xff00ff, 0x00ff00, 0xffff00, 0x00ffff, 0xffffff, 0x000000}
        };*/

        uint32_t m_monoPalette1[2][8] = {
            {0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x0000ff, 0x000000},
            {0xffffff, 0xff0000, 0x00ff00, 0x0000ff, 0x00ffff, 0xffff00, 0x000000, 0x000000}
        };

        uint32_t m_monoPalette2[2][8] = {
            {0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x00ff00, 0x000000},
            {0xffffff, 0x00ff00, 0x0000ff, 0xff0000, 0xff00ff, 0x00ffff, 0x000000, 0x000000}
        };

        uint32_t m_monoPaletteGray[2][8] = {
            {0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000},
            {0x555555, 0x555555, 0x808080, 0x555555, 0x808080, 0x808080, 0xffffff, 0x000000}
        };

        uint32_t (*m_colorPalette)[4] = m_colorPalette1;
        uint32_t (*m_monoPalette)[8] = m_monoPalette1;

        const uint8_t* m_screenMemory[2] = {nullptr, nullptr};
        uint32_t* m_frameBuf = nullptr;

        bool m_lowResMode = false;
        int m_page = 0;
        int m_palette = 0;
        uint8_t m_vscroll = 0;
        int m_hscroll = 0;
        uint32_t m_bgColor = 0;
        int m_screenPage = 0;

        ColorMode m_colorMode = ColorMode::Color1;
        bool m_showBorder = false;

        int m_curLine = 0;
        bool m_hblank = true;

        int m_offsetX = 0;
        int m_offsetY = 0;

        void renderLine(int nLine);
        void setColorMode(ColorMode colorMode);

        EmuOutput* m_vblankOutput = nullptr;
        EmuOutput* m_hblankOutput = nullptr;
};


class OkeanCore : public PlatformCore
{
    public:
        //OkeanCore();
        //virtual ~OkeanCore();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void draw() override;
        void inte(bool isActive) override;

//        void attachCrtRenderer(CrtRenderer* crtRenderer);

        static EmuObject* create(const EmuValuesList&) {return new OkeanCore();}

    private:
        CrtRenderer* m_crtRenderer = nullptr;
        Pic8259* m_pic = nullptr;
};


class OkeanKeyboard : public Keyboard
{
    public:
        //OkeanKeyboard();

        void initConnections() override;

        void resetKeys() override {}
        void processKey(EmuKey key, bool isPressed) override;
        bool processKeyCode(int keyCode) override;

        static EmuObject* create(const EmuValuesList&) {return new OkeanKeyboard();}

    private:

    void ack(bool val);

    EmuOutput* m_strobeOutput = nullptr;
    EmuOutput* m_keyCodeOutput = nullptr;

    bool m_strobe = false;
    int m_code = 0;
};


class OkeanMatrixKeyboard : public Ms7007Keyboard
{
public:
    //OkeanMatrixKeyboard();

    void initConnections() override;

    void resetKeys() override;
    void processKey(EmuKey key, bool isPressed) override;

    static EmuObject* create(const EmuValuesList&) {return new OkeanMatrixKeyboard();}

private:

    void ack(bool val);

    EmuOutput* m_strobeOutput = nullptr;
    EmuOutput* m_columnOutput = nullptr;
    EmuOutput* m_rowDataOutput = nullptr;
    EmuOutput* m_shiftOutput = nullptr;
    EmuOutput* m_ctrlOutput = nullptr;

    bool m_strobe = false;
    int m_rowData = 0;
    int m_column = 0;
    bool m_shift = false;
    bool m_ctrl = false;
};


class OkeanKbdLayout : public KbdLayout
{
public:
    static EmuObject* create(const EmuValuesList&) {return new OkeanKbdLayout();}

protected:
    EmuKey translateKey(PalKeyCode keyCode) override;
};


class OkeanFileLoader : public FileLoader
{
    public:
        bool loadFile(const std::string& fileName, bool run = false) override;

        static EmuObject* create(const EmuValuesList&) {return new OkeanFileLoader();}
};


class OkeanFddRegisters : public AddressableDevice
{
public:
    bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

    void writeByte(int addr, uint8_t value) override;
    uint8_t readByte(int addr)  override;

    static EmuObject* create(const EmuValuesList&) {return new OkeanFddRegisters();}

private:
    Fdc1793* m_fdc = nullptr;
    int m_drive = 0;
    int m_head = 0;
};


#endif // OCEAN_H
