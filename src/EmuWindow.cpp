/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2021
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

#include <sstream>
#include <iomanip>
#include <string.h>

#include "EmuWindow.h"
#include "Emulation.h"
#include "EmuConfig.h"
#include "Platform.h"

using namespace std;


EmuWindow::EmuWindow()
{
    m_windowType = EWT_EMULATION;

    m_curWindowWidth = m_defWindowWidth;
    m_curWindowHeight = m_defWindowHeight;

    m_params.style = PWS_FIXED;
    m_params.visible = false;
    m_params.width = m_defWindowWidth;
    m_params.height = m_defWindowHeight;
    m_params.title = "";
    m_params.vsync = g_emulation->getVsync();
    m_params.antialiasing = false;

    applyParams();
}

EmuWindow::~EmuWindow()
{
    if (m_interlacedImage)
        delete m_interlacedImage;
}


void EmuWindow::init()
{
    initPalWindow();
}


string EmuWindow::getPlatformObjectName()
{
    if (m_platform)
        return m_platform->getName();
    else
        return "";
}


void EmuWindow::setDefaultWindowSize(int width, int height)
{
    m_defWindowWidth = width;
    m_defWindowHeight = height;
    if (m_frameScale == FS_BEST_FIT || m_frameScale == FS_FIT || m_frameScale == FS_FIT_KEEP_AR) {
        m_params.width = m_defWindowWidth;
        m_params.height = m_defWindowHeight;
        applyParams();

        m_curWindowWidth = m_defWindowWidth;
        m_curWindowHeight = m_defWindowHeight;
    }
}


void EmuWindow::setFrameScale(FrameScale fs)
{
    m_frameScale = fs;
}



void EmuWindow::setCaption(string caption)
{
    m_caption = caption;
    string ver = VERSION;
    m_params.title = "Emu80 " + ver + ": " + caption;
    applyParams();
}

string EmuWindow::getCaption()
{
    return m_caption;
}


void EmuWindow::setWindowStyle(WindowStyle ws)
{
    m_isFullscreenMode = false;
    if (ws == WS_FIXED || ws == WS_AUTOSIZE)
        m_params.style = PWS_FIXED;
    else // if (ws == WS_SIZABLE)
        m_params.style = PWS_SIZABLE;

    if (ws == WS_FIXED) {
        m_params.width = m_defWindowWidth;
        m_params.height = m_defWindowHeight;
    }

    applyParams();

    m_windowStyle = ws;
}


void EmuWindow::setFullScreen(bool fullscreen)
{
    if (fullscreen) {
        m_params.style = PWS_FULLSCREEN;
    } else
        setWindowStyle(m_windowStyle);
    applyParams();

    m_isFullscreenMode = fullscreen;
}


// Переписать - запоминать предыдущий режим!
void EmuWindow::toggleFullScreen()
{
    setFullScreen(!m_isFullscreenMode);
}


void EmuWindow::setFieldsMixing(FieldsMixing fm)
{
    m_fieldsMixing = fm;
}


void EmuWindow::setAntialiasing(bool aal)
{
    m_isAntialiased = aal;
    m_params.antialiasing = aal;
    applyParams();
}


void EmuWindow::setAspectCorrection(bool aspectCorrection)
{
    m_aspectCorrection = aspectCorrection;
}


void EmuWindow::setWideScreen(bool wideScreen)
{
    if (!m_useCustomScreenFormat)
        m_wideScreen = wideScreen;
}


void EmuWindow::setCustomScreenFormat(bool custom)
{
    m_useCustomScreenFormat = custom;
}


void EmuWindow::setCustomScreenFormatValue(double format)
{
    m_customScreenFormat = format;
}


void EmuWindow::show()
{
    m_params.visible = true;
    applyParams();
}


void EmuWindow::hide()
{
    m_params.visible = false;
    applyParams();
}



