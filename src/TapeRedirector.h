/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2018
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

#ifndef FILEREDIRECT_H
#define FILEREDIRECT_H

#include "PalFile.h"

#include "WavWriter.h"


// Файловый редиректор. Обеспечивает перенаправление ф файл или из файла различных обращений эмулируемой платформы
class TapeRedirector : public EmuObject
{
    public:
        TapeRedirector();
        virtual ~TapeRedirector();

        void reset() override;
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        void openFile();
        void closeFile();
        void setFilePos(unsigned pos);

        void assignFile(std::string fileName, std::string rwMode);
        uint8_t readByte();
        uint8_t peekByte();
        void writeByte(uint8_t bt);
        bool waitForSequence(const uint8_t* seq, int len);
        uint8_t readByteSkipSeq(const uint8_t* seq, int len);
        int getPos();
        bool isEof();
        bool isOpen();
        bool isCancelled();

        static EmuObject* create(const EmuValuesList&) {return new TapeRedirector();}

    private:
        std::string m_fileName;
        std::string m_permanentFileName;
        std::string m_filter;
        std::string m_rwMode;

        PalFile m_file;
        bool m_isOpen = false;
        bool m_cancelled = false;
        bool m_read = false;

        WavWriter* m_wavWriter = nullptr;
};


#endif // FILEREDIRECT_H
