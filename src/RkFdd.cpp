/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2022
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

#include "Globals.h"
#include "Emulation.h"
#include "RkFdd.h"
#include "DiskImage.h"

using namespace std;


uint8_t RkFddRegister::readByte(int)
{
    m_fdd->updateState();
    return m_fdd->readCurByte();
}


RkFddController::RkFddController()
{
    m_images[0] = nullptr;
    m_images[1] = nullptr;

    m_ticksPerByte = g_emulation->getFrequency() / 125000 * 8;

    m_drive = 0;
    m_track = 0;
    m_side = 0;

    m_dirFwd = true;

    m_ready = true; //!!!
}


RkFddController::~RkFddController()
{

}


void RkFddController::attachFdImage(int driveNum, FdImage* image)
{
    if (driveNum < 2)
        m_images[driveNum] = image;
}


uint8_t RkFddController::getPortB()
{
    updateState();
    uint8_t res = 0xFF;
    if (m_ready)
        res &= ~0x10;
    if (m_track == 0)
        res &= ~0x20;
    if (m_index)
        res &= ~0x40;
    if (!m_nextByteReady)
        res &= ~0x80;
    if (m_images[m_drive] && m_images[m_drive]->getWriteProtectStatus())
        res &= ~0x08;
    return res;
}


void RkFddController::setPortA(uint8_t value)
{
    updateState();
    if (m_write)
        writeCurByte(value);
}


void RkFddController::setPortC(uint8_t value)
{
    updateState();
    m_drive = value & 0x8 ? 0 : 1;
    m_write = !(value & 1);
    m_dirFwd = !(value & 2);
    m_side = (value & 4) >> 2;
    m_step = !(value & 0x10);
    if (m_step && !m_prevStep) {
        if (m_dirFwd && m_track < 79)
            m_track++;
        else if (!m_dirFwd && m_track > 0)
            m_track--;
    }
    m_prevStep = m_step;

    if (m_images[m_drive]) {
        m_images[m_drive]->setCurHead(m_side);
        m_images[m_drive]->setCurTrack(m_track);
        m_images[m_drive]->startSectorAccess(0);
    }
}


void RkFddController::updateState()
{
    uint64_t clock = g_emulation->getCurClock() / m_ticksPerByte;
    m_pos = clock % 3125;
    m_nextByteReady = clock != m_prevClock;
    m_prevClock = clock;
    m_index = m_pos < 47; // 3 ms
}


uint8_t RkFddController::readCurByte()
{
    updateState();
    m_nextByteReady = false;
    return m_images[m_drive]->readByte(m_pos);
}


void RkFddController::writeCurByte(uint8_t bt)
{
    updateState();
    m_nextByteReady = false;
    m_images[m_drive]->writeByte(m_pos, bt);
}


bool RkFddController::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "fddRegister") {
        attachRkFddRegister(static_cast<RkFddRegister*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "fdImage" && values[0].isInt()) {
        attachFdImage(values[0].asInt(), static_cast<FdImage*>(g_emulation->findObject(values[1].asString())));
        return true;
    }
    return false;
}
