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

#ifndef CPUHOOK_H
#define CPUHOOK_H

#include <vector>

#include "Cpu.h"


class Cpu;
class TapeRedirector;


// Базовый класс ловушки процессора
class CpuHook : public EmuObject
{
    public:
        CpuHook(int addr);
        //CpuHook(int addr, uint8_t memCheck);
        virtual ~CpuHook();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        virtual void setCpu(Cpu* cpu) {m_cpu = cpu;}
        virtual bool hookProc() = 0; // returns false if continue

        void setEnabled(bool isEnabled);
        inline bool getEnabled() {return m_isEnabled;}

        inline int getHookAddr() {return m_hookAddr;}

        void setTapeRedirector(TapeRedirector* file) {m_file = file;}
        void setSignature(std::string signature);

    protected:
        Cpu* m_cpu = nullptr;
        bool m_isEnabled = true;
        TapeRedirector* m_file = nullptr;
        bool m_hasSignature = false;
        bool checkSignature();

    private:
        int m_hookAddr;
        std::vector<uint8_t> m_signature;
        unsigned m_signatureLen = 0;
        uint8_t* m_signatureBytes = nullptr;
};


class Ret8080Hook : public CpuHook
{
    public:
        Ret8080Hook(uint16_t addr) : CpuHook(addr) {}

        bool hookProc() override;

        static EmuObject* create(const EmuValuesList& parameters) {return parameters[0].isInt() ? new Ret8080Hook(parameters[0].asInt()) : nullptr;}
};


#endif //CPUHOOK_H

