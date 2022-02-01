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

#include "sdlPalWindow.h"

using namespace std;

PalWindow::PalWindow()
{
    m_params.style = m_prevParams.style = PWS_FIXED;
    m_params.antialiasing = m_prevParams.antialiasing = false;
    m_params.width = m_prevParams.width = 800;
    m_params.height = m_prevParams.height = 600;
    m_params.visible = m_prevParams.visible = false;
    m_params.title = m_prevParams.title = "";

    m_lastX = SDL_WINDOWPOS_UNDEFINED;
    m_lastY = SDL_WINDOWPOS_UNDEFINED;
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
    if (m_params.style == PWS_SIZABLE)
        SDL_MaximizeWindow(m_window);
}


void PalWindow::applyParams()
{
    if (!m_window || m_params.style != m_prevParams.style)
        recreateWindow();
    else {
        if (m_params.vsync != m_prevParams.vsync)
            recreateRenderer();

        if (m_params.width != m_prevParams.width || m_params.height != m_prevParams.height)
            SDL_SetWindowSize(m_window, m_params.width, m_params.height);
    }

    //if (m_params.antialiasing != m_prevParams.antialiasing)
        //SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, m_params.antialiasing ? "2" : "0");

    if (m_params.visible != m_prevParams.visible)
        m_params.visible ? SDL_ShowWindow(m_window) : SDL_HideWindow(m_window);

    if (m_params.title != m_prevParams.title)
        SDL_SetWindowTitle(m_window, m_params.title.c_str());

    m_prevParams.style = m_params.style;
    m_prevParams.title = m_params.title;
    m_prevParams.visible = m_params.visible;
    m_prevParams.antialiasing = m_params.antialiasing;
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
        if (m_params.style == PWS_SIZABLE)
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
            if (m_params.style == PWS_SIZABLE) {
                w = m_lastWidth;
                h = m_lastHeight;
            }
        }
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
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

    Uint32 flags = 0;

    if (m_params.style == PWS_SIZABLE)
        flags |= SDL_WINDOW_RESIZABLE;
    else if (m_params.style == PWS_FULLSCREEN)
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    if (!m_params.visible)
        flags |= SDL_WINDOW_HIDDEN;

    m_window = SDL_CreateWindow(m_params.title.c_str(), x, y, w, h, flags);
    PalWindow::m_windowsMap.insert(make_pair(SDL_GetWindowID(m_window), this));

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
    uint8_t red = (color & 0xFF0000) >> 16;
    uint8_t green = (color & 0xFF00) >> 8;
    uint8_t blue = color & 0xFF;

    SDL_SetRenderDrawColor(m_renderer, red, green, blue, 0xff);
    SDL_RenderClear(m_renderer);
}


void PalWindow::drawImage(uint32_t* pixels, int imageWidth, int imageHeight, double aspectRatio, bool blend, bool useAlpha)
{
    int width, height;
    SDL_GetWindowSize(m_window, &width, &height);

    if (!blend && !useAlpha)
        calcDstRect(imageWidth, imageHeight, aspectRatio, width, height, m_dstWidth, m_dstHeight, m_dstX, m_dstY);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, m_params.antialiasing ? "2" : "0");

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


void PalWindow::drawEnd()
{
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


void PalWindow::screenshotRequest(const std::string& ssFileName)
{
    m_ssFileName = ssFileName;
}


map<uint32_t, PalWindow*> PalWindow::m_windowsMap;

PalWindow* PalWindow::windowById(uint32_t id)
{
    return PalWindow::m_windowsMap[id];
}
