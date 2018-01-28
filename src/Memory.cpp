/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2017
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

#include <string.h>

#include "Memory.h"
#include "Pal.h"

using namespace std;

// Ram implementation

/*Ram::Ram()
{
    m_buf = nullptr;
    m_extBuf = nullptr;
}*/



Ram::Ram(unsigned memSize)
{
    m_extBuf = nullptr;
    m_buf = new uint8_t [memSize];
    memset(m_buf, 0, memSize);
    m_size = memSize;
}



Ram::Ram(uint8_t* buf, unsigned memSize)
{
    m_extBuf = buf;
    m_buf = buf;
    m_size = memSize;
}



/*Ram::Ram(unsigned memSize, string fileName)
{
    m_buf = new uint8_t [memSize];
    m_size = memSize;
    palReadFromFile(fileName, 0, memSize, m_buf);
    //if (palReadFromFile(fileName, 0, memSize, _buf) != memSize) {
    //    delete[] _buf;
    //    _buf = nullptr;
    //}
}*/



Ram::~Ram()
{
    if (!m_extBuf)
        delete[] m_buf;
}



void Ram::writeByte(int addr, uint8_t value)
{
    if (m_addrMask)
        addr &= m_addrMask;
    if (m_buf && addr < m_size)
        m_buf[addr] = value;
}



uint8_t Ram::readByte(int addr)
{
    if (m_addrMask)
        addr &= m_addrMask;
    if (m_buf && addr < m_size)
        return m_buf[addr];
    else
        return 0xFF;
}



// Rom implementation

Rom::Rom()
{
    m_buf = nullptr;
    m_size = 0;
}



Rom::Rom(unsigned memSize, string fileName)
{
    m_buf = new uint8_t [memSize];
    m_size = memSize;
    if (palReadFromFile(fileName, 0, memSize, m_buf) == 0/*!= memSize*/) {
        delete[] m_buf;
        m_buf = nullptr;
    }
}



Rom::~Rom()
{
    if (!m_buf)
        delete[] m_buf;
}



uint8_t Rom::readByte(int addr)
{
    if (m_addrMask)
        addr &= m_addrMask;
    if (m_buf && addr < m_size)
        return m_buf[addr];
    else
        return 0xFF;
}
