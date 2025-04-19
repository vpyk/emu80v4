/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2025
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

#ifndef RKSDCONTROLLER_H
#define RKSDCONTROLLER_H


#include <string>
#include <list>

#include "Pal.h"
#include "PalFile.h"

#include "Ppi8255Circuit.h"


class RkSdController : public Ppi8255Circuit
{
    public:
        RkSdController(std::string sdDir);
        ~RkSdController() override;

        void reset() override;
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        uint8_t getPortA() override;
        uint8_t getPortB() override {return 0xff;}
        uint8_t getPortC() override;
        void setPortA(uint8_t value) override;
        void setPortB(uint8_t value) override;
        void setPortC(uint8_t value) override;

        static EmuObject* create(const EmuValuesList& parameters) {return new RkSdController(parameters[0].asString());}

    private:

    enum CommandStage {
        CS_WAIT40,
        CS_WAIT0,
        CS_WAIT13,
        CS_WAITB4,
        CS_WAIT57,
        CS_START,  // ERR_START
        CS_OKDISK, // ERR_OK_DISK
        CS_CMD,
        CS_PREPARE,
        CS_REQUEST,
        CS_ANSWER
    };

    enum ControllerCommand {
        CMD_BOOT = 0,
        CMD_VER = 1,
        CMD_EXEC = 2,
        CMD_FIND = 3,
        CMD_OPEN = 4,
        CMD_LSEEK = 5,
        CMD_READ = 6,
        CMD_WRITE = 7,
        CMD_MOVE = 8
    };

    enum FileOperation {
        O_OPEN = 0,
        O_CREATE = 1,
        O_MKDIR = 2,
        O_DELETE = 100,
        O_SWAP = 101
    };

    enum ErrorCode {
        ERR_DISK_ERR = 0x02,
        ERR_NOT_OPENED = 0x03,
        ERR_NO_PATH = 0x04,
        ERR_DIR_NOT_EMPTY = 0x07,
        ERR_FILE_EXISTS = 0x08,
        ERR_MAX_FILES = 0x0A,
        ERR_RECV_STRING = 0x0B,
        ERR_INVALID_COMMAND = 0x0C,
        ERR_START = 0x40,
        ERR_OK_DISK = 0x42,
        ERR_OK_CMD = 0x43,
        ERR_OK_READ = 0x44,
        ERR_OK_ENTRY = 0x45,
        ERR_OK_WRITE = 0x46,
        ERR_OK_ADDR = 0x47,
        ERR_OK_BLOCK = 0x4F
    };

        bool m_specModel = false;

        bool m_readOnly = false;

        CommandStage m_stage = CS_WAIT40;
        int m_subStage = 0;
        bool m_multiStageCmdPending = false;
        int m_cmd;
        std::string m_sdDir;
        uint8_t m_prevValue = 0;

        uint8_t m_outValue = 0;
        uint8_t m_inValue = 0;
        uint8_t* m_romBuffer;
        const char* m_version = "V1.0 (EMU80)    ";

        uint8_t* m_inBuffer;
        uint8_t* m_outBuffer = nullptr;
        unsigned m_inBufferPos = 0;
        unsigned m_outBufferPos = 0;
        unsigned m_outBufferSize = 0;

        bool loadRkFile(const std::string& fileName);
        uint8_t* m_progPtr;
        uint16_t m_progBegAddr;
        uint16_t m_progLen;
        uint8_t* m_execFileBuffer = nullptr;
        std::list<PalFileInfo*> m_fileList;

        PalFile m_file;

        std::string m_curFileName;
        std::string m_curFileMode = "rw";
        unsigned m_curFileSize;
        unsigned m_filePos;

        std::string m_file2Name;
        std::string m_file2Mode = "rw";
        unsigned m_file2Size;
        std::string m_srcDir;
        unsigned m_file2Pos;

        bool m_fileIsOpen = false;

        uint16_t m_bytesToWrite;
        uint16_t m_curBytesToWrite;

        void resetState();
        void createErrorAnswer(ErrorCode error);
        void error();

        bool cmd();
        bool cmdBoot();
        bool cmdVer();
        bool cmdExec();
        bool cmdFind();
        bool cmdOpen();
        bool cmdLseek();
        bool cmdRead();
        bool cmdWrite();
        bool cmdMove();
};


#endif // RKSDCONTROLLER_H


