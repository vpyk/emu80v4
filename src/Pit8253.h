/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2024
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

// Pit8253.h

#ifndef PIT8253_H
#define PIT8253_H

//#include <string>

#include "EmuObjects.h"

// Uncomment for Raspberry Pi etc.
// Thanks to Viacheslav Slavinsky aka svofski
//#define LESS_64BIT_DIVS

class Pit8253;
class Pit8253Helper;
class PlatformCore;

class Pit8253Counter : public EmuObject //PassiveDevice
{
    public:
        Pit8253Counter(Pit8253* pit, int number);
        //~Pit8253Counter();

        void setGate(bool gate);
        bool getOut();

        int getAvgOut();
        int getSumOutTicks() {return m_sumOutTicks;}
        void getStats(uint64_t& clocksTotal, uint64_t& clocksHi);
        void resetStats();

        void updateState();
        void operateForTicks(int ticks);

        void setExtClockMode(bool extClockMode) {m_extClockMode = extClockMode;}
        inline bool getExtClockMode() {return m_extClockMode;}

        friend class Pit8253;
        friend class Pit8253Helper;

    private:
        int m_number; // номер счетчика
        Pit8253* m_pit;
        Pit8253Helper* m_helper = nullptr;
        bool m_extClockMode = false;

        PlatformCore* m_core = nullptr;

        uint64_t m_prevClock = 0;
        uint64_t m_sampleClock = 0;
        int m_avgOut = 0;
        int m_sumOutTicks = 0;
        int m_tempSumOut = 0;
        int m_tempAddOutClocks = 0;

#ifdef LESS_64BIT_DIVS
        uint32_t m_prevFastClock = 0;
#endif

        int m_mode;
        bool m_gate;
        bool m_out;

        bool m_prevOut;

        int m_counter;
        int m_counterInitValue;
        bool m_isCounting;
        int m_countDelay = 0;

        void setMode(int mode);
        void setHalfOfCounter();
        void setCounter(uint16_t counter);

        void startCount();
        void stopCount();

        void planIrq();
        void outChangeNotify();
};

class Pit8253 : public AddressableDevice
{

    // pitLoadMode values
    enum PitReadLoadMode
    {
        PRLM_LATCH = 0,
        PRLM_LOWBYTE = 1,
        PRLM_HIGHBYTE = 2,
        PRLM_WORD = 3
    };

    public:
        Pit8253();
        virtual ~Pit8253();

        void setFrequency(int64_t freq) override;
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        // std::string getDebugInfo() override;

        void mute(); // i8253 does not have reset input so use this method instead if necessary

        // derived from AddressableDevice
        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        void updateState();
        void setGate(int counter, bool gate);
        bool getOut(int counter);

        Pit8253Counter* getCounter(int counterNum) {return m_counters[counterNum];}

        static EmuObject* create(const EmuValuesList&) {return new Pit8253();}

    private:
        Pit8253Counter* m_counters[3];
        uint16_t m_latches[3];
        bool m_latched[3];
        PitReadLoadMode m_rlModes[3];
        bool m_waitingHi[3];
};


class Pit8253Helper : public ActiveDevice
{
public:
    Pit8253Helper();
    void operate() override;
    //bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

    void setCounter(Pit8253Counter* cnt) {m_counter = cnt;}

    void updateAndScheduleNext(uint64_t time);

    static EmuObject* create(const EmuValuesList&) {return new Pit8253Helper();}

private:
    Pit8253Counter* m_counter = nullptr;

    void checkForInt();
};

#endif // PIT8253_H
