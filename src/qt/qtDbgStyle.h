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

// Common look of the Qt debugger panels: font, metrics and colors

#ifndef QTDBGSTYLE_H
#define QTDBGSTYLE_H

#include <QColor>
#include <QFont>

class QPalette;


// The font of the debugger panels (monospace)
QFont dbgFont();


// Character cell of a monospace font
struct DbgMetrics {
    int charWidth;
    int lineHeight;
    int ascent;

    explicit DbgMetrics(const QFont& font);
};


// Colors of the debugger panels. The background and text come from the palette;
// the accent colors have a light and a dark theme set. The theme is dark if the
// window color of the palette is dark (as in the main window).
//
// "Stale" colors show the data of the last stop while the emulation runs.
struct DbgColors {
    bool dark;

    QColor background;
    QColor text;
    QColor dimText;             // secondary text: addresses, bytes
    QColor staleBackground;
    QColor staleText;
    QColor staleDimText;

    QColor pcBackground;        // the line or value at PC
    QColor stalePcBackground;
    QColor pcMarker;            // PC arrow
    QColor cursorBackground;    // the cursor line in a focused panel
    QColor cursorFrame;
    QColor cursorFrameUnfocused;
    QColor changedText;         // values changed since the previous stop
    QColor breakpoint;          // breakpoint of the UI
    QColor externalBreakpoint;  // breakpoint of an external client (MCP)

    // Colors for the run state
    const QColor& bg(bool stale) const {return stale ? staleBackground : background;}
    const QColor& fg(bool stale) const {return stale ? staleText : text;}
    const QColor& dimFg(bool stale) const {return stale ? staleDimText : dimText;}
    const QColor& pcBg(bool stale) const {return stale ? stalePcBackground : pcBackground;}

    explicit DbgColors(const QPalette& palette);
};


// Linear mix of two colors: k = 0 gives c1, k = 1 gives c2
QColor dbgMixColors(const QColor& c1, const QColor& c2, double k);

// Colors with the outdated look applied (a breakpoint marker while running etc.)
QColor dbgStaleColor(const DbgColors& colors, const QColor& color);


#endif // QTDBGSTYLE_H
