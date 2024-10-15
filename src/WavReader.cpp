/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2024
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

#include <sstream>

#include "Pal.h"
#include "Globals.h"
#include "WavReader.h"
#include "TapeRedirector.h"
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
        g_emulation->setTemporarySpeedUpFactor(0);
        m_isOpen = false;
        return false;
    }

    string fileName = palOpenFileDialog("Open wave file", "Wav and csw files|*.wav;*.WAV;*.csw;*.CSW", false);
    g_emulation->restoreFocus();
    if (fileName == "")
        return true;

    return loadFile(fileName);
}

void WavReader::reportError(const std::string& errorStr)
{
    emuLog << errorStr << " " << m_fileName << "\n";
}



bool WavReader::loadFile(const std::string& fileName, TapeRedirector* tapeRedirector)
{
    m_fileName = fileName;

    if (!m_file.open(fileName)) {
        reportError("Can't open file");
        return false;
    }

    bool res = tryWavFormat();

    if (res) {
        m_tapeRedirector = tapeRedirector;
        m_startClock = g_emulation->getCurClock();
        m_curSample = 0;
        m_isOpen = true;
        m_hasMoreSamples = true;
        readNextSample();

        g_emulation->setTemporarySpeedUpFactor(m_speedUpFactor);
    }

    return res;
}


bool WavReader::tryWavFormat()
{
    unsigned len = m_file.getSize();

    if (len < 8) {
        reportError("Invalid file size:");
        m_file.close();
        return false;
    }

    uint32_t signature = m_file.read32();
    if (signature != 0x46464952) { // "RIFF"
        m_file.seek(0);
        return tryCswFormat();
    }
    len -= 4;

    uint32_t dataSize = m_file.read32();
    len -=4;
    if (len < dataSize) {
        reportError("Invalid WAV file format:");
        m_file.close();
        return false;
    }
    len = dataSize;

    signature = m_file.read32();
    if (signature != 0x45564157) { // "WAVE"
        reportError("Not WAVE file:");
        m_file.close();
        return false;
    }
    len -= 4;

    while (len >= 8) {
        signature = m_file.read32();
        dataSize = m_file.read32();
        len -= 8;
            if (len < dataSize || dataSize < 16) {
                reportError("Invalid WAV file format:");
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
                reportError("Not PCM WAV file:");
                m_file.close();
                return false;
            }
            if (m_channels > 2 || m_bytesPerSample > 2) {
                reportError("Invalid WAV file format, should be mono or stereo and 8 or 16 bit:");
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
            if (len < dataSize) {
                reportError("Invalid WAV file format:");
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

    m_cswFormat = false;

    return true;
}

bool WavReader::tryCswFormat()
{
    unsigned len = m_file.getSize();

    if (len < 32) {
        reportError("Invalid file size:");
        m_file.close();
        return false;
    }

    const char* cswSignature = "Compressed Square Wave\x1a";

    for (int i = 0; i < 23; i++) {
        char c = m_file.read8();
        if (c != cswSignature[i]) {
            reportError("Invalid file format:");
            m_file.close();
            return false;
        }
    }

    uint8_t majorVersion = m_file.read8();
    uint8_t minorVersion = m_file.read8();

    if (majorVersion != 1) {
        reportError("Only CSW-1 supported for now:");
        m_file.close();
        return false;
    }

    m_sampleRate = m_file.read16();

    uint8_t compression = m_file.read8();

    if (compression != 1) {
        reportError("Only RLE CSW compression supported for now:");
        m_file.close();
        return false;
    }

    m_curValue = ! (minorVersion > 0 ? m_file.read8() & 1 : false); // inverted bit 0 - will be inverted at first reading

    m_file.read16(); //reserved
    m_file.read8();  //reserved

    m_rleCounter = 0;
    m_cswFormat = true;

    return true;
}


void WavReader::readNextSample()
{
    if (m_cswFormat)
        readNextCswSample();
    else
        readNextWavSample();
}


void WavReader::readNextWavSample()
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

    if (m_curSample >= m_samples)
        m_hasMoreSamples = false;
}


void WavReader::readNextCswSample()
{
    if (m_rleCounter == 0) {
        m_rleCounter = m_file.read8();
        if (m_rleCounter == 0)
            m_rleCounter = m_file.read32();
        m_curValue = !m_curValue;
    }
    --m_rleCounter;

    if (m_file.eof() && m_rleCounter == 0)
        m_hasMoreSamples = false;
}


bool WavReader::getCurValue()
{
    if (!m_isOpen)
        return false;

    uint64_t curClock = g_emulation->getCurClock();
    int sampleNo = (curClock - m_startClock) * m_sampleRate / g_emulation->getFrequency();

    if (sampleNo == m_curSample)
        return m_curValue;

    while (m_hasMoreSamples && m_curSample < sampleNo) {
        readNextSample();
        ++m_curSample;
    }
    if (m_hasMoreSamples)
        return m_curValue;
    else {
        m_isOpen = false;
        m_file.close();
        g_emulation->setTemporarySpeedUpFactor(0);
        if (m_tapeRedirector)
            m_tapeRedirector->closeFile();
        return false;
    }
}


string WavReader::posToTime(unsigned sampleNo)
{
    int sec = sampleNo / m_sampleRate;
    int min = sec / 60;
    sec = sec % 60;

    stringstream ss;
    ss << min << ":";
    ss.width(2);
    ss.fill('0');
    ss << sec;
    return ss.str();
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
    } else if (propertyName == "speedUpFactor") {
        m_speedUpFactor = values[0].asInt();
        if (m_speedUpFactor == 0)
            m_speedUpFactor = 1;
        if (m_isOpen)
            g_emulation->setTemporarySpeedUpFactor(m_speedUpFactor);
        return true;
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
    else if (propertyName == "speedUpFactor") {
        stringstream stringStream;
        stringStream << m_speedUpFactor;
        stringStream >> res;
    } else if (propertyName == "currentFile" && m_isOpen) {
        res = m_fileName;
    } else if (propertyName == "position" && m_isOpen) {
        res = posToTime(m_curSample);
        if (!m_cswFormat)
            res += "/" + posToTime(m_samples);
    }

    return res;
}


WavSoundSource::WavSoundSource(WavReader* wavReader) : SoundSource()
{
    m_wavReader = wavReader;
}


int WavSoundSource::calcValue()
{
    return m_wavReader->getCurValue() ? MAX_SND_AMP / 2 * m_ampFactor : 0;
}
