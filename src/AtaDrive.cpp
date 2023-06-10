/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2018-2023
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

#include "Globals.h"
#include "AtaDrive.h"
#include "DiskImage.h"
#include "Emulation.h"

using namespace std;


/*AtaDrive::AtaDrive()
{
    //
}*/


void AtaDrive::assignDiskImage(DiskImage* image)
{
    m_image = image;

    if (image)
        image->setOwner(this);
}


void AtaDrive::reset()
{
    m_cs = 0;
    m_addr = 0;
    m_rdData = 0xFFFF;
    m_wrData = 0;
    m_dataReg = 0;
    m_ior = false;
    m_iow = false;
    m_reset = false;

    m_dev = 0;
    m_useLba = false;
    m_cylinder = 0;
    m_head = 0;
    m_sector = 1;
    m_lbaAddress = 0;
    m_sectorCount = 0;
    m_lastCommand = 0;
    m_prefilledData = false;
}


void AtaDrive::writeData(uint16_t value)
{
    m_wrData = value;
}


uint16_t AtaDrive::readData()
{
    if (m_addr != 0 || m_cs != 1)
        return m_rdData;
    else
        return m_dataReg;
}

void AtaDrive::writeControl(int cs, int addr, bool ior, bool iow, bool rst)
{
    // workaround for DSDOS
    if (m_ior && ior && addr == 0 && cs == 1 && m_addr != 0)
        m_ior = false;

    m_cs = cs;
    m_addr = addr;

    if (!m_reset && rst)
        reset();

    if (m_cs == 0 || m_cs == 3)
        m_rdData = 0xFFFF;

    if (!m_ior && ior && addr == 0 && cs == 1) {
        if (m_dataCounter) {
            if (!m_prefilledData && !(m_dataCounter & 0xFF)) {
                m_image->read(m_sectorBuf, 512);
                m_dataPtr = m_sectorBuf;
            }
            m_dataReg = m_rdData = m_dataPtr[0] + (m_dataPtr[1] << 8);
            m_dataPtr += 2;
            m_dataCounter--;
        } else
            m_dataReg = m_rdData = 0xFFFF;
        }

    if (ior) {
        switch(addr) {
        case 0:
            break;
        case 1:
            // error register
            m_rdData = 0x00; // except diagnostic !
            break;
        case 7:
            // status register
            m_rdData = (m_image->getImagePresent() ? 0x40 : 0x20) | (m_dataCounter ? 0x08 : 0) | (m_dev != 0 ? 0x80 : 0) | 0x10; // DRDY & DSC & DRQ if data transfer is active, DF if no disk
            break;
        default:
            m_rdData = 0xFFFF;
            break;
        }
    }

    if (!m_iow && iow)
        writeReg(m_addr, m_wrData);

    m_ior = ior;
    m_iow = iow;
    m_reset = rst;
}


void AtaDrive::writeReg(int reg, uint16_t value)
{
    reg &= 0x7;

    switch(reg) {
    case 0:
        // data port
        if (m_dataCounter) {
            *(m_dataPtr++) = value & 0xFF;
            *(m_dataPtr++) = value >> 8;
            if (!(--m_dataCounter & 0xFF) && !m_image->getWriteProtectStatus()) {
                m_image->write(m_sectorBuf, 512);
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

        // only drive 0 for now
        if (m_dev != 0)
            break;

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


uint16_t AtaDrive::readReg(int addr)
{
    writeControl(1, addr, true, false, false);
    writeControl(1, addr, false, false, false);
    return readData();
}


uint8_t AtaDrive::readStatus()
{
    return (m_image->getImagePresent() ? 0x40 : 0x20) | (m_dataCounter ? 0x08 : 0) | (m_dev != 0 ? 0x80 : 0) | 0x10;
}


bool AtaDrive::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "diskImage") {
        assignDiskImage(static_cast<DiskImage*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "geometry") {
        if (values[0].asString() == "lba") {
            m_lba = true;
        } else  if (values[0].asString() == "vector") {
            //m_forceLba = false;
            m_vectorGeometry = true;
            setVectorGeometry();
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

    //putWord(0, 0x0040);
  putWord(0, 0x55aa);
    if (!m_lba) {
        putWord(1, m_cylinders);
        putWord(3, m_heads);
        putWord(6, m_sectors);
    }
    putStr(10, "0000000000");
    putStr(23, "v.1.00");
    putStr(27, "Emu80 Virtual ATA Controller");
    //putWord(49, m_lba ? 0x0200 : 0);
    //putWord(49, 0x030);
    putWord(49, m_lba ? 0x0202 : 0);

    //putWord(50, 0x4000);
    //putWord(51, 0x0002);

    if (m_image->getImagePresent()) {
        uint32_t size = m_image->getSize() / 512;
        putWord(57, size & 0xFFFF);
        putWord(58, size >> 16);
        if (m_lba) {
            putWord(60, size & 0xFFFF);
            putWord(61, size >> 16);
        }
    }

    m_dataPtr = m_sectorBuf;
    m_dataReg = m_dataPtr[0] + (m_dataPtr[1] << 8);
    //m_dataPtr += 2;
    m_dataCounter = 256/* - 1*/;
    m_prefilledData = true;
}


void AtaDrive::readSectors()
{
    if (!m_image->getImagePresent())
        return;

    seek();

    m_prefilledData = false;
    m_dataCounter = 256 * m_sectorCount;
}


void AtaDrive::writeSectors()
{
    if (!m_image->getImagePresent())
        return;

    seek();
    m_dataCounter = 256 * m_sectorCount;
    m_dataPtr = m_sectorBuf;
}


void AtaDrive::putWord(int wordOffset, uint16_t word)
{
    m_sectorBuf[wordOffset * 2] = word & 0xFF;
    m_sectorBuf[wordOffset * 2 + 1] = word >> 8;
}


void AtaDrive::putStr(int wordOffset, const char* str)
{
    int i = 0;
    for (uint8_t* ptr = m_sectorBuf + wordOffset * 2; *str; str++, i++)
        ptr[i^1] = *str;
}


void AtaDrive::seek()
{
    if (m_useLba || m_lba)
        m_image->setCurOffset(m_lbaAddress * 512);
    else
        m_image->setCurOffset(((m_sector - 1) + (m_head + m_cylinder * m_heads) * m_sectors) * 512);
}


void AtaDrive::setVectorGeometry()
{
    if (!m_image)
        return;

    if (!m_image->getImagePresent()) {
        m_lba = true;
        return;
    }

    m_lba = false;
    m_image->setCurOffset(0x80);
    m_sectors = m_image->read8();
    m_heads = m_image->read8();
    uint8_t cylLo = m_image->read8();
    uint8_t cylHi = m_image->read8();
    m_cylinders = cylHi << 8 | cylLo;
    m_image->setCurOffset(0);
}


void AtaDrive::notify(EmuObject* sender, int data)
{
    if (sender == m_image && data == DISKIMAGE_NOTIFY_FILEOPENED) {
        if (m_image->getImagePresent()) {
            int64_t size = m_image->getSize();
            size /= 512;
            if (size / 512 > 0xFFFFFFFF) {
                m_image->close();
            }
            if (m_vectorGeometry)
                setVectorGeometry();
        }
    }
}
