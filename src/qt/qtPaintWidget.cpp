/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2025
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
#include <QFile>

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

    delete m_standardVShader;
    delete m_standardFShader;

    if (m_customVShader)
        delete m_customVShader;
    if (m_customFShader)
        delete m_customFShader;

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


const char* vShaderSrc = R"(
#ifdef GL_ES
precision mediump float;
#endif
        attribute vec2 VertexCoord;
        attribute vec2 TexCoord;

        varying vec2 vTexCoord;
        varying vec2 prescale;

        uniform vec2 TextureSize;
        uniform vec2 OutputSize;

        void main()
        {
            gl_Position = vec4(VertexCoord, 0.0, 1.0);
            vTexCoord  = TexCoord * TextureSize;
            prescale = ceil(OutputSize / TextureSize);
        })";

const char* fShaderSrc = R"(
#ifdef GL_ES
precision mediump float;
#endif

        uniform sampler2D texture1;
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
                gl_FragColor = texture2D(texture1, ((vTexCoord + 0.002) / TextureSize));
            if (grayscale)
                gl_FragColor = vec4(vec3(toGrayscale(gl_FragColor.rgb)), gl_FragColor.a);
        })";



void PaintWidget::initializeGL()
{
    initializeOpenGLFunctions();

    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(c_vertices, 16 * sizeof(float));

    m_standardVShader = new QOpenGLShader(QOpenGLShader::Vertex);
    m_standardFShader = new QOpenGLShader(QOpenGLShader::Fragment);

    // create standard internal shaders
    if (!m_standardVShader->compileSourceCode(vShaderSrc))
        qDebug() << m_standardVShader->log();
    if (!m_standardFShader->compileSourceCode(fShaderSrc))
            qDebug() << m_standardVShader->log();

    m_mvpMatrix.setToIdentity();

    //createProgram(m_standardVShader, m_standardFShader);
    //m_useCustomShader = false;

    recreateProgramIfNeeded();
}


bool PaintWidget::createProgram(QOpenGLShader* vShader, QOpenGLShader* fShader)
{
    if (m_program)
        delete m_program;

    m_program = new QOpenGLShaderProgram;
    m_program->addShader(vShader);
    m_program->addShader(fShader);

    m_program->bindAttributeLocation("VertexCoord", PROGRAM_VERTEX_ATTRIBUTE);
    m_program->bindAttributeLocation("TexCoord", PROGRAM_TEXCOORD_ATTRIBUTE);

    bool res = m_program->link();
    if (!res) {
        qDebug() << m_program->log();
        return false;
    }

    m_program->bind();
    m_program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    m_program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);

    m_program->setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 2, sizeof(float) * 4);
    m_program->setAttributeBuffer(PROGRAM_TEXCOORD_ATTRIBUTE, GL_FLOAT, sizeof(float) * 2, 2, sizeof(float) * 4);

    m_program->setUniformValue("MVPMatrix", m_mvpMatrix);

    return true;
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

    glViewport(dstX, dstY, dstWidth, dstHeight);
    m_program->setUniformValue("TextureSize", img->size());
    m_program->setUniformValue("InputSize", img->size());
    m_program->setUniformValue("OutputSize", QSize(dstWidth, dstHeight));

    if (!m_useCustomShader) {
        m_program->setUniformValue("sharp", m_smoothing == ST_SHARP);
    }

    m_program->setUniformValue("grayscale", m_desaturate);

    QOpenGLTexture* texture = new QOpenGLTexture(*img, QOpenGLTexture::DontGenerateMipMaps);
    texture->setMagnificationFilter(m_smoothing != ST_NEAREST ? QOpenGLTexture::Linear : QOpenGLTexture::Nearest);
    texture->setMinificationFilter(m_smoothing != ST_NEAREST ? QOpenGLTexture::Linear : QOpenGLTexture::Nearest);
    texture->setWrapMode(QOpenGLTexture::ClampToEdge);
    texture->bind();

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    delete texture;
}


void PaintWidget::paintGL()
{
    glClearColor(m_fillColor.red() / 255.0, m_fillColor.green() / 255.0, m_fillColor.blue() / 255.0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!m_shaderValid)
        return;

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


void PaintWidget::setSmoothingAndShaderFile(SmoothingType smoothing, const QString& shaderFileName)
{
    m_needToRecreateProgram = m_needToRecreateProgram || (shaderFileName != m_shaderFileName) || (smoothing != m_smoothing);

    m_smoothing = smoothing;
    m_shaderFileName = shaderFileName;

    recreateProgramIfNeeded();
}


void PaintWidget::recreateProgramIfNeeded()
{
    if (!m_needToRecreateProgram || !isValid())
        return;

    makeCurrent();

    m_needToRecreateProgram = false;

    if (m_customVShader)
        delete m_customVShader;
    if (m_customFShader)
        delete m_customFShader;

    if (m_smoothing != ST_CUSTOM) {
        m_useCustomShader = false;
        m_customVShader = nullptr;
        m_customFShader = nullptr;
        createProgram(m_standardVShader, m_standardFShader);
        m_shaderValid = true;
        return;
    }

    m_useCustomShader = true;

    m_customVShader = new QOpenGLShader(QOpenGLShader::Vertex);
    m_customFShader = new QOpenGLShader(QOpenGLShader::Fragment);

    QFile shaderFile(m_shaderFileName);

    bool success = shaderFile.open(QFile::ReadOnly | QFile::Text);

    QString shaderSrc = "";
    QString versionStr = "";

    if (success) {
        while (!shaderFile.atEnd()) {
            QString line = shaderFile.readLine();
            if (line.startsWith("#version"))
                versionStr = line;
            else {
                shaderSrc += line;
            }
        }
    }

    if (success && !m_customVShader->compileSourceCode(versionStr + "#define VERTEX\n" + shaderSrc)) {
        qDebug() << m_customVShader->log();
        success = false;
    }

    if (success && !m_customFShader->compileSourceCode(versionStr + "#define FRAGMENT\n" + shaderSrc)) {
        qDebug() << m_customFShader->log();
        success = false;
    }

    if (success)
        success = createProgram(m_customVShader, m_customFShader);

    m_useCustomShader = success;
    m_shaderValid = success;
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
