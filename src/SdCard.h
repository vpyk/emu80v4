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

// SdCard.h
// SD Card implementation, header file


#ifndef SDCARD_H
#define SDCARD_H

#include "Pal.h"
#include "PalFile.h"
#include "EmuObjects.h"


class SdCard : public EmuObject
{
enum SdStatus {
    SDS_IN_IDLE_STATE = 0x01,
    SDS_ILLEGAL_COMMAND = 0x04
};

enum SdState {
    SDSTATE_WAITING_FOR_COMMAND,
    SDSTATE_CMD,
    SDSTATE_WRITE,
    SDSTATE_ANSWER
};

public:
    //SdCard();

    bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
    void reset() override; // Chip reset

    void sdInit();
    uint8_t io(uint8_t value);
    void setCs(bool cs) {m_cs = cs;}

    bool assignFileName(std::string fileName);

    static EmuObject* create(const EmuValuesList&) {return new SdCard();}

private:
    PalFile m_file;
    bool m_readOnly = false;
    std::string m_fileName = "";

    SdState m_state = SDSTATE_WAITING_FOR_COMMAND;
    bool m_cs = true; //false
    int m_answerLength = 0;
    int m_dataCounter;
    bool m_startBlockFound = false;
    uint8_t m_cmdBuf[6];
    uint8_t m_answerBuf[512 + 6]; // 0xFF + r1 + v1 block size + crc + ?
    uint8_t m_dataBuf[512 + 2]; // v1 block size + crc
    uint32_t m_blockAddr;

    void executeCmd();

    void r1(uint8_t state);

    void cmd0();
    void cmd1();
    void cmd8();
    void cmd17(uint32_t arg);
    void cmd24(uint32_t arg);

    void setReadOnly(bool ro);

};


#endif // SDCARD_H
