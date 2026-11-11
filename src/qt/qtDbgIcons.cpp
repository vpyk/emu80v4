/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2026
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

#include <QGuiApplication>
#include <QIconEngine>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>

#include "qtDbgIcons.h"
#include "qtDbgStyle.h"


namespace {

// Paints an icon on a 16x16 grid scaled to the requested size
class DbgIconEngine : public QIconEngine
{
public:
    explicit DbgIconEngine(DbgIconKind kind) : m_kind(kind) {}

    void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;
    QIconEngine* clone() const override {return new DbgIconEngine(m_kind);}
    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // a sharp pixmap for high DPI screens
    QPixmap scaledPixmap(const QSize& size, QIcon::Mode mode, QIcon::State state, qreal scale) override;
#endif

private:
    DbgIconKind m_kind;
};


void drawArrowHead(QPainter* painter, QPointF tip, QPointF dir, const QColor& color)
{
    // dir is a unit vector the head points to
    QPointF normal(-dir.y(), dir.x());
    const double len = 4.2, half = 3.2;
    QPointF base = tip - dir * len;
    QPolygonF head;
    head << tip << base + normal * half << base - normal * half;
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawPolygon(head);
}


void drawLine(QPainter* painter, QPointF p1, QPointF p2, const QColor& color)
{
    painter->setPen(QPen(color, 1.8, Qt::SolidLine, Qt::FlatCap));
    painter->setBrush(Qt::NoBrush);
    painter->drawLine(p1, p2);
}


// The current instruction: a dot
void drawDot(QPainter* painter, const QColor& color)
{
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawEllipse(QPointF(8, 13.2), 2.0, 2.0);
}


// The current instruction that isn't executed: a cross in place of the dot
void drawCross(QPainter* painter, const QColor& color)
{
    painter->setPen(QPen(color, 1.6, Qt::SolidLine, Qt::RoundCap));
    const double c = 2.1;
    painter->drawLine(QPointF(8 - c, 13 - c), QPointF(8 + c, 13 + c));
    painter->drawLine(QPointF(8 - c, 13 + c), QPointF(8 + c, 13 - c));
}


// An arc arrow from the left over the dot to the right
void drawArcArrow(QPainter* painter, const QColor& color)
{
    QPainterPath path;
    path.moveTo(2.5, 11);
    path.cubicTo(2.5, 2.5, 13.5, 2.5, 13.5, 8);
    painter->setPen(QPen(color, 1.8, Qt::SolidLine, Qt::FlatCap));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path);
    drawArrowHead(painter, QPointF(13.5, 12), QPointF(0, 1), color);
}


void DbgIconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State)
{
    QPalette palette = QGuiApplication::palette();
    DbgColors colors(palette);
    bool disabled = mode == QIcon::Disabled;

    QColor gray = palette.color(QPalette::Disabled, QPalette::WindowText);
    QColor arrow = disabled ? gray : colors.pcMarker;
    QColor dot = disabled ? gray : palette.color(QPalette::WindowText);
    QColor run = disabled ? gray : colors.dark ? QColor(0x4E, 0xC8, 0x6A) : QColor(0x2E, 0x9E, 0x44);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    int side = qMin(rect.width(), rect.height());
    painter->translate(rect.x() + (rect.width() - side) / 2.0, rect.y() + (rect.height() - side) / 2.0);
    painter->scale(side / 16.0, side / 16.0);

    switch (m_kind) {
    case DI_RUN: {
        QPolygonF triangle;
        triangle << QPointF(4, 2.5) << QPointF(13.5, 8) << QPointF(4, 13.5);
        painter->setPen(QPen(run, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(run);
        painter->drawPolygon(triangle);
        break;
    }
    case DI_BREAK:
        painter->setPen(Qt::NoPen);
        painter->setBrush(arrow);
        painter->drawRoundedRect(QRectF(3.5, 2.5, 3, 11), 0.8, 0.8);
        painter->drawRoundedRect(QRectF(9.5, 2.5, 3, 11), 0.8, 0.8);
        break;
    case DI_STEP_INTO:
        drawLine(painter, QPointF(8, 1.5), QPointF(8, 7), arrow);
        drawArrowHead(painter, QPointF(8, 10.2), QPointF(0, 1), arrow);
        drawDot(painter, dot);
        break;
    case DI_STEP_OVER:
        drawArcArrow(painter, arrow);
        drawDot(painter, dot);
        break;
    case DI_STEP_OUT:
        drawLine(painter, QPointF(8, 10.5), QPointF(8, 5), arrow);
        drawArrowHead(painter, QPointF(8, 1.5), QPointF(0, -1), arrow);
        drawDot(painter, dot);
        break;
    case DI_RUN_TO_CURSOR:
        drawLine(painter, QPointF(1.5, 8), QPointF(7.5, 8), arrow);
        drawArrowHead(painter, QPointF(11, 8), QPointF(1, 0), arrow);
        drawLine(painter, QPointF(13.5, 2.5), QPointF(13.5, 13.5), dot);
        break;
    case DI_SKIP:
        // past the instruction to the next one
        drawLine(painter, QPointF(1.5, 6), QPointF(11, 6), arrow);
        drawArrowHead(painter, QPointF(14.5, 6), QPointF(1, 0), arrow);
        drawCross(painter, dot);
        break;
    }

    painter->restore();
}

// The default implementation leaves the pixmap uninitialized
QPixmap DbgIconEngine::pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state)
{
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    paint(&painter, QRect(QPoint(0, 0), size), mode, state);
    return pixmap;
}


#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
QPixmap DbgIconEngine::scaledPixmap(const QSize& size, QIcon::Mode mode, QIcon::State state, qreal scale)
{
    QPixmap pixmap(size * scale);
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(scale);
    QPainter painter(&pixmap);
    paint(&painter, QRect(QPoint(0, 0), size), mode, state);
    return pixmap;
}
#endif

} // namespace


QIcon dbgIcon(DbgIconKind kind)
{
    return QIcon(new DbgIconEngine(kind));
}
