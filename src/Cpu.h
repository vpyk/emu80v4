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

#ifndef CPU_H
#define CPU_H

#include <vector>

#include "EmuObjects.h"


class CpuHook;
class PlatformCore;


class Cpu : public ActiveDevice
{
    public:
        enum CpuType {
            CPU_8080,
            CPU_Z80
        };

        Cpu();
        virtual ~Cpu();

        virtual CpuType getType() = 0;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void attachAddrSpace(AddressableDevice* as);
        void attachIoAddrSpace(AddressableDevice* as);
        void attachCore(PlatformCore* core);
        void setStartAddr(unsigned addr) {m_startAddr = addr;};

        //virtual int doInstruction() = 0;
        virtual void interrupt(int) {};
        virtual void hrq(int) {};

        virtual void addHook(CpuHook* hook);
        virtual void removeHook(CpuHook* hook);

        void debugStepRequest() {m_stepReq = true;};

        AddressableDevice* getAddrSpace() {return m_addrSpace;};

    protected:
        AddressableDevice* m_addrSpace = nullptr;
        AddressableDevice* m_ioAddrSpace = nullptr;
        PlatformCore* m_core = nullptr;
        unsigned m_startAddr = 0;

        std::vector<CpuHook*> m_hookVector;
        int m_nHooks = 0;

        bool m_stepReq = false;

        bool m_debugOnHalt = false;
        bool m_debugOnIllegalCmd = false;
};


class Cpu8080Compatible : public Cpu
{
    public:
        void addHook(CpuHook* hook) override;
        void removeHook(CpuHook* hook) override;

        virtual void intRst(int vect) = 0;
        virtual void ret() = 0;

        void hrq(int ticks) override;

        virtual uint16_t getPC() = 0;
        virtual uint16_t getBC() = 0;
        virtual uint16_t getDE() = 0;
        virtual uint16_t getHL() = 0;
        virtual uint16_t getSP() = 0;
        virtual uint16_t getAF() = 0;

        /*virtual int getA() = 0;
        virtual int getB() = 0;
        virtual int getC() = 0;
        virtual int getD() = 0;
        virtual int getE() = 0;
        virtual int getH() = 0;
        virtual int getL() = 0;*/

        virtual void setBC(uint16_t value) = 0;
        virtual void setDE(uint16_t value) = 0;
        virtual void setHL(uint16_t value) = 0;
        virtual void setSP(uint16_t value) = 0;
        virtual void setPC(uint16_t value) = 0;
        virtual void setAF(uint16_t value) = 0;
        //virtual void exec(int nCmds) = 0;

        virtual bool getInte() = 0;

    protected:
        std::vector<uint16_t>m_hookAddrVector;
        uint16_t* m_hookAddresses = nullptr;

        int io_input(int port);
        void io_output(int port, int value);
};

#endif // CPU_H
