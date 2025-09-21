/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2019
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

#ifndef WAVREADER_H
#define WAVREADER_H

#include "PalFile.h"

#include "EmuObjects.h"
#include "SoundMixer.h"


class WavSoundSource;
class TapeRedirector;

class WavReader : public EmuObject
{
    public:

        enum WavChannel {
            WC_LEFT,
            WC_RIGHT,
            WC_MIX
        };

        WavReader();
        virtual ~WavReader();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        bool loadFile(const std::string& fileName, TapeRedirector* tapeRedirecotr = nullptr);
        bool chooseAndLoadFile();
        bool isPlaying() {return m_isOpen;}
        void unlinkTapeRedirector(TapeRedirector* tapeRedirecotr);

        bool getCurValue();

    private:
        PalFile m_file;

        std::string m_fileName;

        bool m_cswFormat = false;

        int m_channels;
        int m_sampleRate;
        int m_bytesPerSample;
        int m_samples;

        bool m_isOpen = false;
        uint64_t m_startClock;
        int m_curSample;
        bool m_curValue;
        int m_rleCounter = 0;
        bool m_hasMoreSamples;

        WavChannel m_channel = WC_LEFT;
        unsigned m_speedUpFactor = 1;

        WavSoundSource* m_wavSource;

        TapeRedirector* m_tapeRedirector = nullptr;

        void reportError(const std::string& errorStr);
        bool tryWavFormat();
        bool tryCswFormat();
        void readNextSample();
        void readNextWavSample();
        void readNextCswSample();

        std::string posToTime(unsigned sampleNo);
};


class WavSoundSource : public SoundSource
{
    public:
        WavSoundSource(WavReader* wavReader);

        // derived from SoundSOurce
        int calcValue() override;

    private:
        WavReader* m_wavReader;
};


#endif // WAVREADER_H
