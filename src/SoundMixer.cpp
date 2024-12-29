/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2022
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

// SoundMixer.cpp
// Реализация класса звукового микшера и базовых классов источника звука

#include "Globals.h"
#include "Emulation.h"

#include "Pal.h"
#include "SoundMixer.h"

using namespace std;

// Вызывается 48000 (SAMPLE_RATE) раз в секунду для получения текущего сэмпла и его проигрывания
void SoundMixer::operate()
{
    int leftSample = 0;
    int rightSample = 0;
    for(auto it = m_soundSources.begin(); it != m_soundSources.end(); it++) {
        int left, right;
        (*it)->getSample(left, right);
        leftSample += m_volume < 7 ? left : abs(left);
        rightSample += m_volume < 7 ? right : abs(right);
    }

    leftSample = m_muted ? m_silenceLevel : (leftSample >> m_sampleShift) + m_silenceLevel;
    rightSample = m_muted ? m_silenceLevel : (rightSample >> m_sampleShift) + m_silenceLevel;

    palPlaySample(leftSample, rightSample);

    m_curClock += m_ticksPerSample;

    m_error += m_ticksPerSampleRemainder;
    int delta = m_error / m_sampleRate;
    m_error -= delta * m_sampleRate;
    m_curClock += delta;
}


void SoundMixer::addSoundSource(SoundSource* snd)
{
    m_soundSources.push_back(snd);
}


void SoundMixer::removeSoundSource(SoundSource* snd)
{
    m_soundSources.remove(snd);
}


void SoundMixer::setFrequency(int64_t freq)
{
    m_sampleRate = g_emulation->getSampleRate();
    m_ticksPerSample = freq / m_sampleRate;
    m_ticksPerSampleRemainder = freq % m_sampleRate;
}


void SoundMixer::toggleMute()
{
    m_muted = !m_muted;
}


void SoundMixer::setVolume(int volume)
{
    if (volume < 1 || volume > 7)
        return;

    m_volume = volume;
    m_sampleShift = 7 - volume;

    if (volume <= 6)
        m_silenceLevel = 0;
    else
        m_silenceLevel = -32768;
}


int SoundMixer::getVolume()
{
    return m_volume;
}


SoundSource::SoundSource()
{
    g_emulation->getSoundMixer()->addSoundSource(this);
}


SoundSource::~SoundSource()
{
    g_emulation->getSoundMixer()->removeSoundSource(this);
}


void SoundSource::setNegative(bool negative)
{
    m_negative = negative;
    updateAmpFactor();
}


void SoundSource::setMuted(bool muted)
{
    m_muted = muted;
    updateAmpFactor();
}


void SoundSource::updateAmpFactor()
{
    m_ampFactor = m_muted ? 0 : m_negative ? -1 : 1;
}


void SoundSource::getSample(int& left, int& right)
{
    int val = calcValue();
    left = right = val;
}


bool SoundSource::setProperty(const std::string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "muted") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            setMuted(values[0].asString() == "yes");
            return true;
        }
    } else if (propertyName == "polarity") {
        if (values[0].asString() == "positive" || values[0].asString() == "negative") {
            setNegative(values[0].asString() == "negative");
            return true;
        }
    }
    return false;
}


std::string SoundSource::getPropertyStringValue(const std::string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "muted")
        return m_muted ? "yes" : "no";

    return "";
}


void GeneralSoundSource::setValue(int value)
{
    updateStats();
    m_curValue = value;
}


// Обновляет внутренние счетчики, вызывается перед установкой нового значения либо перед получением текущего
void GeneralSoundSource::updateStats()
{
    uint64_t curClock = g_emulation->getCurClock();
    if (m_curValue) {
        int clocks = curClock - prevClock;
        sumVal += clocks;
    }

    prevClock = curClock;
}

// Получение текущего значения
int GeneralSoundSource::calcValue()
{
    updateStats();

    int res = 0;

    uint64_t ticks = g_emulation->getCurClock() - initClock;
    if (ticks)
            res = sumVal * MAX_SND_AMP / ticks;
    sumVal = 0;
    initClock = g_emulation->getCurClock();

    return res * m_ampFactor;
}