void EmuWindow::calcDstRectP(EmuPixelData frame)
{
    m_curImgWidth = frame.width;
    m_curImgHeight = frame.height;
    getSize(m_curWindowWidth, m_curWindowHeight);

    calcDstRect(frame.width, frame.height, frame.aspectRatio, m_curWindowWidth, m_curWindowHeight, m_dstWidth, m_dstHeight, m_dstX, m_dstY);
}



void EmuWindow::calcDstRect(int srcWidth, int srcHeight,  double srcAspectRatio, int wndWidth, int wndHeight, int& dstWidth, int& dstHeight, int& dstX, int& dstY)
{
    //m_curImgWidth = frame.width;
    //m_curImgHeight = frame.height;

    double aspectRatio = 1.0;
    if (m_aspectCorrection)
        aspectRatio = m_useCustomScreenFormat ? srcAspectRatio * 0.75 * m_customScreenFormat : m_wideScreen ? srcAspectRatio / 0.75 : srcAspectRatio;

    FrameScale tempFs = m_frameScale;

    if (m_isFullscreenMode)
        tempFs = m_isAntialiased ? FS_FIT_KEEP_AR : FS_BEST_FIT;

    //getSize(m_curWindowWidth, m_curWindowHeight);

    switch(tempFs) {
        case FS_1X:
            dstWidth = srcWidth * aspectRatio + .5;
            dstHeight = srcHeight;
            dstX = (wndWidth - dstWidth) / 2;
            dstY = (wndHeight - dstHeight) / 2;
            break;
        case FS_2X:
            dstWidth = srcWidth * 2 * aspectRatio + .5;
            dstHeight = srcHeight * 2;
            dstX = (wndWidth - dstWidth) / 2;
            dstY = (wndHeight - dstHeight) / 2;
            break;
        case FS_3X:
            dstWidth = srcWidth * 3 * aspectRatio + .5;
            dstHeight = srcHeight * 3;
            dstX = (wndWidth - dstWidth) / 2;
            dstY = (wndHeight - dstHeight) / 2;
            break;
        case FS_4X:
            dstWidth = srcWidth * 4 * aspectRatio + .5;
            dstHeight = srcHeight * 4;
            dstX = (wndWidth - dstWidth) / 2;
            dstY = (wndHeight - dstHeight) / 2;
            break;
        case FS_5X:
            dstWidth = srcWidth * 5 * aspectRatio + .5;
            dstHeight = srcHeight * 5;
            dstX = (wndWidth - dstWidth) / 2;
            dstY = (wndHeight - dstHeight) / 2;
            break;
        case FS_2X3:
            dstWidth = srcWidth * (m_aspectCorrection ? 2 : 3) * aspectRatio + .5;
            dstHeight = srcHeight * 2;
            dstX = (wndWidth - dstWidth) / 2;
            dstY = (wndHeight - dstHeight) / 2;
            break;
        case FS_3X5:
            dstWidth = srcWidth * (m_aspectCorrection ? 3 : 5) * aspectRatio + .5;
            dstHeight = srcHeight * 3;
            dstX = (wndWidth - dstWidth) / 2;
            dstY = (wndHeight - dstHeight) / 2;
            break;
        case FS_4X6:
            dstWidth = srcWidth * (m_aspectCorrection ? 4 : 6) * aspectRatio + .5;
            dstHeight = srcHeight * 4;
            dstX = (wndWidth - dstWidth) / 2;
            dstY = (wndHeight - dstHeight) / 2;
            break;
        case FS_BEST_FIT: {
            int scaleX = 1;
            int scaleY = 1;
            if (aspectRatio == 1. && (m_frameScale == FS_2X3 || m_frameScale == FS_4X6)) {
                scaleX = 3;
                scaleY = 2;
            } else if (aspectRatio == 1. && m_frameScale == FS_3X5) {
                scaleX = 5;
                scaleY = 3;
            }

            int timesX = wndWidth / (srcWidth * scaleX * aspectRatio + .5);
            int timesY = wndHeight / (srcHeight * scaleY);

            timesX = timesX < timesY ? timesX * scaleX: timesY * scaleX;
            timesY = timesX < timesY ? timesX * scaleY: timesY * scaleY;

            if (timesX == 0 || timesY == 0) {
                dstWidth = wndWidth;
                dstHeight = wndHeight;
                dstX = 0;
                dstY = 0;
            } else {
                int dx = int((wndWidth - srcWidth * timesX * aspectRatio + .5)) / 2;
                int dy = (wndHeight - srcHeight * timesY) / 2;
                dstX = dx;
                dstY = dy;
                dstWidth = srcWidth * timesX * aspectRatio + .5;
                dstHeight = srcHeight * timesY;
            }
            break;
        }
        case FS_FIT:
            dstWidth = wndWidth;
            dstHeight = wndHeight;
            dstX = 0;
            dstY = 0;
            break;
        case FS_FIT_KEEP_AR:
            double ar = aspectRatio;
            if (aspectRatio == 1. && (m_frameScale == FS_2X3 || m_frameScale == FS_4X6))
                ar = ar * 3 / 2;
            else if (aspectRatio == 1 && m_frameScale == FS_3X5)
                ar = ar * 5 / 3;

        int newW = wndHeight * srcWidth * ar / srcHeight + .5;
            int newH = wndWidth * srcHeight / ar / srcWidth + .5;
            if (newW <= wndWidth) {
                dstWidth = newW;
                dstHeight = wndHeight;
                dstX = (wndWidth - newW) / 2;
                dstY = 0;
            } else {
                dstWidth = wndWidth;
                dstHeight = newH;
                dstX = 0;
                dstY = (wndHeight - newH) / 2;
            }
            break;
    }
}


