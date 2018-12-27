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

// Dma8257.h

#ifndef PSG3910_H
#define PSG3910_H

#include <string>

#include "EmuObjects.h"
#include "SoundMixer.h"

class Psg3910;

/*class Psg3910Counter
{
public:
    void setFreq(unsigned freq);
    void setAmp(unsigned amp);

    friend class Psg3910;
};*/



class Psg3910 : public AddressableDevice
{
    public:
        Psg3910();

        // derived from EmuObject
        void reset() override;

        // derived from AddressableDevice
        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        void updateState();
        uint16_t getOutput();

        static EmuObject* create(const EmuValuesList&) {return new Psg3910();}

    private:
        struct Psg3910Counter {
            unsigned freq;
            unsigned amp;
            bool var;
            bool toneGate;
            bool noiseGate;

            unsigned counter;
            bool toneValue;
            double outValue;
        };

        uint64_t m_prevClock = 0;
        uint64_t m_discreteClock = 0;
        double m_accum = 0.0;
        double m_outValue = 0;
        unsigned m_stepNo = 0;

        Psg3910Counter m_counters[3];
        unsigned m_noiseFreq;
        unsigned m_envFreq;
        unsigned m_envCounter;
        unsigned m_envCounter2;
        bool m_att;
        bool m_alt;
        bool m_hold;

        int m_noise;
        bool m_noiseValue;
        unsigned m_noiseCounter;
        unsigned m_envValue = 0;

        unsigned m_curReg;
        uint8_t m_regs[16];

        void step();
        void envStep();
};


class Psg3910SoundSource : public SoundSource
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        int calcValue() override;

        void attachPsg(Psg3910* psg) {m_psg = psg;}

        static EmuObject* create(const EmuValuesList&) {return new Psg3910SoundSource();}

    protected:
        Psg3910* m_psg = nullptr;
};


#endif // PSG3910_H
