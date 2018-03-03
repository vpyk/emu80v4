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

#include "sdlPalFile.h"

using namespace std;

bool PalFile::open(string fileName, string mode)
{
    m_file = SDL_RWFromFile(fileName.c_str(), mode.c_str());
    return m_file;
}



void PalFile::close()
{
    if (m_file)
        SDL_RWclose(m_file);
}


bool PalFile::isOpen()
{
    return m_file != nullptr;
}


uint8_t PalFile::read8()
{
    return SDL_ReadU8(m_file);
}


uint16_t PalFile::read16()
{
    return SDL_ReadLE16(m_file);
}


uint32_t PalFile::read32()
{
    return SDL_ReadLE32(m_file);
}


void PalFile::write8(uint8_t value)
{
    SDL_WriteU8(m_file, value);
}


void PalFile::write16(uint16_t value)
{
    SDL_WriteLE16(m_file, value);
}


void PalFile::write32(uint32_t value)
{
    SDL_WriteLE32(m_file, value);
}


int64_t PalFile::getSize()
{
    return SDL_RWsize(m_file);
}


void PalFile::seek(int position)
{
    SDL_RWseek(m_file, position, RW_SEEK_SET);
}


void PalFile::skip(int len)
{
    SDL_RWseek(m_file, len, RW_SEEK_CUR);
}


int64_t PalFile::getPos()
{
    return SDL_RWtell(m_file);
}


bool PalFile::eof()
{
    return SDL_RWsize(m_file) == SDL_RWtell(m_file);
}
