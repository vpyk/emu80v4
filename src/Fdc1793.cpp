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

// Fdc1793.cpp
// Реализация контроллера НГМД КР580ВГ93 (FDC1793)

#include "Fdc1793.h"
#include "FdImage.h"
#include "Emulation.h"
#include "Dma8257.h"

using namespace std;

Fdc1793::Fdc1793()
{
    m_accessMode = FAM_WAITING;
    m_disk = 0; // номер дисковода
    m_head = 0; // номер головки
    m_track = 0; // регистр дорожки
    m_sector = 0; // регистр сектора
    m_data = 0; // регистр данных
    m_status = 0; // регистр статусаs
    m_directionIn = true; // направление движения true=in, false=out

    for (int i = 0; i < MAX_DRIVES; i++)
        m_images[i] = nullptr;
}


Fdc1793::~Fdc1793()
{
    // dtor
}


void Fdc1793::attachFdImage(int driveNum, FdImage* image)
{
    if (driveNum < MAX_DRIVES)
        m_images[driveNum] = image;
}


void Fdc1793::attachDMA(Dma8257* dma, int channel)
{
    m_dma = dma;
    m_dmaChannel = channel;
}


void Fdc1793::reset()
{
    m_accessMode = FAM_WAITING;
    m_track = 0; // регистр дорожки
    m_sector = 1; // регистр сектора
    m_data = 0; // регистр данных
    m_directionIn = true; // направление движения true=in, false=out
}


void Fdc1793::setDrive(int drive)
{
    m_disk = drive;
}


void Fdc1793::setHead(int head)
{
    m_head = head;
}


void Fdc1793::writeByte(int addr, uint8_t value)
{
    addr &= 0x3;

    switch(addr) {
        case 0:
            // command register
            m_lastCommand = (value & 0xF0) >> 4;
            switch (m_lastCommand) {
                case 0:
                    // restore
                    m_track = 0;
                    m_directionIn = false;
                    m_accessMode = FAM_WAITING;
                    //m_busy = true;
                    m_status = 0x01;
                    generateInt();
                    break;
                case 1:
                    // seek
                    m_track = m_data;
                    if (m_track > 79)
                        m_track = 79;
                    //m_busy = true;
                    m_status = 0x01;
                    generateInt();
                    break;
                case 2:
                case 3:
                    // step
                    if (m_directionIn && m_track < 79)
                        ++m_track;
                    else if (!m_directionIn && m_track > 0)
                        --m_track;
                    //m_busy = true;
                    m_status = 0x01;
                    generateInt();
                    break;
                case 4:
                case 5:
                    // step in
                    m_directionIn = true;
                    if (m_track < 79)
                        ++m_track;
                    //m_busy = true;
                    m_status = 0x01;
                    generateInt();
                    break;
                case 6:
                case 7:
                    // step out
                    m_directionIn = false;
                    if (m_track > 0)
                        --m_track;
                    //m_busy = true;
                    m_status = 0x01;
                    generateInt();
                    break;
                case 8:
                case 9:
                    // read
                    if (!m_images[m_disk])
                        break;
                    m_images[m_disk]->setCurHead(m_head);
                    m_images[m_disk]->setCurTrack(m_track);
                    m_images[m_disk]->startSectorAccess(m_sector - 1);
                    m_accessMode = FAM_READING;
                    m_status = 0x3;
                    break;
                case 0xA:
                case 0xB:
                    // write
                    if (!m_images[m_disk])
                        break;
                    if (m_images[m_disk]->getWriteProtectStatus()) {
                        m_status = 0x40;
                        break;
                    }
                    m_images[m_disk]->setCurHead(m_head);
                    m_images[m_disk]->setCurTrack(m_track);
                    m_images[m_disk]->startSectorAccess(m_sector - 1);
                    m_accessMode = FAM_WRITING;
                    m_status = 0x3;
                    break;
                case 0xD:
                    // force interrupt
                    m_accessMode = FAM_WAITING;
                    m_status = 0x1;
                    break;
            }
            break;
        case 1:
            // track register
            m_track = value;
            break;
        case 2:
            // sector register
            m_sector = value;
            break;
        case 3:
            // data register
            m_data = value;
            //m_status &= ~2;
            if (m_images[m_disk] && m_accessMode == FAM_WRITING) {
                m_images[m_disk]->writeNextByte(m_data);
                if (!m_images[m_disk]->getReadyStatus()) {
                    if (m_lastCommand == 0xB) {
                        m_images[m_disk]->startSectorAccess(m_sector++);
                        m_status = 0x3;
                    }
                    else {
                        m_accessMode = FAM_WAITING;
                        m_status = 0x0;
                    }
                }
            }
            break;
    }
}


