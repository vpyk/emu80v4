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

#include "Pal.h"
#include "Globals.h"
#include "DiskImage.h"
#include "Emulation.h"
#include "Platform.h"
#include "EmuWindow.h"

using namespace std;

DiskImage::DiskImage()
{
    //
}


DiskImage::~DiskImage()
{
    if (m_file.isOpen())
        m_file.close();
}


bool DiskImage::assignFileName(string fileName)
{
    if (m_file.isOpen() && fileName == m_fileName)
        return true;

    m_file.close();

    if (!fileName.empty()) {
        m_fileName = palMakeFullFileName(fileName);
        m_file.open(m_fileName.c_str(), m_isWriteProtected ? "r" : "r+");
    }

    if (!m_file.isOpen())
        m_fileName = "";

    reset();

    if (m_file.isOpen() && m_owner)
        m_owner->notify(this, DISKIMAGE_NOTIFY_FILEOPENED);

    return m_file.isOpen();
}


void DiskImage::chooseFile()
{
    string fileName = palOpenFileDialog("Open floppy disk image file", m_filter, false, m_platform->getWindow());
    g_emulation->restoreFocus();
    if (fileName != "")
        assignFileName(fileName);
}


void DiskImage::close()
{
    if (m_file.isOpen())
        m_file.close();
}


int64_t DiskImage::getSize()
{
    return m_file.getSize();
}


void DiskImage::setWriteProtection(bool isWriteProtected)
{
    bool reopen = m_file.isOpen() && (m_isWriteProtected != isWriteProtected);

    m_isWriteProtected = isWriteProtected;

    if (reopen) {
        int pos = m_file.getPos();
        m_file.close();
        assignFileName(m_fileName);
        if (m_file.isOpen())
            m_file.seek(pos);
    }
}


bool DiskImage::getWriteProtectStatus()
{
    return m_isWriteProtected;
}


void DiskImage::setCurOffset(int offset)
{
    m_file.seek(offset);
}


bool DiskImage::read(uint8_t* buf, int len)
{
    if (!m_file.isOpen())
        return false;
    for (int i = 0; i < len; i++)
        buf[i] = m_file.read8();
    return true;
}


bool DiskImage::write(uint8_t* buf, int len)
{
    if (!m_file.isOpen() || m_isWriteProtected)
        return false;
    for (int i = 0; i < len; i++)
        m_file.write8(buf[i]);
    return true;
}


uint8_t DiskImage::read8()
{
    if (!m_file.isOpen())
        return 0;
    return m_file.read8();
}


bool DiskImage::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "fileName") {
        bool res = assignFileName(values[0].asString());
        if (m_autoMount)
            m_permanentFileName = m_fileName;
        return res;
    } else if (propertyName == "permanentFileName") {
        m_permanentFileName = values[0].asString();
        return m_permanentFileName.empty() ? true : assignFileName(m_permanentFileName);
    } else if (propertyName == "filter") {
        m_filter = values[0].asString();
        return true;
    } else if (propertyName == "label") {
        m_label = values[0].asString();
        return true;
    } else if (propertyName == "readOnly") {
        if (values[0].asString() == "yes")
            setWriteProtection(true);
        else if (values[0].asString() == "no")
            setWriteProtection(false);
        return true;
    } else if (propertyName == "autoMount") {
        if (values[0].asString() == "yes") {
            m_autoMount = true;
            m_permanentFileName = m_fileName;
        } else if (values[0].asString() == "no") {
            m_autoMount = false;
            m_permanentFileName = "";
        }
        return true;
    }
    return false;
}


string DiskImage::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "label")
        return m_label;
    else if (propertyName == "fileName" && m_file.isOpen())
        return m_fileName;
    else if (propertyName == "permanentFileName")
        return m_permanentFileName;
    else if (propertyName == "readOnly")
        return getWriteProtectStatus() ? "yes" : "no";
    else if (propertyName == "autoMount")
        return m_autoMount ? "yes" : "no";

    return "";
}


FdImage::FdImage(int nTracks, int nHeads, int nSectors, int sectorSize)
{
    m_nTracks = nTracks;
    m_nHeads = nHeads;
    m_nSectors = nSectors;
    m_sectorSize = sectorSize;
    m_isWriteProtected = false;

    m_curTrack = 0;
    m_curHead = 0;
    m_curSector = 0;
    m_curSectorOffset = m_sectorSize; // >= sectorSize
}


void FdImage::reset()
{
    m_curTrack = 0;
    m_curHead = 0;
    m_curSector = 0;
    m_curSectorOffset = m_sectorSize; // >= sectorSize
}


uint8_t FdImage::readNextByte()
{
    if (!m_file.isOpen())
        return 0;
    if (m_curSectorOffset == 0)
        seek(0);
    ++m_curSectorOffset;
    return m_file.read8();
}


uint8_t FdImage::readByte(int offset)
{
    if (!m_file.isOpen())
        return 0;
    seek(offset);
    return m_file.read8();
}

void FdImage::writeNextByte(uint8_t bt)
{
    if (!m_file.isOpen() || m_isWriteProtected)
        return;
    if (m_curSectorOffset == 0)
        seek(0);
    m_file.write8(bt);
    ++m_curSectorOffset;
}


void FdImage::writeByte(int offset, uint8_t bt)
{
    if (!m_file.isOpen() || m_isWriteProtected)
        return;
    seek(offset);
    m_file.write8(bt);
}


bool FdImage::getReadyStatus()
{
    return (m_file.isOpen()) && (m_curSectorOffset < m_sectorSize);
}


void FdImage::setCurTrack(int track)
{
    m_curTrack = track;
}


void FdImage::setCurHead(int head)
{
    m_curHead = head;
}


void FdImage::startSectorAccess(int sector)
{
    m_curSector = sector;
    m_curSectorOffset = 0;
}


int FdImage::readSectorAddress()
{
    m_curSector = (m_curSector + 1) % m_nSectors;
    return m_curSector;
}


void FdImage::seek(int offset = 0)
{
    int ofs = offset + (m_curSector + (m_curHead + m_curTrack * m_nHeads) * m_nSectors) * m_sectorSize;
    m_file.seek(ofs);
    m_curSectorOffset = offset;
}
