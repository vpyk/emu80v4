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

#ifndef APOGEY_H
#define APOGEY_H


#include "PlatformCore.h"
#include "RkRomDisk.h"
#include "Crt8275Renderer.h"

class SoundSource;


class ApogeyCore : public PlatformCore
{
    public:
        ApogeyCore();
        virtual ~ApogeyCore();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void draw() override;

        void vrtc(bool isActive) override;
        void inte(bool isActive) override;

        void attachCrtRenderer(Crt8275Renderer* crtRenderer);

        static EmuObject* create(const EmuValuesList&) {return new ApogeyCore();}

    private:
        Crt8275Renderer* m_crtRenderer = nullptr;
};


class ApogeyRenderer : public Crt8275Renderer
{
    public:
        ApogeyRenderer();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;
        void toggleColorMode() override;

        static EmuObject* create(const EmuValuesList&) {return new ApogeyRenderer();}

    protected:
        const uint8_t* getCurFontPtr(bool gpa0, bool gpa1, bool hglt) override;
        const uint8_t* getAltFontPtr(bool gpa0, bool gpa1, bool hglt) override;
        uint32_t getCurFgColor(bool gpa0, bool gpa1, bool hglt) override;
        uint32_t getCurBgColor(bool gpa0, bool gpa1, bool hglt) override;
        wchar_t getUnicodeSymbol(uint8_t chr, bool gpa0, bool gpa1, bool hglt) override;

    private:
        enum class ColorMode {
            Mono,
            Color,
            Grayscale
        };

        ColorMode m_colorMode = ColorMode::Color;
        void setColorMode(ColorMode colorMode);

        const wchar_t* c_apogeySymbols =
            L" ▘▝▀▗▚▐▜ ★⬯↑⬮ ↣↓▖▌▞▛▄▙▟█∼≈╋┃━↢✿▕"
            L" !\"#¤%&'()*+,-./0123456789:;<=>?"
            L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
            L"ЮАБЦДЕФГХИЙКЛМНОПЯРСТУЖВЬЫЗШЭЩЧ▒";

};


class ApogeyRomDisk : public RkRomDisk
{
    public:
        ApogeyRomDisk(std::string romDiskName) : RkRomDisk(romDiskName) {}
        void setPortC(uint8_t value) override;
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        static EmuObject* create(const EmuValuesList& parameters) {return new ApogeyRomDisk(parameters[0].asString());}

    private:
        bool m_oldA15 = false;
        uint8_t m_mask = 0xf; // 512KB
};

#endif // APOGEY_H
