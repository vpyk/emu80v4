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
#include "WavReader.h"
#include "Emulation.h"

using namespace std;

WavReader::WavReader()
{
    m_wavSource = new WavSoundSource(this);
}


WavReader::~WavReader()
{
    delete m_wavSource;
}


bool WavReader::chooseAndLoadFile()
{
    if (m_isOpen) {
        m_file.close();
        m_isOpen = false;
        return false;
    }

    string fileName = palOpenFileDialog("Open WAV file", "*.wav", false);
    g_emulation->restoreFocus();
    if (fileName == "")
        return true;
    if (!loadFile(fileName)) {
        //emuLog << "Error loading file: " << fileName << "\n";
        return false;
    }
    return true;
}

bool WavReader::loadFile(const std::string& fileName)
{
    if (!m_file.open(fileName)) {
        emuLog << "Can't open file " << fileName << "\n";
        return false;
    }

    unsigned len = m_file.getSize();

    if (len < 8) {
        emuLog << "Invalid WAV file size" << fileName << "\n";
        m_file.close();
        return false;
    }

    uint32_t signature = m_file.read32();
    if (signature != 0x46464952) { // "RIFF"
        emuLog << "Not RIFF file: " << fileName << "\n";
        m_file.close();
        return false;
    }
    len -= 4;

    uint32_t dataSize = m_file.read32();
    len -=4;
    if (len < dataSize) {
        emuLog << "Invalid WAV file format: " << fileName << "\n";
        m_file.close();
        return false;
    }
    len = dataSize;

    signature = m_file.read32();
    if (signature != 0x45564157) { // "WAVE"
        emuLog << "Not WAVE file: " << fileName << "\n";
        m_file.close();
        return false;
    }
    len -= 4;

    while (len >= 8) {
        signature = m_file.read32();
        dataSize = m_file.read32();
        len -= 8;
            if (len < dataSize || dataSize < 16) {
                emuLog << "Invalid WAV file format: " << fileName << "\n";
                m_file.close();
                return false;
            }
        if (signature == 0x20746D66) { // "fmt "
            uint16_t compression = m_file.read16();
            m_channels = m_file.read16();
            m_sampleRate = m_file.read32();
            m_file.skip(6);
            m_bytesPerSample = m_file.read16() / 8;
            len -= 16;
            m_file.skip(dataSize - 16);
            len -= (dataSize - 16);

            if (compression != 1) {
                emuLog << "Not PCM WAV file: " << fileName << "\n";
                m_file.close();
                return false;
            }
            if (m_channels > 2 || m_bytesPerSample > 2) {
                emuLog << "Invalid WAV file format, should be mono or stereo and 8 or 16 bit: " << fileName << "\n";
                m_file.close();
                return false;
            }
            break;
        } else {
            m_file.skip(dataSize);
            len -= dataSize;
        }
    }

    while (len >= 8) {
        signature = m_file.read32();
        dataSize = m_file.read32();
        len -= 8;
            if (len < dataSize || dataSize < 8) {
                emuLog << "Invalid WAV file format: " << fileName << "\n";
                m_file.close();
                return false;
            }
        if (signature == 0x61746164) // "data"
            break;
        else {
            m_file.skip(dataSize);
            len -= dataSize;
        }
    }

    m_samples = dataSize / (m_bytesPerSample * m_channels);

    m_startClock = g_emulation->getCurClock();
    m_curSample = 0;
    m_isOpen = true;

    return true;

}


void WavReader::readNextSample()
{
    int val;
    if (m_bytesPerSample == 1) {
        val = (int)m_file.read8() - 128;
        if (m_channels == 2)
            switch (m_channel) {
                case WC_LEFT:
                    m_file.skip(1);
                    break;
                case WC_RIGHT:
                    val = (int)m_file.read8() - 128;
                    break;
                case WC_MIX:
                    val += (int)m_file.read8() - 128;
                    break;
                default:
                    break;
        }
    } else { // if (m_bytesPerSample == 2) {
        val = (int16_t)m_file.read16();
        if (m_channels == 2)
            switch (m_channel) {
                case WC_LEFT:
                    m_file.skip(2);
                    break;
                case WC_RIGHT:
                    val = (int16_t)m_file.read16();
                    break;
                case WC_MIX:
                    val += (int16_t)m_file.read16();
                    break;
                default:
                    break;
            }
    }
    if (m_bytesPerSample == 2)
        val /= 256;
    if (m_channels == 2 && m_channel == WC_MIX)
        val /=2;
    m_curValue = val > (m_curValue ? -2 : 2);
    //m_curValue = val > 0;
}


bool WavReader::getCurValue()
{
    if (!m_isOpen)
        return false;

    uint64_t curClock = g_emulation->getCurClock();
    int sampleNo = (curClock - m_startClock) * m_sampleRate / g_emulation->getFrequency();
    if (sampleNo == m_curSample)
        return m_curValue;
    while (m_curSample < sampleNo && sampleNo < m_samples) {
        readNextSample();
        ++m_curSample;
    }
    if (sampleNo < m_samples)
        return m_curValue;
    else {
        m_isOpen = false;
        m_file.close();
        return false;
    }
}


bool WavReader::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "channel") {
        if (values[0].asString() == "left") {
            m_channel = WC_LEFT;
            return true;
        } else if (values[0].asString() == "right") {
            m_channel = WC_RIGHT;
            return true;
        } else if (values[0].asString() == "mix") {
            m_channel = WC_MIX;
            return true;
        } else
            return false;
    }
    return false;
}


string WavReader::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "channel")
        switch (m_channel) {
            case WC_LEFT:
                res = "left";
                break;
            case WC_RIGHT:
                res = "right";
                break;
            case WC_MIX:
                res = "mix";
                break;
            default:
                break;
        }
    return res;
}


WavSoundSource::WavSoundSource(WavReader* wavReader) : SoundSource()
{
    m_wavReader = wavReader;
}


int WavSoundSource::calcValue()
{
    return m_wavReader->getCurValue() ? 2048 : 0;
}
