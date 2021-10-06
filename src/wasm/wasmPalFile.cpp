/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2024
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

#include "../sdl/sdlPal.h"

#include "wasmPalFile.h"

using namespace std;

bool PalFile::open(string fileName, string mode)
{
    m_fileBuffer = palReadFile(fileName, m_fileSize);
    return m_fileBuffer != nullptr;
}



void PalFile::close()
{
    if (m_fileBuffer) {
        delete[] m_fileBuffer;
        m_fileBuffer = nullptr;
        m_filePos = 0;
    }
}


bool PalFile::isOpen()
{
    return m_fileBuffer != nullptr;
}


uint8_t PalFile::read8()
{
    if (m_filePos > m_fileSize - 1)
        return 0;
    return m_fileBuffer[m_filePos++];
}


uint16_t PalFile::read16()
{
    if (m_filePos > m_fileSize - 2)
        return 0;
    uint16_t res = m_fileBuffer[m_filePos] | (m_fileBuffer[m_filePos + 1] << 8);
    m_filePos += 2;
    return res;
}


uint32_t PalFile::read32()
{
    if (m_filePos > m_fileSize - 4)
        return 0;
    uint16_t res = m_fileBuffer[m_filePos] | (m_fileBuffer[m_filePos + 1] << 8) | (m_fileBuffer[m_filePos + 2] << 16) | (m_fileBuffer[m_filePos + 3] << 24);
    m_filePos += 4;
    return res;
}


void PalFile::write8(uint8_t value)
{
    if (m_filePos > m_fileSize - 1)
        return;
    m_fileBuffer[m_filePos++] = value;
}


void PalFile::write16(uint16_t value)
{
    if (m_filePos > m_fileSize - 2)
        return;
    m_fileBuffer[m_filePos++] = value & 0xFF;
    m_fileBuffer[m_filePos++] = value >> 8;
}


void PalFile::write32(uint32_t value)
{
    if (m_filePos > m_fileSize - 4)
        return;
    m_fileBuffer[m_filePos++] = value & 0xFF;
    m_fileBuffer[m_filePos++] = (value >> 8) & 0xFF;
    m_fileBuffer[m_filePos++] = (value >> 16) & 0xFF;
    m_fileBuffer[m_filePos++] = (value >> 24) & 0xFF;
}


int64_t PalFile::getSize()
{
    return m_fileSize;
}


void PalFile::seek(int position)
{
    if (position < m_fileSize)
        m_filePos = position;
    else
        m_filePos = m_fileSize;
}


void PalFile::skip(int len)
{
    if (m_fileSize + len < m_fileSize)
        m_filePos += len;
    else
        m_filePos = m_fileSize;
}


int64_t PalFile::getPos()
{
    return m_filePos;
}


bool PalFile::eof()
{
    return m_filePos < m_fileSize;
}
