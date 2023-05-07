/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2023
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

#ifndef CMDLINE_H
#define CMDLINE_H


#include <string>
#include <map>
//#include <list>
#include <vector>
#include <set>

#include "EmuTypes.h"


class CmdLine
{
public:
    CmdLine(int argc, char** argv);

    const std::string operator[](const std::string& key) const;
    bool checkParam(const std::string& key) const;
    const std::string& getWarnings() {return m_warnings;}
    void processPlatforms(const std::vector<PlatformInfo>& platforms);

private:
    const std::map<std::string, bool> c_allowedParams = {
        {"help", false},
        {"platform", true},
        {"conf-file", true},
        {"post-conf", true},
//        {"pre-cfg", true},
        {"load", true},
        {"run", true},
        {"disk-a", true},
        {"disk-b", true},
        {"disk-c", true},
        {"disk-d", true},
        {"hdd", true},
        {"edd", true},
        {"edd2", true}
//        {"load-only", false}
    };

    const std::set<const char*> c_ignoredParams = {"opengl", "angle", "dx9", "dx11", "warp", "soft"};

    std::map<std::string, std::string> m_parameters;
    std::set<std::string> m_oldParameters;

    std::string m_warnings;
};

#endif // CMDLINE_H