bool EmuWindow::translateCoords(int& x, int& y)
{
    if ((x < m_dstX) || (x >= m_dstX + m_dstWidth) || (y < m_dstY) || (y >= m_dstY + m_dstHeight))
        return false;

    x = (x - m_dstX) * m_curImgWidth / m_dstWidth;
    y = (y - m_dstY) * m_curImgHeight / m_dstHeight;

    return true;
}


void EmuWindow::interlaceFields(EmuPixelData frame)
{
    int requiredSize = frame.height * 2 * frame.width;
    if (requiredSize > m_interlacedImageSize) {
        if (m_interlacedImage)
            delete m_interlacedImage;
        m_interlacedImage = new uint32_t[requiredSize];
        m_interlacedImageSize = requiredSize;
    }

    for (int i = 0; i < frame.height; i++) {
        memcpy(m_interlacedImage + frame.width * i * 2, frame.pixelData + i * frame.width, frame.width * 4);
        memcpy(m_interlacedImage + frame.width * (i * 2 + 1), frame.prevPixelData + i * frame.width, frame.width * 4);
    }
}


void EmuWindow::prepareScanline(EmuPixelData frame)
{
    int requiredSize = frame.height * 2 * frame.width;
    if (requiredSize > m_interlacedImageSize) {
        if (m_interlacedImage)
            delete m_interlacedImage;
        m_interlacedImage = new uint32_t[requiredSize];
        m_interlacedImageSize = requiredSize;
    }

    for (int i = 0; i < frame.height; i++) {
        memcpy(m_interlacedImage + frame.width * i * 2, frame.pixelData + i * frame.width, frame.width * 4);
        memcpy(m_interlacedImage + frame.width * (i * 2 + 1), frame.pixelData + i * frame.width, frame.width * 4);
        uint8_t* secondLine = (uint8_t*)(m_interlacedImage + frame.width * (i * 2 + 1));
        for (int x = 0; x < frame.width * 4; x += 4) {
            int r = secondLine[x]; secondLine[x] = r >> 2;
            int g = secondLine[x + 1]; secondLine[x + 1] = g >> 2;
            int b = secondLine[x + 2]; secondLine[x + 2] = b >> 2;
        }
    }
}


