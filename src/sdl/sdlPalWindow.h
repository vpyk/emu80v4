/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2019-2024
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
#include <GL/gl.h>

#include "../EmuTypes.h"
#include "../PalKeys.h"

class PalWindow
{
    public:

    enum PalWindowStyle {
        PWS_FIXED,
        PWS_RESIZABLE,
        PWS_FULLSCREEN
    };

    struct PalWindowParams {
        PalWindowStyle style;
        SmoothingType smoothing;
        bool vsync;
        bool visible;
        int width;
        int height;
        std::string title;
    };

        PalWindow();
        virtual ~PalWindow();
        void initPalWindow() {}

        static PalWindow* windowById(uint32_t id);
        void bringToFront();
        void maximize();
        void focusChanged(bool isFocused);

        virtual void mouseClick(int x, int y, PalMouseKey key) {}
        virtual void mouseDrag(int x, int y) = 0;

        virtual std::string getPlatformObjectName() = 0;
        EmuWindowType getWindowType() {return m_windowType;}
        virtual void calcDstRect(int srcWidth, int srcHeight,  double srcAspectRatio, int wndWidth, int wndHeight, int& dstWidth, int& dstHeight, int& dstX, int& dstY) = 0;

    protected:
        SDL_Window* m_window = nullptr;
        SDL_Renderer* m_renderer = nullptr;
        PalWindowParams m_params;
        //std::string m_title = ""; // убрать в private!

        void setTitle(const std::string& title);
        void getSize(int& width, int& height);
        void applyParams();

        void drawFill(uint32_t color);
        void drawImage(uint32_t* pixels, int imageWidth, int imageHeight, double aspectratio,
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

        int m_dstWidth = 0;
        int m_dstHeight = 0;
        int m_dstX = 0;
        int m_dstY = 0;

        SDL_Surface* m_ssSurface = nullptr;
        SDL_Renderer* m_ssRenderer = nullptr;
        std::string m_ssFileName = "";

        static std::map<uint32_t, PalWindow*> m_windowsMap;

        // OpenGL related
        bool m_glAvailable = true;
        SDL_GLContext m_glContext = NULL;
        GLuint m_VBO;
        GLuint m_program;

        void createGlContext();
        void drawFillGl(uint32_t color);
        void drawImageGl(uint32_t* pixels, int imageWidth, int imageHeight, double aspectratio,
                         bool blend = false, bool useAlpha = false);
        void drawEndGl();

        void paintGl();

        const int PROGRAM_VERTEX_ATTRIBUTE   = 0;
        const int PROGRAM_TEXCOORD_ATTRIBUTE = 1;

        const GLfloat c_vertices[16] = {
            1.0, -1.0,  1.0, 1.0,
           -1.0, -1.0,  0.0, 1.0,
           -1.0,  1.0,  0.0, 0.0,
            1.0,  1.0,  1.0, 0.0
        };

        const char* c_vShader = R"(
            attribute vec2 vertCoord;
            attribute vec2 texCoord;

            varying vec2 vTexCoord;
            varying vec2 prescale;

            uniform vec2 textureSize;
            uniform vec2 outputSize;
            uniform vec2 destSize;

            void main()
            {
                vec2 scale = destSize / outputSize;
                gl_Position = vec4(vertCoord * scale, 0.0, 1.0);
                vTexCoord = texCoord * textureSize;
                prescale = ceil(outputSize / textureSize);
            })";


        const char* c_fShader = R"(
            precision highp float;
            uniform sampler2D texture1;
            uniform vec2 textureSize;
            uniform bool sharp;

            varying vec2 vTexCoord;
            varying vec2 prescale;

            void main()
            {
                if (sharp) {
                    const /*mediump*/ vec2 halfp = vec2(0.5);
                    vec2 texel_floored = floor(vTexCoord);
                    vec2 s = fract(vTexCoord);
                    vec2 region_range = halfp - halfp / prescale;

                    vec2 center_dist = s - halfp;
                    vec2 f = (center_dist - clamp(center_dist, -region_range, region_range)) * prescale + halfp;

                    vec2 mod_texel = min(texel_floored + f, textureSize-halfp);
                    gl_FragColor = texture2D(texture1, mod_texel / textureSize);
                } else
                    gl_FragColor = texture2D(texture1, (vTexCoord + 0.002) / textureSize);
            })";

};


#endif // SDLPALWINDOW_H
