/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017
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
#include "Emulation.h"
#include "Platform.h"
#include "PlatformCore.h"
#include "WavWriter.h"

using namespace std;


WavWriter::WavWriter(Platform* platform, const string& fileName)
{
    m_core = platform->getCore();
    m_ticksPerSample = g_emulation->getFrequency() / 44100;
    m_open = m_file.open(fileName, "w");

    if (m_open)
        for (unsigned i = 0; i < 44; i++)
            m_file.write8(c_wavHeader[i]);
}


WavWriter::~WavWriter()
{
    if (m_open) {
        m_file.seek(4);
        m_file.write32(m_size + 36); // litte endian only!
        m_file.seek(40);
        m_file.write32(m_size);      // litte endian only!
        m_file.close();
    }
}


void WavWriter::operate()
{
    m_curClock += m_ticksPerSample;
    if (m_open) {
        m_file.write8(m_core->getTapeOut() ? 0xE0 : 0x20);
        ++m_size;
    }
}
