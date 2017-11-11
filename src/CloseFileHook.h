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

#ifndef CLOSEFILEHOOK_H
#define CLOSEFILEHOOK_H

#include <vector>

#include "CpuHook.h"

class TapeRedirector;


class CloseFileHook : public CpuHook
{
    public:
        CloseFileHook(uint16_t addr) : CpuHook(addr) {};
        void addTapeRedirector(TapeRedirector* fr);
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        bool hookProc() override;

    private:
        std::vector<TapeRedirector*> m_frVector;
        TapeRedirector** m_frs;
        int m_nFr = 0;
};

#endif //CLOSEFILEHOOK_H
