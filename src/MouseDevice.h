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

#ifndef MOUSEDEVICE_H
#define MOUSEDEVICE_H

#include <string>

// Мышь платформы (Вектор: контроллер USB-мыши KTSerg на порту D4h). Координаты — в точках кадра
// после EmuWindow::translateCoords; inside = false, когда указатель вне картинки
class MouseDevice
{
    public:
        virtual ~MouseDevice() = default;
        virtual bool hostMouseMove(int x, int y, bool inside, int buttons) = 0;   // кнопки: 1 левая, 2 правая, 4 средняя;
                                                                                   // true — указатель над картинкой
        virtual void hostMouseWheel(int steps) = 0;                                // + от себя, - на себя
        virtual void emuMouseSet(int x, int y, int buttons) = 0;                   // для MCP: x 0..511, y 0..255 (вверх); -1 — не менять
        virtual bool isMouseActive() = 0;                                          // программа включила мышь
        virtual std::string getMouseInfo() = 0;
};

#endif // MOUSEDEVICE_H
