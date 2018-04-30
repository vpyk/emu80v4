/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2018
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

#include <QFile>

#include "qtPalFile.h"

using namespace std;

bool PalFile::open(string fileName, string mode)
{
    m_file = new QFile(QString::fromUtf8(fileName.c_str()));
    QIODevice::OpenModeFlag qmode;
    if (mode == "w")
        qmode = QIODevice::WriteOnly;
    else if (mode == "r")
        qmode = QIODevice::ReadOnly;
    else //if (mode == "rw")
        qmode = QIODevice::ReadWrite;
    if (m_file->open(qmode))
        return true;
    return false;
}


void PalFile::close()
{
    m_file->close();
    m_file = nullptr;
}


bool PalFile::isOpen()
{
    return m_file && m_file->isOpen();
}


uint8_t PalFile::read8()
{
    uint8_t data;
    m_file->read((char*)&data, 1);
    return data;
}


uint16_t PalFile::read16()
{
    uint16_t data;
    m_file->read((char*)&data, 2);
    return data;
}


uint32_t PalFile::read32()
{
    uint32_t data;
    m_file->read((char*)&data, 4);
    return data;
}


void PalFile::write8(uint8_t value)
{
    m_file->write((char*)&value, 1);
}


void PalFile::write16(uint16_t value)
{
    m_file->write((char*)&value, 2);
}


void PalFile::write32(uint32_t value)
{
    m_file->write((char*)&value, 4);
}


int64_t PalFile::getSize()
{
    return m_file->size();
}


void PalFile::seek(int position)
{
    m_file->seek(position);
}


void PalFile::skip(int len)
{
    m_file->seek(m_file->pos() + len);
}


int64_t PalFile::getPos()
{
    return m_file->pos();
}


bool PalFile::eof()
{
    return m_file->atEnd();
}
