/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2022
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

#ifndef CPUZ80DASM_H
#define CPUZ80DASM_H

#include <string>

enum STEP_FLAG {
    SF_STEP,
    SF_OVER,
    SF_OUT
};

extern const std::string& cpu_disassemble_z80(uint16_t pc, const uint8_t *mem, unsigned& length, STEP_FLAG& flag);


#endif //CPUZ80DASM_H
