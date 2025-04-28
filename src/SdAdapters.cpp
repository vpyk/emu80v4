/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2020-2022
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
#include "SdAdapters.h"
#include "SdCard.h"

using namespace std;

// SdAdapter

bool SdAdapter::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (AddressableDevice::setProperty(propertyName, values))
        return true;

    if (propertyName == "type") {
        if (values[0].asString() == "hwmpvv")
            m_type = SDA_HWMPVV;
        else if (values[0].asString() == "msx")
            m_type = SDA_MSX;
        else if (values[0].asString() == "n8vem")
            m_type = SDA_N8VEM;
        else
            return false;
        return true;
    } else if (propertyName == "sdCard") {
        attachSdCard(static_cast<SdCard*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


void SdAdapter::reset()
{
    m_readValue = 0xFF;
    m_valueToWrite = 0;
    m_bitCnt = 0;
    m_prevClk = 0;
}


void SdAdapter::writeByte(int addr, uint8_t value)
{
    if (!m_sdCard)
        return;

    if (addr == 0)
        writeDataPort(value);
    else
        writeConfPort(value);
};


uint8_t SdAdapter::readByte(int addr)
{
    if (!m_sdCard)
        return 0xFF;

    if (addr == 0)
        return readDataPort();
    else
        return m_type == SDA_HWMPVV ? 0 : 0xFF;
}


void SdAdapter::writeConfPort(uint8_t value)
{
    if (m_type == SDA_HWMPVV)
        m_sdCard->setCs(value & 1);
}


void SdAdapter::writeDataPort(uint8_t value)
{
    switch (m_type) {
    case SDA_HWMPVV:
        m_readValue = m_sdCard->io(value);
        break;
    case SDA_N8VEM:
    {
        m_sdCard->setCs(value & 0x04);
        bool clk = value & 0x02;
        if (!m_prevClk && clk) {
            m_valueToWrite <<= 1;
            m_valueToWrite |= (value & 1);
            m_bitCnt++;
            if (m_bitCnt == 8) {
                m_readValue = m_sdCard->io(m_valueToWrite);
                m_bitCnt = 0;
            }
        }
        m_prevClk = clk;
        break;
    }
    case SDA_MSX:
        m_valueToWrite <<= 1;
        if (value & 0x80)
            m_valueToWrite |= 1;
        m_bitCnt++;
        if (m_bitCnt == 8) {
            m_readValue = m_sdCard->io(m_valueToWrite);
            m_bitCnt = 0;
        }
        break;
    default:
        break;
    }
}


uint8_t SdAdapter::readDataPort()
{
    if (m_type == SDA_N8VEM)
        return m_readValue & (0x80 >> m_bitCnt) ? 0x80 : 0;
    else // SDA_HWMPVV, SDA_MSX
        return m_readValue;
}


// PpiSdAdapter

bool PpiSdAdapter::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "type") {
        if (values[0].asString() == "hwmpvv")
            m_type = SDA_HWMPVV;
        else if (values[0].asString() == "msx")
            m_type = SDA_MSX;
        else if (values[0].asString() == "n8vem")
            m_type = SDA_N8VEM;
        else
            return false;
        return true;
    }

    if (propertyName == "sdCard") {
        attachSdCard(static_cast<SdCard*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


void PpiSdAdapter::reset()
{
    m_readValue = 0xFF;
    m_valueToWrite = 0;
    m_prevWr = 0;
    m_bitCnt = 0;
    m_prevClk = 0;
}


void PpiSdAdapter::attachSdCard(SdCard* card)
{
    m_sdCard = card;
    m_sdCard->setCs(true);
}


void PpiSdAdapter::setPortB(uint8_t value)
{
    // data port
    if (!m_sdCard)
        return;

    switch (m_type) {
    case SDA_HWMPVV:
        m_valueToWrite = value;
        break;
    default:
        break;
    }
}


void PpiSdAdapter::setPortC(uint8_t value)
{
    // conf port
    if (!m_sdCard)
        return;

    switch (m_type) {
    case SDA_HWMPVV: {
        bool wr = value & 0x10;
        if (!m_prevWr && wr && !(value & 0x08)) {
            // CS_WR front & !CS_A0
            m_readValue = m_sdCard->io(m_valueToWrite);
        }
        m_prevWr = wr;
        break;
    }
    case SDA_N8VEM:
    {
        bool clk = value & 0x20;
        if (!m_prevClk && clk) {
            m_valueToWrite <<= 1;
            m_valueToWrite |= (value & 1);
            m_bitCnt++;
            if (m_bitCnt == 8) {
                m_readValue = m_sdCard->io(m_valueToWrite);
                m_bitCnt = 0;
            }
        }
        m_prevClk = clk;
    }
        break;
    default:
        break;
    }
}


uint8_t PpiSdAdapter::getPortA()
{
    switch (m_type) {
    case SDA_HWMPVV:
        return m_readValue;
    case SDA_N8VEM:
        return m_readValue & (0x80 >> m_bitCnt) ? 0xFF : 0;
    default:
        return 0xFF;
    }
}
