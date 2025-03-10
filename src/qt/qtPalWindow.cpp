/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2025
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

#include "qtPalWindow.h"
#include "qtMainWindow.h"
#include "qtPaintWidget.h"

using namespace std;


static MainWindow* qtWindow = nullptr;

PalWindow::PalWindow()
{
    m_params.style = m_prevParams.style = PWS_RESIZABLE;
    m_params.smoothing = m_prevParams.smoothing = ST_SHARP;
    m_params.width = m_prevParams.width = 0;
    m_params.height = m_prevParams.height = 0;
    m_params.visible = m_prevParams.visible = false;
    m_params.title = m_prevParams.title = "";
    m_params.grayBackground = m_prevParams.grayBackground = false;
    m_params.desaturate = m_prevParams.desaturate = false;
}


PalWindow::~PalWindow()
{
    if (PalWindow::getWindowType() == EWT_DEBUG)
        delete m_qtWindow;
    else
        m_qtWindow->setPalWindow(nullptr);
}


void PalWindow::initPalWindow()
{
    if (!m_qtWindow) {
        if (!qtWindow)
            qtWindow = new MainWindow();
        if (getWindowType() == EWT_DEBUG)
            m_qtWindow = new MainWindow();
        else
            m_qtWindow = qtWindow;

        // default parameters
        m_qtWindow->setClientSize(0, 0);
        m_qtWindow->getPaintWidget()->setSmoothingAndShaderFile(ST_SHARP, "");
        m_qtWindow->setPalWindow(this);
    }
}


void PalWindow::getSize(int& width, int& height)
{
    width = m_qtWindow->getPaintWidget()->width();
    height = m_qtWindow->getPaintWidget()->height();
}


void PalWindow::bringToFront()
{
    m_qtWindow->activateWindow();
    m_qtWindow->raise();
}


void PalWindow::maximize()
{
    // not used in Qt
}


void PalWindow::applyParams()
{
    if (!m_qtWindow)
        return;

    if (m_params.style != PWS_FULLSCREEN && m_prevParams.style == PWS_FULLSCREEN) {
        m_qtWindow->setFullScreen(false);
    }

    if (m_params.style == PWS_FIXED && (m_params.width != m_prevParams.width || m_params.height != m_prevParams.height))
        m_qtWindow->setClientSize(m_params.width, m_params.height);

    if (m_params.style != m_prevParams.style) {
        switch (m_params.style) {
        case PWS_RESIZABLE:
            m_qtWindow->setClientSize(0, 0);
            //m_qtWindow->adjustClientSize();
            break;
        case PWS_FIXED:
            m_qtWindow->setClientSize(m_params.width, m_params.height);
            break;
        default: // case PWS_FULLSCREEN
            m_qtWindow->setFullScreen(true);
            break;
        }
    }

    if (m_params.title != m_prevParams.title)
        m_qtWindow->setWindowTitle(m_params.title.c_str());

    if (m_params.smoothing != m_prevParams.smoothing || m_params.shader != m_prevParams.shader)
        m_qtWindow->setSmoothingAndShader(m_params.smoothing, m_params.shader);

    if (m_params.desaturate != m_prevParams.desaturate)
        m_qtWindow->getPaintWidget()->setDesaturate(m_params.desaturate);

    if (m_params.visible != m_prevParams.visible)
        m_params.visible ? m_qtWindow->showWindow() : m_qtWindow->hideWindow();

    m_prevParams.style = m_params.style;
    m_prevParams.title = m_params.title;
    m_prevParams.visible = m_params.visible;
    m_prevParams.smoothing = m_params.smoothing;
    m_prevParams.shader = m_params.shader;
    m_prevParams.desaturate = m_params.desaturate;
    //m_prevParams.vsync = m_params.vsync;
    if (m_params.style != PWS_FULLSCREEN) {
        m_prevParams.width = m_params.width;
        m_prevParams.height = m_params.height;
    }

}


void PalWindow::focusChanged(bool)
{
    // not used
}


void PalWindow::drawFill(uint32_t color)
{
    uint8_t red = (color & 0xFF0000) >> 16;
    uint8_t green = (color & 0xFF00) >> 8;
    uint8_t blue = color & 0xFF;

    if (m_qtWindow)
        m_qtWindow->getPaintWidget()->colorFill(QColor(red, green, blue));
}


void PalWindow::drawImage(uint32_t* pixels, int imageWidth, int imageHeight, double aspectRatio, bool blend, bool useAlpha)
{
    if (m_qtWindow)
        m_qtWindow->getPaintWidget()->drawImage(pixels, imageWidth, imageHeight, aspectRatio, blend, useAlpha);
}


void PalWindow::drawEnd()
{
    m_qtWindow->getPaintWidget()->update();
}


void PalWindow::screenshotRequest(const string& ssFileName)
{
    QString fileName = QString::fromUtf8(ssFileName.c_str());
    m_qtWindow->getPaintWidget()->screenshot(fileName);
}


void PalWindow::mouseClick(int, int, PalMouseKey) {}


void PalWindow::mouseDrag(int, int) {}


map<uint32_t, PalWindow*> PalWindow::m_windowsMap;

PalWindow* PalWindow::windowById(uint32_t id)
{
    return PalWindow::m_windowsMap[id];
}
