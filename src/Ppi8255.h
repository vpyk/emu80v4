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

// Crt8255.h

// Реализация контроллера программируемого перефирийного интерфейса КР580ВВ55 (i8255), заголовочный файл

#ifndef PPI8255_H
#define PPI8255_H

#include "EmuObjects.h"

class Ppi8255Circuit;


// Контроллер i8255
class Ppi8255 : public AddressableDevice
{
    enum PpiChMode
    {
        PCM_OUT = 0,
        PCM_IN = 1//,
        // PCM_INOUT = 2 // двунаправленный режим - пока не реализован
    };

    public:
        Ppi8255();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        void reset() override; // Chip reset

        // derived from AddressableDevice
        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        // Подключение объекта - обвязки ВВ55
        void attachPpi8255Circuit(Ppi8255Circuit* circuit);

        static EmuObject* create(const EmuValuesList&) {return new Ppi8255();}

        /*
        void setPortA(uint8_t bValue) {m_portA = bValue;}
        void setPortB(uint8_t bValue) {m_portB = bValue;}
        void setPortC(uint8_t bValue) {m_portC = bValue;}
        uint8_t getPortA() {return m_portA;}
        uint8_t getPortB() {return m_portB;}
        uint8_t getPortC() {return m_portC;}
        PpiChMode getChAMode() {return m_chAMode;}
        PpiChMode getChBMode() {return m_chBMode;}
        PpiChMode getChCHiMode() {return m_chCHiMode;}
        PpiChMode getChCLowMode() {return m_chCLowMode;}
        */
    private:
        // обвязка ВВ55
        Ppi8255Circuit* m_ppiCircuit = nullptr;

        // Текущие значения портов
        uint8_t m_portA;
        uint8_t m_portB;
        uint8_t m_portC;

        // Текущие режимы портов
        PpiChMode m_chAMode;
        PpiChMode m_chBMode;
        PpiChMode m_chCHiMode;
        PpiChMode m_chCLoMode;
};

#endif // PPI8255_H
