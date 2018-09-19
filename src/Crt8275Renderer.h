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

// Crt8275Renderer.h

#ifndef CRT8275RENDERER_H
#define CRT8275RENDERER_H

#include "CrtRenderer.h"

class Crt8275;


class Crt8275Renderer : public TextCrtRenderer
{

    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        void attachCrt(Crt8275* crt);

        void setFontSetNum(int fontNum) {m_fontNumber = fontNum;};

    protected:
        void toggleCropping() override;
        void setCropping(bool cropping) override;

        virtual const uint8_t* getCurFontPtr(bool, bool, bool) {return nullptr;};
        virtual const uint8_t* getAltFontPtr(bool, bool, bool) {return nullptr;};
        virtual uint32_t getCurFgColor(bool, bool, bool) = 0;
        virtual uint32_t getCurBgColor(bool, bool, bool) {return 0x000000;};
        virtual void customDrawSymbolLine(uint32_t*, uint8_t, int, bool, bool, bool, bool, bool, bool) {};

        Crt8275* m_crt = nullptr;

        int m_fontNumber = 0;

        int m_fntCharHeight;
        int m_fntCharWidth;
        unsigned m_fntLcMask;

        bool m_customDraw = false;

        bool m_ltenOffset;
        bool m_rvvOffset;
        bool m_hgltOffset;
        bool m_gpaOffset;

        bool m_useRvv;

        int m_dataSize = 0;

        bool isRasterPresent() override;

        void primaryRenderFrame() override;
        void altRenderFrame() override;

    private:
        double m_freqMHz;
        double m_frameRate;
        bool m_cropping = false;

        std::string getCrtMode();
        void calcAspectRatio(int charWidth);
        void trimImage(int charWidth, int charHeight);
};


#endif // CRT8275RENDERER_H