void EmuWindow::drawFrame(EmuPixelData frame)
{
    if (frame.width == 0 || frame.height == 0 || !frame.pixelData) {
        drawFill(0x303050);
        return;
    }

    calcDstRectP(frame);

    if ((m_windowStyle == WS_AUTOSIZE) && !m_isFullscreenMode
          && (m_frameScale == FS_1X || m_frameScale == FS_2X || m_frameScale == FS_3X || m_frameScale == FS_4X || m_frameScale == FS_5X || m_frameScale == FS_2X3 || m_frameScale == FS_3X5 || m_frameScale == FS_4X6)
          && (m_curWindowWidth != m_dstWidth || m_curWindowHeight != m_dstHeight)) {
        m_curWindowHeight = m_dstHeight;
        m_curWindowWidth = m_dstWidth;

        m_params.width = m_dstWidth;
        m_params.height = m_dstHeight;
        applyParams();
    }

    drawFill(0x282828);

    if (m_fieldsMixing != FM_INTERLACE && m_fieldsMixing != FM_SCANLINE) {
        drawImage((uint32_t*)frame.pixelData, frame.width, frame.height, frame.aspectRatio, false, false);
        if (m_fieldsMixing == FM_MIX && frame.prevPixelData && frame.prevHeight == frame.height && frame.prevWidth == frame.width) {
            drawImage((uint32_t*)frame.prevPixelData, frame.prevWidth, frame.prevHeight, frame.prevAspectRatio, true, false);
        }
    } else if (m_fieldsMixing == FM_INTERLACE && frame.prevPixelData && frame.prevHeight == frame.height && frame.prevWidth == frame.width) { // FM_INTERLACE
        if (frame.frameNo & 1)
            interlaceFields(frame);
        if (m_interlacedImage)
            drawImage(m_interlacedImage, frame.width, frame.height * 2, frame.aspectRatio, false, false);
    } else { // if (m_fieldsMixing == FM_SCANLINE)
        prepareScanline(frame);
        drawImage(m_interlacedImage, frame.width, frame.height * 2, frame.aspectRatio, false, false);
    }
}


void EmuWindow::drawOverlay(EmuPixelData frame)
{
    if (frame.width == 0 || frame.height == 0 || !frame.pixelData)
        return;

    //drawImage((uint32_t*)frame.pixelData, frame.width, frame.height, m_dstX, m_dstY, m_dstWidth, m_dstHeight, true, true);
    drawImage((uint32_t*)frame.pixelData, frame.width, frame.height, frame.aspectRatio, true, true);

/*    if (m_fieldsMixing == FM_MIX && frame.prevPixelData) {
        drawImage((uint32_t*)frame.prevPixelData, frame.prevWidth, frame.prevHeight, m_dstX, m_dstY, m_dstWidth, m_dstHeight, true, true);
    }*/
}


void EmuWindow::endDraw()
{
    drawEnd();
}


