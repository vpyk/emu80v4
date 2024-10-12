/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2024
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
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>

#include "qtMainWindow.h"
#include "qtPalWindow.h"

//class GrWidget : public QWidget
class PaintWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
        Q_OBJECT

    public:
        explicit PaintWidget(QWidget *parent = 0);
        ~PaintWidget();

        void drawImage(uint32_t* pixels, int imageWidth, int imageHeight, double aspectRatio, bool blend = false, bool useAlpha = false);
        void colorFill(QColor color);
        void screenshot(const QString& ssFileName);
        void setHideCursor(bool hide);

        void setSmoothing(SmoothingType smoothing) {m_smoothing = smoothing;}
        //void setVsync(bool vsync);

        int getImageWidth() {return m_dstRect.width();}
        int getImageHeight() {return m_dstRect.height();}

    protected:
        void mouseMoveEvent(QMouseEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
        void mouseDoubleClickEvent(QMouseEvent *event) override;
        void wheelEvent(QWheelEvent *event) override;

        void initializeGL() override;
        void resizeGL(int w, int h) override;
        void paintGL() override;

    private slots:
        void onHideCursorTimer();

    private:
        void paintImageGL(QImage* img/*, double aspectRatio*/);

        void mouseDrag(int x, int y);

        QImage* m_image = nullptr;
        QImage* m_image2 = nullptr;
        uchar* m_imageData;
        uchar* m_imageData2;
        double m_img1aspectRatio;
        //double m_img2aspectRatio;

        bool m_useAlpha = false;
        QColor m_fillColor = Qt::black;
        QRect m_dstRect;

        SmoothingType m_smoothing = ST_SHARP;

        bool m_hideCursor = true;
        QTimer m_hideCursorTimer;
        bool m_cursorHidden = false;

        QOpenGLShaderProgram* m_program;
        QOpenGLBuffer m_vbo;

        QOpenGLTexture* m_texture = nullptr;

        float c_vertices[16] = {
            1.0, -1.0,  1.0, 1.0,
           -1.0, -1.0,  0.0, 1.0,
           -1.0,  1.0,  0.0, 0.0,
            1.0,  1.0,  1.0, 0.0
        };
};


#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1


#endif // QTPAINTWIDGET_H
