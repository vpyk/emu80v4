/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-20122
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

#include "Pal.h"
#include "Globals.h"
#include "Emulation.h"
#include "Platform.h"
#include "PlatformCore.h"
#include "WavWriter.h"

using namespace std;


WavWriter::WavWriter(Platform* platform, const string& fileName, bool cswFormat)
{
    setName("wavWriter");
    m_core = platform->getCore();
    m_ticksPerSample = g_emulation->getFrequency() / 44100;
    m_open = m_file.open(fileName, "w");
    m_cswFormat = cswFormat;
    m_initialValue = m_core->getTapeOut();

    if (!m_open)
        return;

    m_fileName = fileName;

    if (m_cswFormat) {
        m_cswCurValue = m_initialValue;
        m_cswRleCounter = 1;
        for (unsigned i = 0; i < 32; i++)
            m_file.write8(c_cswHeader[i]);
    } else
        for (unsigned i = 0; i < 44; i++)
            m_file.write8(c_wavHeader[i]);

    //m_file.write8(1); // 1 sample of initial value
}


WavWriter::~WavWriter()
{
    if (!m_open)
        return;

    if (!m_cswFormat)
    {
        // WAV
        m_file.seek(4);
        m_file.write32(m_size + 36); // litte endian only!
        m_file.seek(40);
        m_file.write32(m_size);      // litte endian only!
    } else {
        writeCswSequence();
        m_file.seek(0x1C);
        m_file.write8(m_initialValue ? 0 : 1);
    }
    m_file.close();
}


void WavWriter::writeCswSequence()
{
    if (m_cswRleCounter <= 255)
        m_file.write8(m_cswRleCounter);
    else {
        m_file.write8(0);
        m_file.write32(m_cswRleCounter);
    }
}


void WavWriter::operate()
{
    m_curClock += m_ticksPerSample;
    if (!m_open)
        return;

    bool value = m_core->getTapeOut();

    if (m_size == 0 && value == m_initialValue) // skip silence at the beginning
        return;

    ++m_size;

    if (m_cswFormat) {
        if (value == m_cswCurValue)
            ++m_cswRleCounter;
        else {
            m_cswCurValue = value;
            writeCswSequence();
            m_cswRleCounter = 1;
        }

    } else
        m_file.write8(value ? 0xE0 : 0x20);
}


string WavWriter::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "currentFile" && m_open) {
        return m_fileName;
    }

    return "";
}
