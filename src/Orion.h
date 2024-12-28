/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2024
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

#ifndef ORION_H
#define ORION_H

#include "PlatformCore.h"
#include "CrtRenderer.h"
#include "FileLoader.h"

class Ram;
class SoundMixer;
class PlatformCore;
class Fdc1793;
class AddrSpaceMapper;
class GeneralSoundSource;


class OrionMemPageSelector : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void attachAddrSpaceMapper(AddrSpaceMapper* addrSpaceMapper) {m_addrSpaceMapper = addrSpaceMapper;}

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int)  override {return 0xff;}

        static EmuObject* create(const EmuValuesList&) {return new OrionMemPageSelector();}

    private:
        AddrSpaceMapper* m_addrSpaceMapper = nullptr;
        uint8_t m_mask = 0x03;
};


class OrionRenderer : public CrtRenderer, public IActive
{
    enum OrionColorMode {
        OCM_MONO = 0,
        OCM_BLANK = 1,
        OCM_4COLOR = 2,
        OCM_16COLOR = 3
    };

    const uint32_t orion16ColorPalette[16] = {
        0x000000, 0x0000C0, 0x00C000, 0x00C0C0,
        0xC00000, 0xC000C0, 0xC0C000, 0xC0C0C0,
        0x000000, 0x0000FF, 0x00FF00, 0x00FFFF,
        0xFF0000, 0xFF00FF, 0xFFFF00, 0xFFFFFF
    };

    const uint32_t orion4ColorPalettes[2][5] = {
        {0x000000, 0xC00000, 0x00C000, 0x0000C0,
         0xC0C0C0}, // белый для ч/б режима
        {0x00C0C0, 0xC0C0C0, 0xC0C000, 0xC000C0,
        0xC0C0C0} // белый для ч/б режима
    };

    public:
        OrionRenderer();
        void renderFrame() override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;
        void toggleColorMode()  override {m_isColorMode = !m_isColorMode;}
        void toggleCropping() override;

        // derived from ActiveDevice
        void operate() override;

        void attachScreenMemory(Ram* screenMemory);
        void attachColorMemory(Ram* colorMemory);
        void setScreenBase(uint16_t base);
        void setColorModeByte(uint8_t modeByte);

        static EmuObject* create(const EmuValuesList&) {return new OrionRenderer();}

    private:
        const uint8_t* m_screenMemory = nullptr;
        const uint8_t* m_colorMemory = nullptr;
        uint16_t m_screenBase = 0xC000;
        OrionColorMode m_colorMode = (OrionColorMode)0;
        int m_palette = 0;
        bool m_isColorMode = true;
        bool m_showBorder = false;
};


class OrionScreenSelector : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        inline void attachCrtRenderer(OrionRenderer* renderer) {m_renderer = renderer;}

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int)  override {return 0xff;}

        static EmuObject* create(const EmuValuesList&) {return new OrionScreenSelector();}

    private:
        OrionRenderer* m_renderer = nullptr;
};


class OrionColorModeSelector : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        inline void attachCrtRenderer(OrionRenderer* renderer) {m_renderer = renderer;}

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int)  override {return 0xff;}

        static EmuObject* create(const EmuValuesList&) {return new OrionColorModeSelector();}

    private:
        OrionRenderer* m_renderer = nullptr;
};


class OrionFddControlRegister : public AddressableDevice
{
    enum OrionFddControlRegisterType {
        OFT_STANDARD, // port F714, F720
        OFT_SPDOS     // port F708
    };

    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        inline void attachFdc1793(Fdc1793* fdc) {m_fdc = fdc;}

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int)  override {return 0xff;}

        static EmuObject* create(const EmuValuesList&) {return new OrionFddControlRegister();}

    private:
        Fdc1793* m_fdc = nullptr;
        OrionFddControlRegisterType m_type = OFT_STANDARD;
};


class OrionFddQueryRegister : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        inline void attachFdc1793(Fdc1793* fdc) {m_fdc = fdc;}

        void writeByte(int, uint8_t)  override {}
        uint8_t readByte(int addr) override;

        static EmuObject* create(const EmuValuesList&) {return new OrionFddQueryRegister();}

    private:
        Fdc1793* m_fdc = nullptr;
};


class OrionCore : public PlatformCore
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void draw() override;
        void inte(bool isActive) override;

        void attachCrtRenderer(OrionRenderer* crtRenderer);

        static EmuObject* create(const EmuValuesList&) {return new OrionCore();}

    private:
        OrionRenderer* m_crtRenderer = nullptr;

        GeneralSoundSource* m_beepSoundSource;
};


class OrionFileLoader : public FileLoader
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        bool loadFile(const std::string& fileName, bool run = false) override;

        static EmuObject* create(const EmuValuesList&) {return new OrionFileLoader();}

    private:
        AddressableDevice* m_ramDisk = nullptr;
};


#endif // ORION_H

