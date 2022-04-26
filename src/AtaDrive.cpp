/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2018-2019
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

// AtaDrive.cpp
// Basic implementation of ATA drive
// Command EC (identify), 20/21 (read) and 30/31 (write) for now
// Only first device and only LBA

#include <string.h>

#include "AtaDrive.h"
#include "Emulation.h"

using namespace std;


/*AtaDrive::AtaDrive()
{
    //
}*/


bool AtaDrive::assignFileName(string fileName)
{
    m_fileName = fileName;

    fileName = palMakeFullFileName(fileName);
    m_file.open(fileName.c_str(), m_readOnly ? "r" : "r+");

    if (!m_file.isOpen())
        return false;

    int64_t size = m_file.getSize();
    size /= 512;
    if (size / 512 > 0xFFFFFFFF)
        return false;

    if (m_vectorGeometry)
        setVectorGeometry();

    return true;
}


void AtaDrive::reset()
{
    //
}


void AtaDrive::writeReg(int reg, uint16_t value)
{
    reg &= 0x7;

    switch(reg) {
    case 0:
        // data port
        if (m_dataCounter) {
            *(m_dataPtr++) = value;
            if (!(--m_dataCounter & 0xFF) && !m_readOnly) {
                for (int i = 0; i < 256; i++) {
                    m_file.write8(m_sectorBuf[i] & 0xFF);
                    m_file.write8(m_sectorBuf[i] >> 8);
                }
                m_dataPtr = m_sectorBuf;
            }
        }
        break;
    case 1:
        // features register
        break;
    case 2:
        // sector count register
        m_sectorCount = value & 0xFF;
        break;
    case 3:
        // sector number register
        m_sector = value;
        m_lbaAddress = (m_lbaAddress & ~0x000000FF) | value;
        break;
    case 4:
        // cylinder low register
        m_cylinder = (m_cylinder & ~0x00FF) | value;
        m_lbaAddress = (m_lbaAddress & ~0x0000FF00) | (value << 8);
        break;
    case 5:
        // cylinder high register
        m_cylinder = (m_cylinder & ~0xFF00) | (value << 8);
        m_lbaAddress = (m_lbaAddress & ~0x00FF0000) | (value << 16);
        break;
    case 6:
        // device/head register
        m_head = value & 0xF;
        m_useLba = value & 0x40;
        m_dev = (value & 0x10) >> 4;
        m_lbaAddress = (m_lbaAddress & ~0x0F000000) | ((value & 0xF) << 24);
        break;
    case 7:
        // command register
        m_lastCommand = value; // for future use
        switch (value) {
        case 0xEC:
            identify();
            break;
        case 0x20:
        case 0x21:
            readSectors();
            break;
        case 0x30:
        case 0x31:
            writeSectors();
            break;
        }
        break;
    }
}


uint16_t AtaDrive::readReg(int reg)
{
    reg &= 0x7;

    switch(reg) {
    case 0:
        // data port
        if (m_dataCounter) {
            if (!m_prefilledData && !(m_dataCounter-- & 0xFF)) {
                for (int i = 0; i < 256; i++) {
                    uint8_t lsb = m_file.read8();
                    uint8_t msb = m_file.read8();
                    m_sectorBuf[i] = lsb | (msb << 8);
                }
                m_dataPtr = m_sectorBuf;
            }
            return *m_dataPtr++;
        }
        return 0;
    case 1:
        // error register
        return 0x00; // except diagnostic !
    case 7:
        // status register
        return (m_file.isOpen() ? 0x40 : 0x20) | (m_dataCounter ? 0x08 : 0) | 0x10; // DRDY & DSC & DRQ if data transfer is active, DF if no disk
    default:
        return 0;
    }
}


void AtaDrive::setReadOnly(bool ro)
{
    if (ro != m_readOnly) {
        m_readOnly = ro;
        if (m_fileName != "")
            assignFileName(m_fileName);
    }
}


bool AtaDrive::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "imageFile")
        return assignFileName(values[0].asString());
    else if (propertyName == "readOnly") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            setReadOnly(values[0].asString() == "yes");
            return true;
        }
    } else if (propertyName == "geometry") {
        if (values[0].asString() == "lba") {
            m_lba = true;
        } else  if (values[0].asString() == "vector") {
            //m_forceLba = false;
            m_vectorGeometry = true;
        } else {
            m_lba = false;
            m_cylinders = values[0].asInt();
            m_heads = values[1].asInt();
            m_sectors = values[2].asInt();
        }
        return true;
    }

    return false;
}


void AtaDrive::identify()
{
    memset(m_sectorBuf, 0, 512);

    putWord(0, 0x0040);
    if (!m_lba) {
        putWord(1, m_cylinders);
        putWord(3, m_heads);
        putWord(6, m_sectors);
    }
    putStr(10, "0000000000");
    putStr(23, "v.1.00");
    putStr(27, "Emu80 Virtual ATA Controller");
    putWord(49, 0x0030);
    putWord(50, 0x4000);
    putWord(51, 0x0002);

    if (m_file.isOpen()) {
        uint32_t size = m_file.getSize() / 512;
        putWord(57, size & 0xFFFF);
        putWord(58, size >> 16);
        if (m_lba) {
            putWord(60, size & 0xFFFF);
            putWord(61, size >> 16);
        }
    }

    m_dataPtr = m_sectorBuf;
    m_dataCounter = 256;
    m_prefilledData = true;
}


void AtaDrive::readSectors()
{
    if (!m_file.isOpen())
        return;

    seek();

    //m_dataPtr = m_sectorBuf;
    m_dataCounter = 256 * m_sectorCount;
    m_prefilledData = false;
}


void AtaDrive::writeSectors()
{
    if (!m_file.isOpen())
        return;

    seek();
    //m_dataPtr = m_sectorBuf;
    m_dataCounter = 256 * m_sectorCount;
}


void AtaDrive::putWord(int wordOffset, uint16_t word)
{
    m_sectorBuf[wordOffset] = word;
}


void AtaDrive::putStr(int wordOffset, const char* str)
{
    int i = 0;
    for (uint16_t* ptr = m_sectorBuf + wordOffset; *str; str++, i++)
        if (i & 1)
            ptr[i / 2] |= (uint8_t)*str;
        else
            ptr[i / 2] = (uint8_t)*str << 8;
}


void AtaDrive::seek()
{
    if (m_useLba || m_lba)
        m_file.seek(m_lbaAddress * 512);
    else
        m_file.seek(((m_sector - 1) + (m_head + m_cylinder * m_heads) * m_sectors) * 512);
}


void AtaDrive::setVectorGeometry()
{
    if (!m_file.isOpen()) {
        m_lba = true;
        return;
    }

    m_lba = false;
    m_file.seek(0x80);
    m_sectors = m_file.read8();
    m_heads = m_file.read8();
    m_cylinders = m_file.read16();
    m_file.seek(0);
}
