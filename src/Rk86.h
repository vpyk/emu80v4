/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2026
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

#ifndef RK86_H
#define RK86_H

#include "PlatformCore.h"
#include "Crt8275Renderer.h"

class GeneralSoundSource;


class Rk86Core : public PlatformCore
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void draw() override;

        void vrtc(bool isActive) override;
        void inte(bool isActive) override;

        void attachCrtRenderer(Crt8275Renderer* crtRenderer);

        static EmuObject* create(const EmuValuesList&) {return new Rk86Core();}

    private:
        Crt8275Renderer* m_crtRenderer = nullptr;
        GeneralSoundSource* m_beepSoundSource;
};


class Rk86Renderer : public Crt8275Renderer
{
    public:
        Rk86Renderer();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        void toggleColorMode() override;

        static EmuObject* create(const EmuValuesList&) {return new Rk86Renderer();}

    protected:
        enum Rk86ColorMode {
            RCM_MONO_ORIG,
            RCM_MONO,
            RCM_COLOR1,
            RCM_COLOR2,
            RCM_COLOR3
        };

        const uint8_t* getCurFontPtr(bool gpa0, bool gpa1, bool hglt) override;
        const uint8_t* getAltFontPtr(bool gpa0, bool gpa1, bool hglt) override;
        uint32_t getCurFgColor(bool gpa0, bool gpa1, bool hglt) override;
        uint32_t getCurBgColor(bool gpa0, bool gpa1, bool hglt) override;
        wchar_t getUnicodeSymbol(uint8_t chr, bool gpa0, bool gpa1, bool hglt) override;
        Rk86ColorMode m_colorMode = RCM_COLOR1;

        virtual void setColorMode(Rk86ColorMode mode);
};


class RkPixeltronRenderer : public Crt8275Renderer
{
    public:
        RkPixeltronRenderer();

        static EmuObject* create(const EmuValuesList&) {return new RkPixeltronRenderer();}

    protected:
        void customDrawSymbolLine(uint32_t* linePtr, uint8_t symbol, int line, bool lten, bool vsp, bool rvv, bool gpa0, bool gpa1, bool hglt) override;
};


class RkRamFontRenderer : public Crt8275Renderer
{
    public:
        RkRamFontRenderer();
        ~RkRamFontRenderer();

        void reset() override;

        void primaryRenderFrame() override;

        void initConnections() override;

        static EmuObject* create(const EmuValuesList&) {return new RkRamFontRenderer();}

    protected:
        void customDrawSymbolLine(uint32_t* linePtr, uint8_t symbol, int line, bool lten, bool vsp, bool rvv, bool gpa0, bool gpa1, bool hglt) override;

    private:
        const uint32_t c_palette[16] = {
            0x000000, 0x800000, 0x008000, 0x808000,
            0x000080, 0x800080, 0x008080, 0x808080,
            0x000000, 0xFF0000, 0x00FF00, 0xFFFF00,
            0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF,
        };

        uint8_t* m_ramFont = nullptr;
        uint32_t m_fgColors[8];
        uint32_t m_bgColors[8];

        void setColorRec(uint8_t color);
};

#endif // RK86_H
