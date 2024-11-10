/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2021-2024
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

#ifndef RFSTAPEHOOKS_H
#define RFSTAPEHOOKS_H

#include "CpuHook.h"

class RfsTapeOutHeaderHook : public CpuHook
{
    public:
        RfsTapeOutHeaderHook(uint16_t addr) : CpuHook(addr) {}
        //virtual ~RfsTapeOutHeaderHook() {}

        bool hookProc() override;

        static EmuObject* create(const EmuValuesList& parameters) {return parameters[0].isInt() ? new RfsTapeOutHeaderHook(parameters[0].asInt()) : nullptr;}
};


class RfsTapeOutHook : public CpuHook
{
    public:
        RfsTapeOutHook(uint16_t addr) : CpuHook(addr) {}
        //virtual ~RfsTapeOutHook() {}
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        bool hookProc() override;

        static EmuObject* create(const EmuValuesList& parameters) {return parameters[0].isInt() ? new RfsTapeOutHook(parameters[0].asInt()) : nullptr;}

    private:
        uint16_t m_leaveAddr = 0;
};


class RfsTapeInHeaderHook : public CpuHook
{
    public:
        RfsTapeInHeaderHook(uint16_t addr) : CpuHook(addr) {}
        //virtual ~RfsTapeInHeaderHook() {}

        bool hookProc() override;

        static EmuObject* create(const EmuValuesList& parameters) {return parameters[0].isInt() ? new RfsTapeInHeaderHook(parameters[0].asInt()) : nullptr;}
};


class RfsTapeInHook : public CpuHook
{
    public:
        RfsTapeInHook(uint16_t addr) : CpuHook(addr) {}
        //virtual ~RfsTapeInHook() {}

        bool hookProc() override;

        static EmuObject* create(const EmuValuesList& parameters) {return parameters[0].isInt() ? new RfsTapeInHook(parameters[0].asInt()) : nullptr;}
};


#endif //RFSTAPEHOOKS_H
