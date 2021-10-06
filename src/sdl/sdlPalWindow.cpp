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

#include "sdlPalWindow.h"
#include "sdlGlExt.h"

#ifdef PAL_WASM
#include "../wasm/wasmEmuCalls.h"
#endif

 #include "../lite/litePal.h"

using namespace std;

PalWindow::PalWindow()
{
    m_params.style = m_prevParams.style = PWS_FIXED;
    m_params.smoothing = m_prevParams.smoothing = ST_SHARP;
    m_params.width = m_prevParams.width = 800;
    m_params.height = m_prevParams.height = 600;
    m_params.visible = m_prevParams.visible = false;
    m_params.title = m_prevParams.title = "";

    m_lastX = SDL_WINDOWPOS_UNDEFINED;
    m_lastY = SDL_WINDOWPOS_UNDEFINED;

#ifdef PAL_WASM
    wasmSetWindowId(this);
#endif
}


PalWindow::~PalWindow()
{
    if (m_renderer)
        SDL_DestroyRenderer(m_renderer);
    if (m_window) {
        PalWindow::m_windowsMap.erase(SDL_GetWindowID(m_window));
        SDL_DestroyWindow(m_window);
    }
}


/*void PalWindow::setTitle(const string& title)
{
    m_title = title;
    SDL_SetWindowTitle(m_window, m_title.c_str());
}*/


void PalWindow::getSize(int& width, int& height)
{
    SDL_GetWindowSize(m_window, &width, &height);
}


void PalWindow::bringToFront()
{
    SDL_RaiseWindow(m_window);
}


void PalWindow::maximize()
{
    if (m_params.style == PWS_RESIZABLE)
        SDL_MaximizeWindow(m_window);
}


