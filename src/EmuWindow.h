/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2022
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

#ifndef EMUWINDOW_H
#define EMUWINDOW_H

#include <string>
#include <map>

#include "Pal.h"
#include "PalWindow.h"

#include "EmuTypes.h"
#include "EmuObjects.h"


class Emulation;

class PalWindow;

enum FrameScale {
    FS_BEST_FIT,
    FS_FIXED,
    FS_FIT,
};

enum FieldsMixing {
    FM_NONE,
    FM_MIX,
    FM_INTERLACE,
    FM_SCANLINE
};

enum WindowStyle {
    WS_AUTOSIZE,
    WS_FIXED,
    WS_RESIZABLE
};


class EmuWindow : public EmuObject, public PalWindow
{
    public:
        EmuWindow();
        virtual ~EmuWindow();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        void init() override;

        void calcDstRect(int srcWidth, int srcHeight, double srcAspectRatio, int wndWidth, int wndHeight, int& dstWidth, int& dstHeight, int& dstX, int& dstY) override;

        virtual void processKey(PalKeyCode, bool) {}
        virtual void closeRequest() {}

        void sysReq(SysReq sr);

        void show();
        void hide();
        void setDefaultWindowSize(int width, int height);
        void setCaption(std::string caption);
        void setFrameScale(FrameScale fs);
        void setFixedYScale(int yScale);
        void setFieldsMixing(FieldsMixing fm);
        void setWindowStyle(WindowStyle ws);
        void setSmoothing(SmoothingType smoothing);
        void setFullScreen(bool fullscreen);
        void setAspectCorrection(bool aspectCorrection);
        void setWideScreen(bool wideScreen);
        void setCustomScreenFormat(bool custom);
        void setCustomScreenFormatValue(double format);
        void setSquarePixels(bool squarePixels);
        void toggleFullScreen();

        void drawFrame(EmuPixelData frame);
        void drawOverlay(EmuPixelData frame);
        void endDraw();

        bool translateCoords(int& x, int& y);

        std::string getCaption();

        std::string getPlatformObjectName() override;

        static EmuObject* create(const EmuValuesList&) {return new EmuWindow();}

    private:
        int m_defWindowWidth = 800;
        int m_defWindowHeight = 600;

        int m_curWindowWidth;
        int m_curWindowHeight;

        bool m_isFullscreenMode = false;
        bool m_aspectCorrection = false;
        bool m_wideScreen = false;
        bool m_useCustomScreenFormat = false;
        double m_customScreenFormat = 5. / 3.;
        bool m_squarePixels = false;

        std::string m_caption = "";

        FrameScale m_frameScale = FS_BEST_FIT;
        FieldsMixing m_fieldsMixing = FM_NONE;
        WindowStyle m_windowStyle = WS_AUTOSIZE;
        SmoothingType m_smoothing = ST_SHARP;
        int m_scaleY;

        int m_curImgWidth = 0;
        int m_curImgHeight = 0;

        int m_dstX;
        int m_dstY;
        int m_dstWidth;
        int m_dstHeight;

        void interlaceFields(EmuPixelData frame);
        void prepareScanline(EmuPixelData frame);

        uint32_t* m_interlacedImage = nullptr;
        int m_interlacedImageSize = 0;

        double calcBestAspectRatio(double srcAspectRatio, int scaleY);
};

#endif // EMUWINDOW_H
