/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2026
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

// Pci8251.cpp

// Implementation of Programmable Communication Interface i8251 (RJ580VV51)

#include "EmuObjects.h"
#include "Pci8251.h"


void Pci8251::initConnections()
{
    AddressableDevice::initConnections();

    // "data" output and input carry a whole byte per setValue() call, there is no separate strobe
    REG_OUTPUT("data", m_dataOutput);
    REG_OUTPUT("dtr", m_dtrOutput);
    REG_OUTPUT("rts", m_rtsOutput);
    REG_OUTPUT("txrdy", m_txrdyOutput);
    REG_OUTPUT("rxrdy", m_rxrdyOutput);

    REG_INPUT("data", Pci8251::setData);
    REG_INPUT("dsr", Pci8251::setDsr);
    REG_INPUT("cts", Pci8251::setCts);
}


void Pci8251::reset()
{
    internalReset();
}


// Common reset used by hardware reset and the IR command
void Pci8251::internalReset()
{
    m_idle = true;
    m_outBufferLen = 0;
    m_inDataWasRead = true;
    m_txen = false;
    m_rxe = false;
    m_oe = false;
    m_dtr = false;
    m_rts = false;

    updateOutputs();
    m_rxrdyOutput->setValue(false);
}


void Pci8251::writeByte(int addr, uint8_t value)
{
    addr &= 0x01;
    switch (addr) {
    case 0:
        // data
        if (m_idle)
            break;

        // If the buffer is full, the last byte gets overwritten
        if (m_outBufferLen == 2)
            m_outBuffer[1] = value & m_bitMask;
        else
            m_outBuffer[m_outBufferLen++] = value & m_bitMask;
        // Drop TxRDY so that the next transmission produces a rising edge
        updateTxRdy();
        tryTransmit();
        break;
    case 1:
        if (m_idle) {
            // mode instruction
            // Synchronous mode is not supported: sync characters following the mode instruction
            // are not skipped and get treated as command instructions
            m_bitMask = (1 << (((value >> 2) & 3) + 5)) - 1;
            m_idle = false;
        } else {
            // command instruction
            if (value & 0x40) {
                internalReset();
                break;
            }
            m_dtr = !!(value & 0x02);
            m_rts = !!(value & 0x20);

            m_txen = value & 0x01;
            // Simplification: on a real chip RxE = 0 also masks RxRDY (pin and status bit)
            // if a received byte is still pending; here it stays set until the byte is read
            m_rxe = value & 0x04;

            if (value & 0x10)
                m_oe = false;

            updateOutputs();

            tryTransmit();
        }
        break;
    }
}



uint8_t Pci8251::readByte(int addr)
{
    addr &= 1;
    switch (addr) {
    case 0:
    {
        m_inDataWasRead = true;
        uint8_t val = m_inData;
        m_rxrdyOutput->setValue(false);
        return val;
    }
    case 1:
    default:
        return (m_dsr ? 0x80 : 0) | (m_oe ? 0x10 : 0)| (m_outBufferLen == 0 ? 0x04 : 0) | (!m_inDataWasRead ? 0x02 : 0) | (m_outBufferLen == 0 ? 0x01 : 0);
    }
}


void Pci8251::setData(uint8_t val)
{
    // Received bytes are dropped while the chip is not initialized or the receiver is disabled
    if (m_idle || !m_rxe)
        return;

    m_inData = val & m_bitMask;
    if (!m_inDataWasRead)
        m_oe = true;
    m_inDataWasRead = false;
    m_rxrdyOutput->setValue(true);
}


void Pci8251::setDsr(bool val)
{
    m_dsr = val;
}


void Pci8251::setCts(bool val)
{
    m_cts = val;
    updateTxRdy();
    tryTransmit();
}


void Pci8251::tryTransmit()
{
    if (m_outBufferLen == 0 || !m_txen || !m_cts)
        return;

    // Transmission is instant, so send everything buffered
    for (int i = 0; i < m_outBufferLen; i++)
        m_dataOutput->setValue(m_outBuffer[i]);
    m_outBufferLen = 0;

    updateTxRdy();
}


void Pci8251::updateTxRdy()
{
    m_txrdyOutput->setValue(m_outBufferLen == 0 && m_cts && m_txen);
}


void Pci8251::updateOutputs()
{
    m_dtrOutput->setValue(m_dtr);
    m_rtsOutput->setValue(m_rts);
    updateTxRdy();
}
