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

#ifndef QTPAINTWIDGET_H
#define QTPAINTWIDGET_H

#include <QWidget>
#include <QOpenGLWidget>

#include "qtMainWindow.h"

//class GrWidget : public QWidget
class PaintWidget : public QOpenGLWidget
{
        Q_OBJECT

    public:
        explicit PaintWidget(QWidget *parent = 0);
        ~PaintWidget();

        void drawImage(uint32_t* pixels, int imageWidth, int imageHeight, int dstX, int dstY, int dstWidth, int dstHeight, bool blend = false, bool useAlpha = false);
        void colorFill(QColor color);
        void draw();
        void screenshot(QString& ssFileName);

        void setAntialiasing(bool aa) {m_antialiasing = aa;};
        //void setVsync(bool vsync);

    protected:
        void paintEvent(QPaintEvent *);

    private:
        void paintScreen(QPainter* painter, QRect dstRect);

        QImage* m_image = nullptr;
        QImage* m_image2 = nullptr;
        uchar* m_imageData;
        uchar* m_imageData2;

        bool m_needPaint = false;
        bool m_useAlpha = false;
        QColor m_fillColor = Qt::black;
        QRect m_dstRect;

        bool m_antialiasing = false;
};

#endif // QTPAINTWIDGET_H
