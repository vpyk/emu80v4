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

#include "qtAudioDevice.h"


EmuAudioIoDevice::EmuAudioIoDevice(int sampleRate/*, int frameRate*/) : QIODevice(nullptr)
{
    m_buffer = new QByteArray();
    m_pos = 0;
    /*if (frameRate == 0 || frameRate > 60)
        frameRate = 60;*/
    const int frameRate = 60; // !!!
    m_minSamples = sampleRate / (frameRate - 6);
    m_maxSamples = (sampleRate / (frameRate - 2)) * 5;

    memset(m_buf, 0, sizeof(m_buf));
}


void EmuAudioIoDevice::start()
{
    open(QIODevice::ReadOnly);
}


void EmuAudioIoDevice::stop()
{
    m_pos = 0;
    close();
}


qint64 EmuAudioIoDevice::readData(char *data, qint64 maxSize)
{
    if (m_pos < m_minSamples) {
        for (;m_pos < m_minSamples; m_pos++)
            m_buf[m_pos] = m_lastSample;
    } else if (m_pos > m_maxSamples)
        m_pos = m_minSamples * 4;

    if (maxSize >= m_pos * 4) {
        memcpy(data, m_buf, m_pos * 4);
        int read = m_pos * 4;
        m_pos = 0;
        return read;
    } else {
        memcpy(data, m_buf, maxSize);
        memcpy(m_buf, m_buf + maxSize / 4, m_pos * 4 - maxSize);
        m_pos = m_pos - maxSize / 4;
        return maxSize;
    }
}


qint64 EmuAudioIoDevice::bytesAvailable() const
{
    return 16384 * 4;
}


void EmuAudioIoDevice::addSample(int16_t sample)
{
    addSample(sample, sample);
}


void EmuAudioIoDevice::addSample(int16_t leftSample, int16_t rightSample)
{
    uint32_t stereoSample = ((rightSample << 16) & 0xFFFF0000) | (leftSample & 0xFFFF);
    m_lastSample = stereoSample;
    if (m_pos < 16384) {
        m_buf[m_pos++] = stereoSample;
    }
}
