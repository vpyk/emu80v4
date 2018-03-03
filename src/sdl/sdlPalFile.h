/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017
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

#ifndef SDLPALFILE_H
#define SDLPALFILE_H

#include <string>

#include <SDL2/SDL.h>

class PalFile
{
    public:
        bool open(std::string fileName, std::string mode = "r");
        void close();
        bool isOpen();
        bool eof();
        uint8_t read8();
        uint16_t read16();
        uint32_t read32();
        void write8(uint8_t value);
        void write16(uint16_t value);
        void write32(uint32_t value);
        int64_t getSize();
        int64_t getPos();
        void seek(int position);
        void skip(int len);

    private:
        SDL_RWops* m_file = nullptr;
};

#endif // SDLPALFILE_H
