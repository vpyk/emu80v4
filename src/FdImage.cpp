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

#include "Pal.h"
#include "FdImage.h"
#include "Emulation.h"
#include "Platform.h"
#include "EmuWindow.h"

using namespace std;

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


FdImage::~FdImage()
{
    if (m_file.isOpen())
        m_file.close();
}


void FdImage::reset()
{
    m_curTrack = 0;
    m_curHead = 0;
    m_curSector = 0;
    m_curSectorOffset = m_sectorSize; // >= sectorSize
}

bool FdImage::assignFileName(string fileName)
{
    m_fileName = palMakeFullFileName(fileName);
    m_file.open(m_fileName.c_str(), m_isWriteProtected ? "r" : "r+");

    reset();

    return (m_file.isOpen());
}


void FdImage::chooseFile()
{
    string fileName = palOpenFileDialog("Open floppy disk image file", m_filter, false, m_platform->getWindow());
    g_emulation->restoreFocus();
    if (fileName != "") {
        m_fileName = fileName;
        if (m_file.isOpen())
            m_file.close();
        m_file.open(m_fileName.c_str(), m_isWriteProtected ? "r" : "r+");

    reset();
    }
}


void FdImage::setWriteProtection(bool isWriteProtected)
{
    m_isWriteProtected = isWriteProtected;
}


bool FdImage::getReadyStatus()
{
    return (m_file.isOpen()) && (m_curSectorOffset < m_sectorSize);
}


bool FdImage::getWriteProtectStatus()
{
    return m_isWriteProtected;
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


void FdImage::seek(int offset = 0)
{
    int ofs = offset + (m_curSector + (m_curHead + m_curTrack * m_nHeads) * m_nSectors) * m_sectorSize;
    m_file.seek(ofs);
    m_curSectorOffset = offset;
}


bool FdImage::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "fileName") {
        return assignFileName(values[0].asString());
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
    }
    return false;
}


string FdImage::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "label")
        return m_label;
    else if (propertyName == "fileName" && m_file.isOpen())
        return m_fileName;

    return "";
}
