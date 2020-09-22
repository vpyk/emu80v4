/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2020
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

// SdCard.cpp
// Simplified implementation of SD Card

#include <string.h>

#include "SdCard.h"
#include "Emulation.h"

using namespace std;


bool SdCard::assignFileName(string fileName)
{
    m_fileName = fileName;

    fileName = palMakeFullFileName(fileName);
    m_file.open(fileName.c_str(), m_readOnly ? "r" : "r+");

    if (!m_file.isOpen())
        return false;

    return true;
}


void SdCard::reset()
{
    sdInit();
}


void SdCard::sdInit()
{
    m_state = SDSTATE_WAITING_FOR_COMMAND;
    m_answerLength = 0;
    m_dataCounter = 0;
}


uint8_t SdCard::io(uint8_t value)
{
    if (!m_cs)
        return 0xFF;

    if (m_state == SDSTATE_WAITING_FOR_COMMAND && !(value & 0x80)) {
        // CMD start
        m_state = SDSTATE_CMD;
        m_dataCounter = 0;
    }

    switch (m_state) {
    case SDSTATE_CMD:
        // recv cmd
        m_cmdBuf[m_dataCounter++] = value;
        if (m_dataCounter == 6) {
            m_dataCounter = 0;
            m_state = SDSTATE_ANSWER;
            executeCmd();
        }
        return 0xFF;
    case SDSTATE_ANSWER:
    {
        // answer
        if (m_answerLength == 0)
            return 0xFF;
        uint8_t answer = m_answerBuf[m_dataCounter++];
        if (m_dataCounter >= m_answerLength) {
            m_dataCounter = 0;
            m_answerLength = 0;
            m_state = SDSTATE_WAITING_FOR_COMMAND;
        }
        return answer;
    }
    case SDSTATE_WRITE:
        if (!m_startBlockFound) {
            if (value == 0xFE) {
                m_dataCounter = 0;
                m_startBlockFound = true;
            }
        } else {
            m_dataBuf[m_dataCounter++] = value;
            if (m_dataCounter == 512 + 2) {
                m_file.seek(m_blockAddr);
                for (int i = 0; i < 512; i++)
                    m_file.write8(m_dataBuf[i]);
                m_state = SDSTATE_WAITING_FOR_COMMAND;
            }
        }
        return 0;
    default:
        return 0xFF;
    }
}


void SdCard::setReadOnly(bool ro)
{
    if (ro != m_readOnly) {
        m_readOnly = ro;
        if (m_fileName != "")
            assignFileName(m_fileName);
    }
}


bool SdCard::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "imageFile")
        return assignFileName(values[0].asString());
    else if (propertyName == "readOnly")
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            setReadOnly(values[0].asString() == "yes");
            return true;
    }

    return false;
}


void SdCard::executeCmd()
{
    uint8_t cmd = m_cmdBuf[0] & 0x3F;
    uint32_t arg = (m_cmdBuf[1] << 24) | (m_cmdBuf[2] << 16) | (m_cmdBuf[3] << 8) | m_cmdBuf[4];

    switch(cmd) {
    case 0:
        // GO_IDLE_STATE
        cmd0();
        break;
    case 1:
        // SEND_OP_COND
        cmd1();
        break;
    case 8:
        // SEND_IF_CONF
        cmd8();
        break;
    case 9:
        // SEND_SCD
        break;
    case 10:
        // SEND_CID
        break;
    case 16:
        break;
    case 17:
        // READ_SINGLE_BLOCK
        cmd17(arg);
        break;
    case 24:
        // WRITE_BLOCK
        cmd24(arg);
        break;
    case 41:
        break;
    case 55:
        // APP_CMD
        break;
    case 58:
        break;
    case 59:
        break;
    default:
        r1(SDS_ILLEGAL_COMMAND);
        break;
    }
}


void SdCard::r1(uint8_t state)
{
    m_answerBuf[0] = 0xFF;
    m_answerBuf[1] = state;
    m_answerLength = 2;
}


// CMD0 - GO_IDLE_STATE (reset)
void SdCard::cmd0()
{
    r1(SDS_IN_IDLE_STATE);
}


// CMD1 - SEND_OP_COND
void SdCard::cmd1()
{
    r1(0);
}


// CMD17 - READ_SINGLE_BLOCK
void SdCard::cmd17(uint32_t arg)
{
    r1(0);
    m_answerBuf[m_answerLength++] = 0xFF; // ???
    m_answerBuf[m_answerLength++] = 0xFE; // ???
    m_file.seek(arg);
    for (int i = 0; i < 512; i++)
        m_answerBuf[m_answerLength++] = m_file.read8();
    m_answerBuf[m_answerLength++] = 0; // crc
    m_answerBuf[m_answerLength++] = 0; // crc
}


// CMD24 - WRITE_BLOCK
void SdCard::cmd24(uint32_t arg)
{
    m_state = SDSTATE_WRITE;
    m_startBlockFound = false;
    m_blockAddr = arg;
    //r1(0);
}


// CMD8 - SEND_IF_COND (SD ver 2 only)
void SdCard::cmd8()
{
    r1(SDS_ILLEGAL_COMMAND);

    /*if (m_sdVersion == 2) {
        uint8_t pattern = m_cmdBuf[4];
        m_answerBuf[0] = SDS_IN_IDLE_STATE;
        m_answerBuf[1] = 0x00;
        m_answerBuf[2] = 0x00;
        m_answerBuf[3] = 0x01; // 2.7-3.6 V
        m_answerBuf[4] = pattern;
        m_answerLength = 5;
    }*/
}