void EmuWindow::sysReq(SysReq sr)
{
    switch (sr) {
        case SR_FULLSCREEN:
            toggleFullScreen();
            break;
        case SR_1X:
            setWindowStyle(WS_AUTOSIZE);
            setFrameScale(FS_1X);
            setAntialiasing(false);
            m_aspectCorrection = false;
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_2X:
            setWindowStyle(WS_AUTOSIZE);
            setFrameScale(FS_2X);
            setAntialiasing(false);
            m_aspectCorrection = false;
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_3X:
            setWindowStyle(WS_AUTOSIZE);
            setFrameScale(FS_3X);
            setAntialiasing(false);
            m_aspectCorrection = false;
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_4X:
            setWindowStyle(WS_AUTOSIZE);
            setFrameScale(FS_4X);
            setAntialiasing(false);
            m_aspectCorrection = false;
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_5X:
            setWindowStyle(WS_AUTOSIZE);
            setFrameScale(FS_5X);
            setAntialiasing(false);
            m_aspectCorrection = false;
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_2X3:
            setWindowStyle(WS_AUTOSIZE);
            setFrameScale(FS_2X3);
            setAntialiasing(false);
            m_aspectCorrection = false;
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_3X5:
            setWindowStyle(WS_AUTOSIZE);
            setFrameScale(FS_3X5);
            setAntialiasing(false);
            m_aspectCorrection = false;
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_4X6:
            setWindowStyle(WS_AUTOSIZE);
            setFrameScale(FS_4X6);
            setAntialiasing(false);
            m_aspectCorrection = false;
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_FIT:
            if (!m_isFullscreenMode)
                setWindowStyle(WS_SIZABLE);
            setFrameScale(FS_FIT_KEEP_AR);
            setAntialiasing(true);
            m_aspectCorrection = true;
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_STRETCH:
            if (!m_isFullscreenMode)
                setWindowStyle(WS_SIZABLE);
            setFrameScale(FS_FIT);
            setAntialiasing(true);
            m_aspectCorrection = true;
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_MAXIMIZE:
            setWindowStyle(WS_SIZABLE);
            setFrameScale(FS_FIT_KEEP_AR);
            setAntialiasing(true);
            maximize();
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_ASPECTCORRECTION:
            m_aspectCorrection = !m_aspectCorrection;
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_WIDESCREEN:
            setWideScreen(!m_wideScreen);
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_ANTIALIASING:
            setAntialiasing(!m_isAntialiased);
            if (m_windowType == EWT_EMULATION)
                g_emulation->getConfig()->updateConfig();
            break;
        case SR_SCREENSHOT:
            screenshotRequest(palOpenFileDialog("Save screenshot",
                "BMP files (*.bmp)|*.bmp"
#ifdef PAL_QT
                "|PNG files (*.png)|*.png"
#endif
                , true, this));
            break;
        default:
            break;
    }
}


bool EmuWindow::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "caption") {
        setCaption(values[0].asString());
        return true;
    } else if (propertyName == "windowStyle") {
        if (values[0].asString() == "autosize") {
            setWindowStyle(WS_AUTOSIZE);
            return true;
        } else if (values[0].asString() == "fixed") {
            setWindowStyle(WS_FIXED);
            return true;
        } else if (values[0].asString() == "sizable") {
            setWindowStyle(WS_SIZABLE);
            return true;
        } else
            return false;
    } else if (propertyName == "frameScale") {
        if (values[0].asString() == "bestFit") {
            setFrameScale(FS_BEST_FIT);
            return true;
        } else if (values[0].asString() == "1x") {
            setFrameScale(FS_1X);
            return true;
        } else if (values[0].asString() == "2x") {
            setFrameScale(FS_2X);
            return true;
        } else if (values[0].asString() == "3x") {
            setFrameScale(FS_3X);
            return true;
        } else if (values[0].asString() == "4x") {
            setFrameScale(FS_4X);
            return true;
        } else if (values[0].asString() == "5x") {
            setFrameScale(FS_5X);
            return true;
        } else if (values[0].asString() == "2x3") {
            setFrameScale(FS_2X3);
            return true;
        } else if (values[0].asString() == "3x5") {
            setFrameScale(FS_3X5);
            return true;
        } else if (values[0].asString() == "4x6") {
            setFrameScale(FS_4X6);
            return true;
        } else if (values[0].asString() == "fit") {
            setFrameScale(FS_FIT);
            return true;
        } else if (values[0].asString() == "fitKeepAR") {
            setFrameScale(FS_FIT_KEEP_AR);
            return true;
        }
    } else if (propertyName == "fieldsMixing") {
        if (values[0].asString() == "none") {
            setFieldsMixing(FM_NONE);
            return true;
        } else if (values[0].asString() == "mix") {
            setFieldsMixing(FM_MIX);
            return true;
        } else if (values[0].asString() == "interlace") {
            setFieldsMixing(FM_INTERLACE);
            return true;
        } else if (values[0].asString() == "scanline") {
            setFieldsMixing(FM_SCANLINE);
            return true;
        }
    } else if (propertyName == "defaultWindowSize") {
        if (values[0].isInt() && values[1].isInt()) {
            setDefaultWindowSize(values[0].asInt(), values[1].asInt());
            return true;
        }
    } else if (propertyName == "defaultWindowWidth") { // !!!
        if (values[0].isInt()) {
            setDefaultWindowSize(values[0].asInt(), m_defWindowHeight);
            return true;
        }
    } else if (propertyName == "defaultWindowHeight") { // !!!
        if (values[0].isInt()) {
            setDefaultWindowSize(m_defWindowWidth, values[0].asInt());
            return true;
        }
    } else if (propertyName == "antialiasing") {
        if (values[0].asString() == "yes") {
            setAntialiasing(true);
            return true;
        } else if (values[0].asString() == "no") {
            setAntialiasing(false);
            return true;
        }
    } else if (propertyName == "fullscreen") {
        if (values[0].asString() == "yes") {
            setFullScreen(true);
            return true;
        } else if (values[0].asString() == "no") {
            setFullScreen(false);
            return true;
        }
    } else if (propertyName == "aspectCorrection") {
        if (values[0].asString() == "no") {
            setAspectCorrection(false);
            return true;
        } else if (values[0].asString() == "yes") {
            setAspectCorrection(true);
            return true;
        }
    } else if (propertyName == "wideScreen") {
        if (values[0].asString() == "no") {
            setCustomScreenFormat(false);
            setWideScreen(false);
            return true;
        } else if (values[0].asString() == "yes") {
            setCustomScreenFormat(false);
            setWideScreen(true);
            return true;
        } else if (values[0].asString() == "custom") {
            setCustomScreenFormat(true);
            return true;
        }
    } else if (propertyName == "customScreenFormat") {
        if (values[0].isFloat()) {
            setCustomScreenFormatValue(values[0].asFloat());
            return true;
        }
    }

    return false;
}


