/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2019
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

#include <QPainter>
#include <QOpenGLContext>
#include <QPaintEvent>
//#include <QMessageBox>
#include <QGuiApplication>
#include <QClipboard>

#include "qtPaintWidget.h"


PaintWidget::PaintWidget(QWidget *parent) :
    QOpenGLWidget(parent)
{
    setAttribute(Qt::WA_NoSystemBackground);
    //setAttribute(Qt::WA_OpaquePaintEvent);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_dstRect.setRect(0, 0, width(), height()); // на всякий случай

    m_hideCursorTimer.setInterval(3500);
    connect(&m_hideCursorTimer, SIGNAL(timeout()), this, SLOT(onHideCursorTimer()));
    m_hideCursorTimer.start();
    setMouseTracking(true);
}


PaintWidget::~PaintWidget()
{
    if (m_image)
        delete m_image;
    if (m_image2)
        delete m_image2;
}


void PaintWidget::draw()
{
    m_needPaint = true;
    //repaint();
    update();
}

// Save screenshot or copy it to clipboard if filename is empty
void PaintWidget::screenshot(const QString& ssFileName)
{
    /*QImage fullImg = grabFramebuffer();
    QImage img = fullImg.copy(m_dstRect);*/

    if (!m_image) // to be on the safe side
        return;

    int width = m_dstRect.width();
    int height = m_dstRect.height();

    QImage img(width, height, QImage::Format_RGB32);
    QPainter painter;
    painter.begin(&img);
    painter.fillRect(QRect(0, 0, width, height), m_fillColor);
    paintScreen(&painter, QRect(0, 0, width, height));
    painter.end();

    if (ssFileName != "")
        img.save(ssFileName);
    else
        QGuiApplication::clipboard()->setImage(img);

    /*if (!img.save(ssFileName))
    {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Emu80");
        msgBox.setText(tr("Error saving screenshot!"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.addButton(QMessageBox::Ok);
        msgBox.exec();
    }*/
}


void PaintWidget::colorFill(QColor color)
{
    m_fillColor = color;
    if (m_image) {
        delete m_image;
        delete m_imageData;
        m_image = nullptr;
    }
}


void PaintWidget::drawImage(uint32_t* pixels, int imageWidth, int imageHeight, double aspectRatio, bool blend, bool useAlpha)
{
    int dstWidth, dstHeight, dstX, dstY;
    static_cast<MainWindow*>(parent())->getPalWindow()->calcDstRect(imageWidth, imageHeight, aspectRatio, width(), height(), dstWidth, dstHeight, dstX, dstY);

    m_dstRect.setRect(dstX, dstY, dstWidth, dstHeight);

    if (m_image2) {
        delete m_image2;
        m_image2 = nullptr;
        delete m_imageData2;
    }

    if (useAlpha || blend) {
        m_imageData2 = new uchar[imageWidth * imageHeight * 4];
        memcpy(m_imageData2, pixels, imageWidth * imageHeight * 4);
        m_image2 = new QImage(m_imageData2, imageWidth, imageHeight, useAlpha ? QImage::Format_ARGB32 : QImage::Format_RGB32);
        m_img2aspectRatio = aspectRatio;
        if (!useAlpha)
            *m_image2 = m_image2->convertToFormat(QImage::Format_ARGB32); // add alpha channel
        m_useAlpha = useAlpha;
        return;
    }

    if (m_image) {
        delete m_image;
        delete m_imageData;
    }
    m_imageData = new uchar[imageWidth * imageHeight * 4];
    memcpy(m_imageData, pixels, imageWidth * imageHeight * 4);
    m_image = new QImage(m_imageData, imageWidth, imageHeight, QImage::Format_RGB32);
    m_img1aspectRatio = aspectRatio;
}


void PaintWidget::paintScreen(QPainter* painter, QRect dstRect)
{
    painter->setRenderHint(QPainter::SmoothPixmapTransform, m_antialiasing);

    QRect srcRect(0, 0, m_image->width(), m_image->height());

    if (!m_image2) {
        /*int kx = dstRect.width() / m_image->width() + 1;
        int ky = dstRect.height() / m_image->height() + 1;
        auto scaledImage = m_image->scaled(m_image->width() * kx, m_image->height() * ky);
        QRect scaledRect(srcRect.left(), srcRect.top(), srcRect.width() * kx, srcRect.height() * ky);
        painter->drawImage(dstRect, scaledImage, scaledRect);*/

        painter->drawImage(dstRect, *m_image, srcRect);
    } else if (m_useAlpha) {
        painter->drawImage(dstRect, *m_image, srcRect);
        QRectF srcRect2(0, 0, m_image2->width(), m_image2->height());
        painter->drawImage(dstRect, *m_image2, srcRect2);
    } else {
        painter->setOpacity(1);
        painter->fillRect(dstRect, Qt::black);
        painter->drawImage(dstRect, *m_image, srcRect);
        QRectF srcRect2(0, 0, m_image2->width(), m_image2->height());
        painter->setOpacity(0.5);
        painter->drawImage(dstRect, *m_image2, srcRect2);
    }
}


void PaintWidget::paintEvent(QPaintEvent*)
{
    if (!m_needPaint)
        return;
    m_needPaint = false;

    QPainter* painter = new QPainter();
    painter->begin(this);
    painter->fillRect(QRect(0, 0, width(), height()), m_fillColor);

    if (m_image)
        paintScreen(painter, m_dstRect);

    painter->end();

    delete painter;

    static_cast<MainWindow*>(parent())->incFrameCount();
}

/*void PaintWidget::setVsync(bool vsync)
{
    QSurfaceFormat format;
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSwapInterval(vsync ? 1 : 0);
    setFormat(format);
}*/


void PaintWidget::mouseMoveEvent(QMouseEvent*)
{
    if (!m_hideCursor)
        return;

    if (m_cursorHidden) {
        setCursor(Qt::ArrowCursor);
        m_cursorHidden = false;
    }
    m_hideCursorTimer.start();
}


void PaintWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() & Qt::LeftButton)
        static_cast<MainWindow*>(parent())->mouseClick(event->x(), event->y(), PM_LEFT_CLICK);
}


void PaintWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() & Qt::LeftButton)
        static_cast<MainWindow*>(parent())->mouseClick(event->x(), event->y(), PM_LEFT_DBLCLICK);
}


void PaintWidget::wheelEvent(QWheelEvent *event)
{
    static_cast<MainWindow*>(parent())->mouseClick(event->x(), event->y(), event->delta() > 0 ? PM_WHEEL_UP : PM_WHEEL_DOWN);
}


void PaintWidget::onHideCursorTimer()
{
    m_hideCursorTimer.stop();
    setCursor(Qt::BlankCursor);
    m_cursorHidden = true;
}


void PaintWidget::setHideCursor(bool hide)
{
    m_hideCursor = hide;
    if (m_hideCursor) {
        m_hideCursorTimer.start();
    } else {
        m_hideCursorTimer.stop();
        setCursor(Qt::ArrowCursor);
        m_cursorHidden = false;
    }
}
