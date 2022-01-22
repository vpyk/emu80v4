/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2022
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

#ifndef PRNWRITER_H
#define PRNWRITER_H

#include "PalFile.h"

#include "EmuObjects.h"


class PrnWriter : public EmuObject
{
    public:
        ~PrnWriter() override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        void setPermanentFileName(const std::string& fileName) {m_permanentFileName = fileName;}
        void startPrinting();
        void stopPrinting();
        void printByte(uint8_t bt);
        bool isPrinting() {return m_isOpen;}
        bool getReady() {return m_isOpen;}
        const std::string& getFileName() {return m_fileName;}

    private:
        PalFile m_file;
        std::string m_permanentFileName;
        std::string m_fileName;
        bool m_isOpen = false;

        void reportError(const std::string& errorStr);
};


#endif // PRNWRITER_H
