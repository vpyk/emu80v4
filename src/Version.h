/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2018-2025
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

#ifndef VERSION_H
#define VERSION_H

#define XSTR(s) STR(s)
#define STR(s) #s

#define VER_MAJOR 4
#define VER_MINOR 0
#define VER_BUILD 546

#define VER_STR XSTR(VER_MAJOR) "." XSTR(VER_MINOR) "." XSTR(VER_BUILD)
#define VER_COMMA_SEP VER_MAJOR,VER_MINOR,VER_BUILD

#define VI_FILE_DESCR "Emu80 emulator"
#define VI_COPYRIGHT "Copyright © Viktor Pykhonin, 2016-2025"
#define VI_FILE_NAME "Emu80qt.exe"
#define VI_PRODUCT "Emu80"

#endif // VERSION_H
