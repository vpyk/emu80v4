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

// SoundMixer.h

#ifndef SOUNDMIXER_H
#define SOUNDMIXER_H

#include <list>

#include "EmuObjects.h"


const int MAX_SIGNAL_AMP = 4095;

class SoundMixer;

// Базовый класс источника звука
class SoundSource : public EmuObject
{
    public:
        SoundSource();
        virtual ~SoundSource();

        // Получение текущего сэмпла
        virtual int calcValue() = 0;
};


// Простой источник звука
class GeneralSoundSource : public SoundSource
{
    public:
        // derived from SoundSOurce
        int calcValue() override;

        // Установка текущего значения источника звука
        void setValue(int value);

    private:
        int m_curValue = 0;
        uint64_t initClock = 0;
        uint64_t prevClock = 0;
        int sumVal = 0;

        void updateStats();
};

// Звуковой микшер
class SoundMixer : public ActiveDevice
{
    public:
        // Добавление источника звука
        void addSoundSource(SoundSource* snd);

        // Удаление источника звука
        void removeSoundSource(SoundSource* snd);

        // derived from ActiveDevice
        void operate() override;

        // устанавливет количество тактов на сэмпл (1/SAMPLE_RATE с) на основании тактовой частоты
        void setFrequency(int64_t freq) override;

        // переключает беззвучный режим
        void toggleMute();

        // устанавливает громкость (1-5, 5 - max)
        void setVolume(int volume);

    private:
        // Список источников звука
        std::list<SoundSource*> m_soundSources;

        // частота дисктеризации
        int m_sampleRate = 48000; // some initial value

        // тактов на сэмпл
        int m_ticksPerSample = 300; // some initial value

        // остаток тактов на сэмпл
        int m_ticksPerSampleRemainder = 0;

        // ошибка накопления
        int m_error = 0;

        // признак беззвучного режима
        bool m_muted = false;

        // сдвиг отсчета вправо для уменьшения громкости
        int m_sampleShift = 0;
};

#endif // SOUNDMIXER_H
