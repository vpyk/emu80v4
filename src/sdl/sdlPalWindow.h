﻿/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2019-2025
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
        std::string shader;
        bool grayBackground;
        bool desaturate;
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

        SmoothingType m_smoothing = ST_SHARP;
        std::string m_shaderFileName;
        bool m_needToRecreateProgram = true;

        static std::map<uint32_t, PalWindow*> m_windowsMap;

        // OpenGL related
        bool m_glAvailable = true;
        SDL_GLContext m_glContext = NULL;
        GLuint m_VBO;
        GLuint m_program = 0;
        bool m_shaderValid = false;

        void createGlContext();
        void recreateProgramIfNeeded();
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

        const GLfloat c_mvpMatrix[16] = {
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0,
        };

        const char* c_vShader = R"(
            attribute vec2 VertexCoord;
            attribute vec2 TexCoord;

            varying vec2 vTexCoord;
            varying vec2 prescale;

            uniform vec2 TextureSize;
            uniform vec2 OutputSize;

            void main()
            {
                gl_Position = vec4(VertexCoord, 0.0, 1.0);
                vTexCoord = TexCoord * TextureSize;
                prescale = ceil(OutputSize / TextureSize);
            })";


        const char* c_fShader =
#ifdef PAL_WASM
         R"(precision highp float;
         )"
#endif
         R"(uniform sampler2D texture1;
            uniform vec2 TextureSize;
            uniform bool sharp;
            uniform bool grayscale;

            varying vec2 vTexCoord;
            varying vec2 prescale;

            float toGrayscale(vec3 rgb)
            {
                float r, g, b;
                if (rgb.r <= 0.04045) r = rgb.r / 12.92; else r = pow(((rgb.r + 0.055)/1.055), 2.4);
                if (rgb.g <= 0.04045) g = rgb.g / 12.92; else g = pow(((rgb.g + 0.055)/1.055), 2.4);
                if (rgb.b <= 0.04045) b = rgb.b / 12.92; else b = pow(((rgb.b + 0.055)/1.055), 2.4);
                float y = 0.212655 * r + 0.715158 * g + 0.072187 * b;
                if (y <= 0.0031308)
                    return y * 12.92;
                else
                    return 1.055 * pow(y, 1.0/2.4) - 0.055;
            }

            void main()
            {
                if (sharp) {
                    const vec2 halfp = vec2(0.5);
                    vec2 texel_floored = floor(vTexCoord);
                    vec2 s = fract(vTexCoord);
                    vec2 region_range = halfp - halfp / prescale;

                    vec2 center_dist = s - halfp;
                    vec2 f = (center_dist - clamp(center_dist, -region_range, region_range)) * prescale + halfp;

                    vec2 mod_texel = min(texel_floored + f, TextureSize - halfp);
                    gl_FragColor = texture2D(texture1, mod_texel / TextureSize);
                } else
                    gl_FragColor = texture2D(texture1, (vTexCoord + 0.002) / TextureSize);
                if (grayscale)
                    gl_FragColor = vec4(vec3(toGrayscale(gl_FragColor.rgb)), gl_FragColor.a);
            })";

};


#endif // SDLPALWINDOW_H