void PalWindow::applyParams()
{
    if (!m_window)
        recreateWindow();

    else if (m_params.style != m_prevParams.style)
#ifndef PAL_WASM
        recreateWindow();
#else  // PAL_WASM
        SDL_SetWindowFullscreen(m_window, m_params.style == PWS_FULLSCREEN ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
#endif // !PAL_WASM

    if (m_params.vsync != m_prevParams.vsync)
        recreateRenderer();

    if (m_params.width != m_prevParams.width || m_params.height != m_prevParams.height) {
        SDL_SetWindowSize(m_window, m_params.width  != 0 && m_params.width > 100 ? m_params.width : 100,
                                    m_params.height != 0 && m_params.height > 75 ? m_params.height : 75);
    }

    if (m_params.visible != m_prevParams.visible)
        m_params.visible ? SDL_ShowWindow(m_window) : SDL_HideWindow(m_window);

    if (m_params.title != m_prevParams.title)
        SDL_SetWindowTitle(m_window, m_params.title.c_str());

    m_prevParams.style = m_params.style;
    m_prevParams.title = m_params.title;
    m_prevParams.visible = m_params.visible;
    m_prevParams.smoothing = m_params.smoothing;
    m_prevParams.vsync = m_params.vsync;
    if (m_params.style != PWS_FULLSCREEN) {
        m_prevParams.width = m_params.width;
        m_prevParams.height = m_params.height;
    }
}


void PalWindow::recreateWindow()
{
    int x = SDL_WINDOWPOS_UNDEFINED;
    int y = SDL_WINDOWPOS_UNDEFINED;
    int w = m_params.width;
    int h = m_params.height;

    SDL_Rect displayBounds;

    if (m_window) {
        if (m_params.style == PWS_RESIZABLE)
            SDL_GetWindowSize(m_window, &w, &h);
        if (m_prevParams.style != PWS_FULLSCREEN) {
            SDL_GetWindowPosition(m_window, &x, &y);
            m_lastX = x;
            m_lastY = y;
            m_lastWidth = w;
            m_lastHeight = h;
        } else {
            x = m_lastX;
            y = m_lastY;
            if (m_params.style == PWS_RESIZABLE) {
                w = m_lastWidth;
                h = m_lastHeight;
            }
        }

        if (m_renderer)
            SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;

        if (m_glContext)
            SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;

        SDL_GetDisplayBounds(SDL_GetWindowDisplayIndex(m_window), &displayBounds);
        PalWindow::m_windowsMap.erase(SDL_GetWindowID(m_window));
        SDL_DestroyWindow(m_window);
    }

    if (m_params.style == PWS_FULLSCREEN) {
        x = displayBounds.x;
        y = displayBounds.y;
        w = displayBounds.w;
        h = displayBounds.h;
    }

    Uint32 flags = SDL_WINDOW_OPENGL;

    if (m_params.style == PWS_RESIZABLE)
        flags |= SDL_WINDOW_RESIZABLE;
    else if (m_params.style == PWS_FULLSCREEN)
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    if (!m_params.visible)
        flags |= SDL_WINDOW_HIDDEN;


    m_window = SDL_CreateWindow(m_params.title.c_str(), x, y, w, h, flags);
    PalWindow::m_windowsMap.insert(make_pair(SDL_GetWindowID(m_window), this));

    createGlContext();

    if (!m_glAvailable)
        recreateRenderer();
}


void PalWindow::recreateRenderer()
{
    if (m_renderer)
        SDL_DestroyRenderer(m_renderer);
    m_renderer = SDL_CreateRenderer(m_window, -1, m_params.vsync ? SDL_RENDERER_PRESENTVSYNC : 0);
}


void PalWindow::focusChanged(bool /*isFocused*/)
{
    /*
    if (m_isFocused != isFocused) {
        m_params.vsync = g_emulation->getVsync();
        applyParams();
    }

    // если устанвлен фокус, убираем фокус у всех остальных окон
    if (isFocused)
        for (auto it = m_windowsMap.begin(); it != m_windowsMap.end(); it++) {
            if (it->second != this)
                it->second->focusChanged(false);
        }
    */
}


void PalWindow::drawFill(uint32_t color)
{
    if (m_glAvailable) {
        drawFillGl(color);
        return;
    }

    uint8_t red = (color & 0xFF0000) >> 16;
    uint8_t green = (color & 0xFF00) >> 8;
    uint8_t blue = color & 0xFF;

    SDL_SetRenderDrawColor(m_renderer, red, green, blue, 0xff);
    SDL_RenderClear(m_renderer);
}


void PalWindow::drawFillGl(uint32_t color)
{
    SDL_GL_MakeCurrent(m_window, m_glContext);

    uint8_t red = (color & 0xFF0000) >> 16;
    uint8_t green = (color & 0xFF00) >> 8;
    uint8_t blue = color & 0xFF;

    glClearColor(red / 255.0f, green / 255.0f, blue / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}


void PalWindow::drawImage(uint32_t* pixels, int imageWidth, int imageHeight, double aspectRatio, bool blend, bool useAlpha)
{
    if (m_glAvailable) {
        drawImageGl(pixels, imageWidth, imageHeight, aspectRatio, blend, useAlpha);
        return;
    }

    int width, height;
    SDL_GetWindowSize(m_window, &width, &height);

    if (!blend && !useAlpha)
        calcDstRect(imageWidth, imageHeight, aspectRatio, width, height, m_dstWidth, m_dstHeight, m_dstX, m_dstY);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, m_params.smoothing != ST_NEAREST ? "2" : "0");

    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(pixels, imageWidth, imageHeight,
                                                    32, imageWidth * 4, 0x00FF0000, 0x0000FF00, 0x000000FF, useAlpha ? 0xFF000000 : 0);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_SetTextureBlendMode(texture, blend ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
    if (blend && !useAlpha)
        SDL_SetTextureAlphaMod(texture, 0x80);
    SDL_Rect dstRect = {m_dstX, m_dstY, m_dstWidth, m_dstHeight};
    SDL_RenderCopy(m_renderer, texture, NULL, &dstRect);

    // Screenshot processing
    int ssWidth = imageWidth * 2;
    int ssHeight = imageHeight < 416 ? imageHeight * 2 : imageHeight; // interlace mode workaround, should pass this info separately
    if (m_ssFileName != "" && !m_ssSurface) {
        m_ssSurface = SDL_CreateRGBSurface(0, ssWidth, ssHeight, 24, 0x00FF0000, 0x0000FF00, 0x000000FF, 0);
        m_ssRenderer = SDL_CreateSoftwareRenderer(m_ssSurface);
    }
    if (m_ssSurface) {
        SDL_Texture* ssTexture = SDL_CreateTextureFromSurface(m_ssRenderer, surface);
        SDL_SetTextureBlendMode(ssTexture, blend ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
        if (blend && !useAlpha)
            SDL_SetTextureAlphaMod(ssTexture, 0x80);
        SDL_Rect ssRect = {0, 0, ssWidth, ssHeight};
        SDL_RenderCopy(m_ssRenderer, ssTexture, NULL, &ssRect);
    }

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}


void PalWindow::drawImageGl(uint32_t* pixels, int imageWidth, int imageHeight, double aspectRatio, bool blend, bool useAlpha)
{
    SDL_GL_MakeCurrent(m_window, m_glContext);

    int outWidth, outHeight;
    SDL_GetWindowSize(m_window, &outWidth, &outHeight);

    if (!blend && !useAlpha)
        calcDstRect(imageWidth, imageHeight, aspectRatio, outWidth, outHeight, m_dstWidth, m_dstHeight, m_dstX, m_dstY);

    glViewport(0, 0, outWidth, outHeight);

    int sharpLocation = glGetUniformLocation(m_program, "sharp");
    glUniform1i(sharpLocation, m_params.smoothing == ST_SHARP);

    int texSizeLocation = glGetUniformLocation(m_program, "textureSize");
    glUniform2f(texSizeLocation, float(imageWidth), float(imageHeight));

    int outSizeLocation = glGetUniformLocation(m_program, "outputSize");
    glUniform2f(outSizeLocation, float(outWidth), float(outHeight));

    int dstSizeLocation = glGetUniformLocation(m_program, "destSize");
    glUniform2f(dstSizeLocation, float(m_dstWidth), float(m_dstHeight));

    GLuint texture;
    glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_params.smoothing == ST_NEAREST ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_params.smoothing == ST_NEAREST ? GL_NEAREST : GL_LINEAR);

#ifndef PAL_WASM
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
#else // PAL_WASM
    size_t bitmapSize = imageWidth * imageHeight;
    uint8_t* rgbPixels = new uint8_t[bitmapSize * sizeof(uint32_t)];
    uint8_t* src = reinterpret_cast<uint8_t*>(pixels);
    uint8_t* dst = rgbPixels + 2;
    for (int i = 0; i < bitmapSize; i++) {
        *dst-- = *src++;
        *dst-- = *src++;
        *dst = *src++;
        dst += 3;
        *dst = *src++;
        dst += 3;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbPixels);
    delete[] rgbPixels;
#endif
    if (!blend && !useAlpha) {
        glDisable(GL_BLEND);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    } else if (useAlpha) {// || blend)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    } else if (blend) {
        glEnable(GL_BLEND);
        glBlendColor(0.0f, 0.0f, 0.0f, 0.5f);
        glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }

    glDeleteTextures(1, &texture);
}


void PalWindow::drawEnd()
{
    if (m_glAvailable) {
        drawEndGl();
        return;
    }

    SDL_RenderPresent(m_renderer);

    // Screenshot
    if (m_ssSurface) {
        SDL_RenderPresent(m_ssRenderer);
        SDL_SaveBMP(m_ssSurface, m_ssFileName.c_str());
        SDL_DestroyRenderer(m_ssRenderer);
        SDL_FreeSurface(m_ssSurface);
        m_ssFileName = "";
        m_ssSurface = nullptr;
        m_ssRenderer = nullptr;
    }
}


void PalWindow::drawEndGl()
{
    //SDL_GL_MakeCurrent(m_window, m_glContext);

    SDL_GL_SwapWindow(m_window);

    if (!m_ssFileName.empty()) {
        int w, h;
        SDL_GetWindowSize(m_window, &w, &h);
        uint8_t* pixels = new uint8_t[w * h * 3];

        int scrW = min(w, m_dstWidth);
        int scrH = min(h, m_dstHeight);

        int scrX = max(m_dstX, 0);
        int scrY = max(m_dstY, 0);

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

        // flip vertical and crop while creating surface
        SDL_Surface* bitmap = SDL_CreateRGBSurfaceWithFormatFrom(pixels + ((scrY + scrH - 1) * w + scrX) * 3, scrW, scrH, 24, -w * 3, SDL_PIXELFORMAT_RGB24);
        SDL_SaveBMP(bitmap, m_ssFileName.c_str());

        delete[] pixels;
        m_ssFileName = "";
    }
}


void PalWindow::screenshotRequest(const std::string& ssFileName)
{
    m_ssFileName = ssFileName;
}


void PalWindow::createGlContext()
{
    if (m_glContext)
        SDL_GL_DeleteContext(m_glContext);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    m_glContext = SDL_GL_CreateContext(m_window);

    if (!m_glContext) {
        m_glAvailable = false;
        return;
    }

    if (!sdlInitGLExtensions())
    {
        m_glAvailable = false;
        SDL_GL_DeleteContext(m_glContext);
        return;
    }

    SDL_GL_SetSwapInterval(m_params.vsync ? 1 : 0);

    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(c_vertices), c_vertices, GL_STATIC_DRAW);

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &c_vShader, NULL);
    glCompileShader(vertex);

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &c_fShader, NULL);
    glCompileShader(fragment);

    m_program = glCreateProgram();
    glAttachShader(m_program, vertex);
    glAttachShader(m_program, fragment);

    glBindAttribLocation(m_program, PROGRAM_VERTEX_ATTRIBUTE, "vertCoord");
    glBindAttribLocation(m_program, PROGRAM_TEXCOORD_ATTRIBUTE, "texCoord");

    glLinkProgram(m_program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    glUseProgram(m_program);

    glVertexAttribPointer(PROGRAM_VERTEX_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
    glEnableVertexAttribArray(PROGRAM_VERTEX_ATTRIBUTE);

    glVertexAttribPointer(PROGRAM_TEXCOORD_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(PROGRAM_TEXCOORD_ATTRIBUTE);

    return;
}


map<uint32_t, PalWindow*> PalWindow::m_windowsMap;

PalWindow* PalWindow::windowById(uint32_t id)
{
    return PalWindow::m_windowsMap[id];
}
