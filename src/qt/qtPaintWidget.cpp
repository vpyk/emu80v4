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

#include <QOpenGLContext>
#include <QOpenGLTexture>
#include <QMouseEvent>
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

    delete m_program;
}


// Save screenshot or copy it to clipboard if filename is empty
void PaintWidget::screenshot(const QString& ssFileName)
{
    QImage fullImg = grabFramebuffer();
    QImage img = fullImg.copy(m_dstRect);

    if (ssFileName != "")
        img.save(ssFileName);
    else
        QGuiApplication::clipboard()->setImage(img);
}


void PaintWidget::colorFill(QColor color)
{
    m_fillColor = color;
    if (m_image) {
        delete m_image;
        delete[] m_imageData;
        m_image = nullptr;
    }
    //update();
}


void PaintWidget::drawImage(uint32_t* pixels, int imageWidth, int imageHeight, double aspectRatio, bool blend, bool useAlpha)
{
    if (m_image2) {
        delete m_image2;
        m_image2 = nullptr;
        delete[] m_imageData2;
    }

    if (useAlpha || blend) {
        m_imageData2 = new uchar[imageWidth * imageHeight * 4];
        memcpy(m_imageData2, pixels, imageWidth * imageHeight * 4);
        m_image2 = new QImage(m_imageData2, imageWidth, imageHeight, useAlpha ? QImage::Format_ARGB32 : QImage::Format_RGB32);
        //m_img2aspectRatio = aspectRatio;
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

    //update();
}


void PaintWidget::initializeGL()
{
    initializeOpenGLFunctions();

    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(c_vertices, 16 * sizeof(float));

    QOpenGLShader *vShader = new QOpenGLShader(QOpenGLShader::Vertex);
    QOpenGLShader *fShader = new QOpenGLShader(QOpenGLShader::Fragment);

    if (!vShader->compileSourceCode(R"(
        attribute highp vec2 vertCoord;
        attribute highp vec2 texCoord;

        varying highp vec2 vTexCoord;
        varying vec2 prescale;

        uniform highp vec2 textureSize;
        uniform highp vec2 outputSize;
        uniform highp vec2 destSize;
        //uniform highp vec2 destPos;

        void main()
        {
            mediump vec2 scale = destSize / outputSize;
            gl_Position = vec4(vertCoord * scale, 0.0, 1.0);
            vTexCoord = texCoord * textureSize;
            prescale = ceil(outputSize / textureSize);
        })")) {
        qDebug() << vShader->log();
    }

    if (!fShader->compileSourceCode(R"(
        uniform sampler2D texture1;
        uniform highp vec2 textureSize;
        uniform bool sharp;

        varying highp vec2 vTexCoord;
        varying highp vec2 prescale;

        void main()
        {
            if (sharp) {
                const mediump vec2 halfp = vec2(0.5);
                highp vec2 texel_floored = floor(vTexCoord);
                highp vec2 s = fract(vTexCoord);
                highp vec2 region_range = halfp - halfp / prescale;

                highp vec2 center_dist = s - halfp;
                highp vec2 f = (center_dist - clamp(center_dist, -region_range, region_range)) * prescale + halfp;

                highp vec2 mod_texel = min(texel_floored + f, textureSize-halfp);
                gl_FragColor = texture2D(texture1, mod_texel / textureSize);
            } else
                gl_FragColor = texture2D(texture1, (vTexCoord + 0.002) / textureSize);
        })")) {
        qDebug() << fShader->log();
    }

    m_program = new QOpenGLShaderProgram;
    m_program->addShader(vShader);
    m_program->addShader(fShader);
    m_program->bindAttributeLocation("vertCoord", PROGRAM_VERTEX_ATTRIBUTE);
    m_program->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    m_program->link();

    m_program->bind();
    //m_program->setUniformValue("texture1", 0);
    m_program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    m_program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    m_program->setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 2, sizeof(float) * 4);
    m_program->setAttributeBuffer(PROGRAM_TEXCOORD_ATTRIBUTE, GL_FLOAT, sizeof(float) * 2, 2, sizeof(float) * 4);

    delete vShader;
    delete fShader;
}


void PaintWidget::paintImageGL(QImage* img/*, double aspectRatio*/)
{
    int dstWidth, dstHeight, dstX, dstY;
    PalWindow* palWindow = static_cast<MainWindow*>(parent())->getPalWindow();

    if (!palWindow)
        return;

    //palWindow->calcDstRect(img->width(), img->height(), aspectRatio, width(), height(), dstWidth, dstHeight, dstX, dstY);
    palWindow->calcDstRect(m_image->width(), m_image->height(), m_img1aspectRatio, width(), height(), dstWidth, dstHeight, dstX, dstY);
    m_dstRect.setRect(dstX, dstY, dstWidth, dstHeight);

    m_program->setUniformValue("sharp", m_smoothing == ST_SHARP);
    m_program->setUniformValue("textureSize", img->size());
    m_program->setUniformValue("destSize", QSize(dstWidth, dstHeight));

    QOpenGLTexture* texture = new QOpenGLTexture(*img, QOpenGLTexture::DontGenerateMipMaps);
    texture->setMagnificationFilter(m_smoothing != ST_NEAREST ? QOpenGLTexture::Linear : QOpenGLTexture::Nearest);
    texture->setMinificationFilter(m_smoothing != ST_NEAREST ? QOpenGLTexture::Linear : QOpenGLTexture::Nearest);
    texture->bind();

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    delete texture;
}


void PaintWidget::paintGL()
{
    glClearColor(m_fillColor.red() / 255.0, m_fillColor.green() / 255.0, m_fillColor.blue() / 255.0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_vbo.bind();
    m_program->bind();

    if (m_image) {
        glDisable(GL_BLEND);
        paintImageGL(m_image/*, m_img1aspectRatio*/);

        if (m_image2) {
            if (m_useAlpha) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                paintImageGL(m_image2/*, m_img2aspectRatio*/);
            } else {
                glEnable(GL_BLEND);
                glBlendColor(0.0, 0.0, 0.0, 0.5);
                glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
                paintImageGL(m_image2/*, m_img2aspectRatio*/);
            }
        }
    }

    static_cast<MainWindow*>(parent())->incFrameCount();
}


void PaintWidget::resizeGL(int w, int h)
{
    m_program->setUniformValue("outputSize", QSize(w, h));
}


/*void PaintWidget::setVsync(bool vsync)
{
    QSurfaceFormat format;
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSwapInterval(vsync ? 1 : 0);
    setFormat(format);
}*/


void PaintWidget::mouseDrag(int x, int y)
{
    static_cast<MainWindow*>(parent())->mouseDrag(x, y);
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  #define POS_X x
  #define POS_Y y
#else
  #define POS_X position().x
  #define POS_Y position().y
#endif

void PaintWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton)
        mouseDrag(event->POS_X(), event->POS_Y());

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
        static_cast<MainWindow*>(parent())->mouseClick(event->POS_X(), event->POS_Y(), PM_LEFT_CLICK);

    mouseDrag(event->POS_X(), event->POS_Y());
}


void PaintWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() & Qt::LeftButton)
        static_cast<MainWindow*>(parent())->mouseClick(event->POS_X(), event->POS_Y(), PM_LEFT_DBLCLICK);
}


void PaintWidget::wheelEvent(QWheelEvent *event)
{
    static_cast<MainWindow*>(parent())->mouseClick(event->POS_X(), event->POS_Y(), event->angleDelta().y() > 0 ? PM_WHEEL_UP : PM_WHEEL_DOWN);
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
