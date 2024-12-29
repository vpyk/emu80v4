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

#ifndef QTAUDIODEVICE_H
#define QTAUDIODEVICE_H

#include <QIODevice>

class EmuAudioIoDevice : public QIODevice
{
    Q_OBJECT

    public:
        EmuAudioIoDevice(int sampleRate/*, int frameRate*/);
        //~EmuAudioIoDevice();

        void addSample(int16_t sample); // mono
        void addSample(int16_t leftSample, int16_t rightSample); //stereo

        void start();
        void stop();

    protected:
        qint64 readData(char* data, qint64 maxSize) override;
        qint64 writeData(const char*, qint64) override {return 0;}
        qint64 bytesAvailable() const override;

    private:
        qint64 m_pos;
        QByteArray* m_buffer;
        uint32_t m_lastSample = 0;

        int m_minSamples;
        int m_maxSamples;

        uint32_t m_buf[16384];
};


#endif // QTAUDIODEVICE_H
