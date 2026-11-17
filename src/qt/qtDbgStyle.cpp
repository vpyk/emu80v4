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

#include <QAbstractScrollArea>
#include <QContextMenuEvent>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QMenu>
#include <QPalette>

#include "qtDbgStyle.h"
#include "qtDbgCompat.h"


namespace {

// The embedded Hack font, or the system fixed font if the font can't be loaded
QFont createFont()
{
    int id = QFontDatabase::addApplicationFont(":/fonts/Hack-Regular.ttf");
    QStringList families = id >= 0 ? QFontDatabase::applicationFontFamilies(id) : QStringList();
    if (families.isEmpty())
        return QFontDatabase::systemFont(QFontDatabase::FixedFont);

    QFont font(families.first());
    font.setStyleHint(QFont::TypeWriter);
    font.setPointSize(10); // todo: make the size an option
    return font;
}

} // namespace


QFont dbgFont()
{
    static QFont font = createFont();
    return font;
}


DbgMetrics::DbgMetrics(const QFont& font)
{
    QFontMetrics fm(font);
    charWidth = dbgTextWidth(fm, "0");
    lineHeight = fm.height();
    ascent = fm.ascent();
}


QColor dbgMixColors(const QColor& c1, const QColor& c2, double k)
{
    return QColor::fromRgbF(c1.redF() * (1 - k) + c2.redF() * k,
                            c1.greenF() * (1 - k) + c2.greenF() * k,
                            c1.blueF() * (1 - k) + c2.blueF() * k);
}


QColor dbgStaleColor(const DbgColors& colors, const QColor& color)
{
    return dbgMixColors(color, colors.staleBackground, 0.4);
}


DbgColors::DbgColors(const QPalette& palette)
{
    dark = qGray(palette.color(QPalette::Window).rgb()) < 128;

    background = palette.color(QPalette::Base);
    text = palette.color(QPalette::Text);
    dimText = dbgMixColors(text, background, 0.4);
    // shifted to the text color: darker in a light theme, lighter in a dark one, where a darker
    // background (the window color) hardly differs from the normal one
    staleBackground = dbgMixColors(background, text, dark ? 0.1 : 0.06);
    staleText = palette.color(QPalette::Disabled, QPalette::Text);
    staleDimText = dbgMixColors(staleText, staleBackground, 0.4);

    if (!dark) {
        pcBackground = QColor(0xC8, 0xDF, 0xF5);
        pcMarker = QColor(0x1E, 0x6F, 0xD0);
        cursorBackground = QColor(0xEE, 0xF4, 0xFB);
        cursorFrame = QColor(0x38, 0x74, 0xC8);
        staleCursorFrame = QColor(0xA8, 0xC4, 0xE6);
        changedText = QColor(0xC8, 0x00, 0x00);
        breakpoint = QColor(0xDC, 0x32, 0x32);
        externalBreakpoint = QColor(0xE6, 0x8C, 0x00);
    } else {
        pcBackground = QColor(0x26, 0x4F, 0x78);
        pcMarker = QColor(0x4F, 0xA3, 0xFF);
        cursorBackground = QColor(0x33, 0x39, 0x40);
        cursorFrame = QColor(0x6A, 0x9B, 0xD8);
        staleCursorFrame = QColor(0x4A, 0x5A, 0x6A);
        changedText = QColor(0xFF, 0x78, 0x64);
        breakpoint = QColor(0xF0, 0x48, 0x48);
        externalBreakpoint = QColor(0xFF, 0xA0, 0x28);
    }

    stalePcBackground = dbgMixColors(staleBackground, pcBackground, 0.5);
}


void dbgShowContextMenu(QAbstractScrollArea* view, QContextMenuEvent* event, const QRect& cursorRect)
{
    if (view->actions().isEmpty())
        return;
    QPoint pos = event->globalPos();
    if (event->reason() == QContextMenuEvent::Keyboard) {
        QWidget* viewport = view->viewport();
        QRect rect = cursorRect.intersected(viewport->rect());
        pos = viewport->mapToGlobal(rect.isEmpty() ? viewport->rect().center() : rect.bottomLeft() + QPoint(0, 1));
    }
    QMenu::exec(view->actions(), pos, nullptr, view);
    event->accept();
}
