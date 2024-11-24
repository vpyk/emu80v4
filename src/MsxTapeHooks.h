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

#ifndef MSXTAPEHOOKS_H
#define MSXTAPEHOOKS_H

#include "CpuHook.h"

class MsxTapeOutHook : public CpuHook
{
    public:
        MsxTapeOutHook(uint16_t addr) : CpuHook(addr) {}
        virtual ~MsxTapeOutHook() {}
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        bool hookProc() override;

        static EmuObject* create(const EmuValuesList& parameters) {return parameters[0].isInt() ? new MsxTapeOutHook(parameters[0].asInt()) : nullptr;}

    private:
        bool m_regC = false;
        uint16_t m_leaveAddr = 0;

        int m_curPos = 0;
};


class MsxTapeOutHeaderHook : public CpuHook
{
    public:
        MsxTapeOutHeaderHook(uint16_t addr) : CpuHook(addr) {}
        virtual ~MsxTapeOutHeaderHook() {}

        bool hookProc() override;

        static EmuObject* create(const EmuValuesList& parameters) {return parameters[0].isInt() ? new MsxTapeOutHeaderHook(parameters[0].asInt()) : nullptr;}
};


class MsxTapeInHook : public CpuHook
{
    public:
        MsxTapeInHook(uint16_t addr) : CpuHook(addr) {}
        virtual ~MsxTapeInHook() {}
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        bool hookProc() override;

        static EmuObject* create(const EmuValuesList& parameters) {return parameters[0].isInt() ? new MsxTapeInHook(parameters[0].asInt()) : nullptr;}

    private:
        bool m_ignoreHeaders = false;
        bool m_apogeyFix = false;
        bool m_lvovFix = false;

        int m_typeRptCount = 0;
        uint8_t m_type;
};


class MsxTapeInHeaderHook : public CpuHook
{
    public:
        MsxTapeInHeaderHook(uint16_t addr) : CpuHook(addr) {}
        virtual ~MsxTapeInHeaderHook() {}

        bool hookProc() override;

        static EmuObject* create(const EmuValuesList& parameters) {return parameters[0].isInt() ? new MsxTapeInHeaderHook(parameters[0].asInt()) : nullptr;}
};


#endif //MSXTAPEHOOKS_H


