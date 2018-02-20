/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2018
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

#ifndef SDLPALWINDOW_H
#define SDLPALWINDOW_H

#include <string>
#include <map>

#include <SDL2/SDL.h>

#include "../EmuTypes.h"

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

        static PalWindow* windowById(uint32_t id);
        void bringToFront();
        void maximize();
        void focusChanged(bool isFocused);

        virtual std::string getPlatformObjectName() = 0;
        EmuWindowType getWindowType() {return m_windowType;};

    protected:
        SDL_Window* m_window = nullptr;
        SDL_Renderer* m_renderer = nullptr;
        PalWindowParams m_params;
        //std::string m_title = ""; // убрать в private!

        void setTitle(const std::string& title);
        void getSize(int& width, int& height);
        void applyParams();

        void drawFill(uint32_t color);
        void drawImage(uint32_t* pixels, int imageWidth, int imageHeight, int dstX, int dstY, int dstWidth, int dstHeight,
                       bool blend = false, bool useAlpha = false);
        void drawEnd();
        void screenshotRequest(const std::string& ssFileName);

        EmuWindowType m_windowType = EWT_UNDEFINED;

    private:
        void recreateWindow();
        void recreateRenderer();

        PalWindowParams m_prevParams;
        int m_lastX;
        int m_lastY;
        int m_lastWidth;
        int m_lastHeight;

        SDL_Surface* m_ssSurface = nullptr;
        SDL_Renderer* m_ssRenderer = nullptr;
        std::string m_ssFileName = "";

        static std::map<uint32_t, PalWindow*> m_windowsMap;
};

#endif // SDLPALWINDOW_H
