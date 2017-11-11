/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2017
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

#ifndef RKTAPEHOOKS_H
#define RKTAPEHOOKS_H

#include "CpuHook.h"

class RkTapeOutHook : public CpuHook
{
    public:
        RkTapeOutHook(uint16_t addr) : CpuHook(addr) {};
        virtual ~RkTapeOutHook() {};
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        bool hookProc() override;

    private:
        bool m_isSbFound = false;
        bool m_regA = false;
};


class RkTapeInHook : public CpuHook
{
    public:
        RkTapeInHook(uint16_t addr) : CpuHook(addr) {};
        virtual ~RkTapeInHook() {};

        //bool setProperty(const string& propertyName, const EmuValuesList& values) override;

        bool hookProc() override;

    private:
};


#endif //RKTAPEHOOKS_H


