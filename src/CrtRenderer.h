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

// CrtRenderer.h

#ifndef CRTRENDERER_H
#define CRTRENDERER_H

#include "EmuTypes.h"
#include "EmuObjects.h"


class CrtRenderer : public EmuObject
{

    public:
        CrtRenderer() {};
        virtual ~CrtRenderer();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        virtual void renderFrame() = 0;
        virtual EmuPixelData getPixelData();

        virtual void toggleRenderingMethod() {};
        virtual void toggleColorMode() {};
        virtual void toggleCropping() {};
        virtual void setCropping(bool) {};

        void attachSecondaryRenderer(CrtRenderer* renderer);

    protected:
        CrtRenderer* m_secondaryRenderer = nullptr;

        uint32_t* m_pixelData = nullptr;
        int m_sizeX = 0;
        int m_sizeY = 0;
        int m_bufSize = 0;
        double m_aspectRatio = 1.0;

        uint32_t* m_prevPixelData = nullptr;
        int m_prevSizeX = 0;
        int m_prevSizeY = 0;
        int m_prevBufSize = 0;
        double m_prevAspectRatio = 1.0;

        virtual bool isRasterPresent() {return true;};
        void swapBuffers();

    private:
        unsigned m_frameNo = 0;
};


class TextCrtRenderer : public CrtRenderer
{
    public:
        virtual ~TextCrtRenderer();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        void toggleRenderingMethod() override;
        void renderFrame() override;

        void setFontFile(std::string fontFileName);
        void setAltFontFile(std::string fontFileName);
        void setAltRender(bool isAltRender);

    protected:
        uint8_t* m_font = nullptr;
        uint8_t* m_altFont = nullptr;
        bool m_isAltRender = false;
        int m_fontSize;
        int m_altFontSize;

        virtual void primaryRenderFrame() = 0;
        virtual void altRenderFrame() = 0;
};

#endif // CRTRENDERER_H
