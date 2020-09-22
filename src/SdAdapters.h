/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2020
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

#ifndef SDADAPTERS_H
#define SDADAPTERS_H


#include "EmuObjects.h"
#include "Ppi8255Circuit.h"

class SdCard;


enum SdAdapterType {
    SDA_HWMPVV,
    SDA_MSX,
    SDA_N8VEM
};


class SdAdapter : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        void reset() override;

        // Derived from AddressableDevice
        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        void attachSdCard(SdCard* card) {m_sdCard = card;}

        static EmuObject* create(const EmuValuesList&) {return new SdAdapter();}

    private:
        SdAdapterType m_type = SDA_HWMPVV;

        SdCard* m_sdCard = nullptr;
        uint8_t m_readValue = 0xFF;
        uint8_t m_valueToWrite = 0;
        int m_bitCnt = 0;
        bool m_prevClk = 0;

        void writeConfPort(uint8_t value);
        void writeDataPort(uint8_t value);
        uint8_t readDataPort();
};


class PpiSdAdapter : public Ppi8255Circuit
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        void reset() override;

        // Derived from Ppi8255Circuit
        void setPortB(uint8_t value) override; // write data
        void setPortC(uint8_t value) override; // write conf
        uint8_t getPortA() override; // read data

        void attachSdCard(SdCard* card);

        static EmuObject* create(const EmuValuesList&) {return new PpiSdAdapter();}

    private:
        SdAdapterType m_type = SDA_HWMPVV;

        SdCard* m_sdCard = nullptr;
        uint8_t m_readValue = 0xFF;
        uint8_t m_valueToWrite = 0;
        bool m_prevWr = 0;
        int m_bitCnt = 0;
        bool m_prevClk = 0;
};

#endif // SDADAPTERS_H
