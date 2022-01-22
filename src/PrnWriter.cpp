/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2022
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

//#include <sstream>

#include "Pal.h"
#include "Emulation.h"
#include "PrnWriter.h"

using namespace std;


void PrnWriter::startPrinting()
{
    stopPrinting();

    if (!m_permanentFileName.empty())
        m_fileName = m_permanentFileName;
    else {
        m_fileName = palOpenFileDialog("Save printer file", "TXT files (*.txt)|*.txt;*.TXT|PRN files (*.prn)|*.prn;*.PRN", true);
        g_emulation->restoreFocus();
        if (m_fileName.empty())
            return;
    }

    if (!m_file.open(m_fileName, "w")) {
        reportError("Can't open file " + m_fileName + " for writing!\n");
        m_fileName = "";
        return;
    }

    m_isOpen = true;
}


void PrnWriter::stopPrinting()
{
    if (m_isOpen) {
        m_file.close();
        m_isOpen = false;
        m_fileName = "";
    }
}


void PrnWriter::printByte(uint8_t bt)
{
    if (m_isOpen)
        m_file.write8(bt);
}


bool PrnWriter::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "fileName") {
        setPermanentFileName(values[0].asString());
        return true;
    }

    return false;
}


string PrnWriter::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "fileName")
        res = getFileName();

    return res;
}


void PrnWriter::reportError(const std::string& errorStr)
{
    emuLog << errorStr << " " << m_fileName << "\n";
}
