/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2018
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

// Fdc1793.h

// Реализация контроллера НГМД КР580ВГ93 (FDC1793), заголовочный файл

#ifndef FDC1793_H
#define FDC1793_H

#include "EmuObjects.h"

class Dma8257;
class FdImage;


const int MAX_DRIVES = 4;

class Fdc1793 : public AddressableDevice
{
    enum FdcAccessMode {
        FAM_WAITING,
        FAM_READING,
        FAM_WRITING
    };

    public:
        Fdc1793();
        virtual ~Fdc1793();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getDebugInfo() override;
        void reset() override; // Chip reset

        // derived from AddressableDevice
        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;


        void setDrive(int drive);
        void setHead(int head);

        bool getIrq() {return m_irq;}
        bool getDrq();


        // Подключение DMA-контроллера
        void attachDMA(Dma8257* dma, int channel);

        // Подключение рбраза диска
        void attachFdImage(int driveNum, FdImage* image);

        static EmuObject* create(const EmuValuesList&) {return new Fdc1793();}

    private:
        FdImage* m_images[MAX_DRIVES];   // Disk images

        Dma8257* m_dma = nullptr;        // Linked DMA Controller
        int m_dmaChannel;                // DMA channel

        FdcAccessMode m_accessMode;
        int m_disk;         // номер дисковода
        int m_head;         // номер головки
        uint8_t m_track;    // регистр дорожки
        uint8_t m_sector;   // регистр сектора
        uint8_t m_data;     // регистр данных
        uint8_t m_status;   // регистр статуса
        bool m_directionIn; // направление движения true=in, false=out

        uint64_t m_cmdTime; // cmd timestamp

        bool m_irq = false;
        int m_lastCommand = 0;
        //bool m_busy = false;

        int m_addressIdCnt = 0;
        uint8_t m_addressId[6];

        void generateInt();
};


#endif // FDC1793_H