string EmuWindow::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "caption")
        return m_caption;
    else if (propertyName == "windowStyle") {
        switch (m_windowStyle) {
            case WS_AUTOSIZE:
                return "autosize";
            case WS_FIXED:
                return "fixed";
            case WS_SIZABLE:
                return "sizable";
        }
    } else if (propertyName == "frameScale") {
        switch (m_frameScale) {
            case FS_BEST_FIT:
                return "bestFit";
            case FS_1X:
                return "1x";
            case FS_2X:
                return "2x";
            case FS_3X:
                return "3x";
            case FS_4X:
                return "4x";
            case FS_5X:
                return "5x";
            case FS_2X3:
                return "2x3";
            case FS_3X5:
                return "3x5";
            case FS_4X6:
                return "4x6";
            case FS_FIT:
                return "fit";
            case FS_FIT_KEEP_AR:
                return "fitKeepAR";
        }

    } else if (propertyName == "fieldsMixing") {
        switch (m_fieldsMixing) {
            case FM_NONE:
                return "none";
            case FM_MIX:
                return "mix";
            case FM_INTERLACE:
                return "interlace";
            case FM_SCANLINE:
                return "scanline";
        }
    } else if (propertyName == "antialiasing") {
        return m_isAntialiased ? "yes" : "no";
    } else if (propertyName == "aspectCorrection") {
        return m_aspectCorrection ? "yes" : "no";
    } else if (propertyName == "wideScreen") {
        return m_useCustomScreenFormat ? "custom" : m_wideScreen ? "yes" : "no";
    } else if (propertyName == "defaultWindowWidth") {
        string res;
        stringstream stringStream;
        stringStream << m_defWindowWidth;
        stringStream >> res;
        return res;
    } else if (propertyName == "defaultWindowHeight") {
        string res;
        stringstream stringStream;
        stringStream << m_defWindowHeight;
        stringStream >> res;
        return res;
    } else if (propertyName == "customScreenFormat") {
        string res;
        stringstream stringStream;
        stringStream << setprecision(4) << m_customScreenFormat;
        stringStream >> res;
        return res;
    }

    return "";
}
