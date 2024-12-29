/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2020
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


class SoundMixer;

// Базовый класс источника звука
class SoundSource : public EmuObject
{
    public:
        SoundSource();
        virtual ~SoundSource();

        // Получение текущего сэмпла
        virtual int calcValue() = 0;
        virtual void getSample(int& left, int& right); // default implementation for mono sound, uses calcValue, reimplement for stereo

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        void setNegative(bool negative);
        void setMuted(bool muted);

    protected:
        int m_ampFactor = 1;

    private:
        bool m_muted = false;
        bool m_negative = false;

        void updateAmpFactor();
};


// Простой источник звука
class GeneralSoundSource : public SoundSource
{
    public:
        // derived from SoundSOurce
        int calcValue() override;

        // Установка текущего значения источника звука
        void setValue(int value);

        static EmuObject* create(const EmuValuesList&) {return new GeneralSoundSource();}

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

        // возвращает текущий уровень громкости
        int getVolume();

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

        // Уровень громкости (1-5)
        int m_volume = 5;

        // сдвиг отсчета вправо для уменьшения громкости
        int m_sampleShift = 0;

        // уровень тишины (0 для гроскости 0-5)
        int m_silenceLevel = 0;
};

#endif // SOUNDMIXER_H