uint8_t Fdc1793::readByte(int addr)
{
    addr &= 0x3;

    switch(addr) {
        case 0: {
            // status register
            m_irq = false;

            if (!(m_images[m_disk]) || !m_images[m_disk]->getImagePresent())
                m_status |= 0x80;
            else
                m_status &= ~0x80;

            if (m_lastCommand <= 7 || m_lastCommand == 0x0D) {
                // index
                int trackPosMks = g_emulation->getCurClock() * 1000000 / g_emulation->getFrequency() % 200000;
                if (trackPosMks < 3000) // 3 ms
                    m_status |= 0x02;
                else
                    m_status &= ~0x02;
            }

            uint8_t res = m_status;
            switch (m_lastCommand) {
                case 0:
                    m_status = 0x24;
                    break;
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 0xD:
                    m_status = m_track == 0 ? 0x24 : 0x20;
                    break;
                case 8:
                case 9:
                    if (m_dma && m_accessMode == FAM_READING) {
                        m_status = 0;
                        m_accessMode = FAM_WAITING;
                        while (m_images[m_disk]->getReadyStatus()) {
                            m_data = m_images[m_disk]->readNextByte();
                            if (!m_dma->dmaRequest(m_dmaChannel, m_data))
                                m_status = 0x06;
                        }
                    }
                    break;
                case 0xA:
                case 0xB:
                    if (m_dma && m_accessMode == FAM_WRITING) {
                        m_status = 0;
                        m_accessMode = FAM_WAITING;
                        while (m_images[m_disk]->getReadyStatus()) {
                            if (!m_dma->dmaRequest(m_dmaChannel, m_data)) {
                                m_status = 0x06;
                                continue;
                            }
                            m_images[m_disk]->writeNextByte(m_data);
                        }
                    }
            }
            return res;
        }
        case 1:
            // track register
            return m_track;
        case 2:
            // sector register
            return m_sector;
        case 3:
            // data register
            if (m_accessMode == FAM_WAITING)
                m_status &= ~2;
            if (m_images[m_disk] && m_accessMode == FAM_READING) {
                m_data = m_images[m_disk]->readNextByte();
                if (!m_images[m_disk]->getReadyStatus()) {
                    if (m_lastCommand == 9) {
                        m_images[m_disk]->startSectorAccess(m_sector++);
                        m_status = 0x1;
                    }
                    else {
                        m_accessMode = FAM_WAITING;
                        m_status = 0x0;
                    }
                }
            }
            return m_data;
    }
    return 0xFF; // normally this not occurs
}


void Fdc1793::generateInt()
{
    m_irq = true;
    // call core.inte()
}

bool Fdc1793::getDrq()
{
    return m_accessMode != FAM_WAITING;
}



bool Fdc1793::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "fdImage" && values[0].isInt()) {
        attachFdImage(values[0].asInt(), static_cast<FdImage*>(g_emulation->findObject(values[1].asString())));
        return true;
    } else if (propertyName == "dma") {
        if (values[1].isInt()) {
            attachDMA(static_cast<Dma8257*>(g_emulation->findObject(values[0].asString())), values[1].asInt());
            return true;
        }
    }

    return false;
}
