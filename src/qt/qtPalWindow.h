/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2018
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

#ifndef QTPALWINDOW_H
#define QTPALWINDOW_H

#include <string>
#include <map>

#include "../EmuTypes.h"
#include "../PalKeys.h"

class MainWindow;

class PalWindow
{
    public:

    enum PalWindowStyle {
        PWS_FIXED,
        PWS_SIZABLE,
        PWS_FULLSCREEN
    };

    struct PalWindowParams {
        PalWindowStyle style;
        bool antialiasing;
        bool vsync;
        bool visible;
        int width;
        int height;
        std::string title;
    };

        PalWindow();
        virtual ~PalWindow();
        void initPalWindow();

        static PalWindow* windowById(uint32_t id);
        void bringToFront();
        void maximize();
        void focusChanged(bool isFocused);

        virtual void mouseClick(int x, int y, PalMouseKey key); //{}

        virtual std::string getPlatformObjectName() = 0;
        virtual EmuWindowType getWindowType() {return m_windowType;}

        MainWindow* getQtWindow() {return m_qtWindow;}

    protected:
        PalWindowParams m_params;

        void setTitle(const std::string& title);
        void getSize(int& width, int& height);
        void applyParams();

        void drawFill(uint32_t color);
        void drawImage(uint32_t* pixels, int imageWidth, int imageHeight, int dstX, int dstY, int dstWidth, int dstHeight, bool blend = false, bool useAlpha = false);
        void drawEnd();
        void screenshotRequest(const std::string& ssFileName);

        EmuWindowType m_windowType = EWT_UNDEFINED;

    private:
        PalWindowParams m_prevParams;
        MainWindow* m_qtWindow = nullptr;

        static std::map<uint32_t, PalWindow*> m_windowsMap;
};

#endif // QTPALWINDOW_H
