/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2021
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

#ifndef DMA8257_H
#define DMA8257_H

#include "EmuObjects.h"

class Cpu;


class Dma8257 : public AddressableDevice
{
    enum DmaCycleType
    {
        DCT_VERIFY  = 0b00,
        DCT_READ    = 0b01,
        DCT_WRITE   = 0b10,
        DCT_ILLEGAL = 0b11
    };

    public:
        Dma8257();
        virtual ~Dma8257() {}

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        void reset() override;
        std::string getDebugInfo() override;

        void attachCpu(Cpu* cpu);
        void attachAddrSpace(AddressableDevice* as);
        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        uint16_t getChAddr(int channel);
        uint16_t getChLen(int channel);
        bool isReadAvailable(int channel);
        bool isWriteAvailable(int channel);
        bool isVerifyAvailable(int channel);
        bool dmaRequest(int channel, uint8_t &value, uint64_t clock = 0);

        uint8_t getMR();
        static EmuObject* create(const EmuValuesList&) {return new Dma8257();}

/*
        void getBlock(int channel, int len, uint8_t* buf);
*/
    private:
        AddressableDevice* m_addrSpace = nullptr;
        Cpu* m_cpu = nullptr;
        uint16_t m_addr[4];
        uint16_t m_count[4];
        uint8_t m_modeReg;
        uint8_t m_statusReg;
        bool m_isLoByte;
        bool m_swapRw = true; // swap MEMW & MEMR, default for RK86 etc.
        //int m_kDiv = 1;

        uint16_t m_initAddr[4] = {0, 0, 0, 0};
        uint16_t m_initCount[4] = {0, 0, 0, 0};
};

#endif // DMA8257_H
