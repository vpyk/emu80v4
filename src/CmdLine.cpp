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

#include "CmdLine.h"

using namespace std;

CmdLine::CmdLine(int argc, char** argv)
{
    const char* loadFile = nullptr;
    bool loadOnly = false;

    for (int i = 1; i < argc; i++) {
        char* arg = argv[i];

        if (arg[0] == '-') {
            if (arg[1] == '-') {
                // -- option
                arg += 2;
                auto it = c_allowedParams.find(arg);
                if (it != c_allowedParams.end()) {
                    bool hasParam = it->second;
                    char* par = argv[i + 1];
                    if (hasParam && (!par || par[0] == '-'))
                        m_warnings += (std::string("No parameter for option --") + arg + "\n");
                    else {
                        if (m_parameters.find(arg) == m_parameters.end())
                            m_parameters.insert(make_pair(arg, hasParam ? par : ""));
                        else
                            m_warnings += (std::string("Duplicate option --") + arg + "\n");
                        if (hasParam && par)
                            i++;
                    }
                } else {
                    m_warnings += (std::string("Unknown option --") + arg + "\n");
                }
            } else {
                // - option
                arg++;
                if (arg[0] == 'l' && arg[1] == 0)
                    loadOnly = true;
                else {
                    if (c_ignoredParams.find(arg) == c_ignoredParams.end())
                        m_oldParameters.insert(arg);
                }
            }
        } else {
            // no '-'
            loadFile = arg;
        }
    }

    if (loadFile && loadFile[0]) {
        std::string opt = loadOnly ? "load" : "run";
        if (m_parameters.find(opt) != m_parameters.end())
            m_warnings += (string("Ignoring -") + opt + "\n");
        else
            m_parameters[opt] = loadFile;
    }
}


void CmdLine::processPlatforms(const std::vector<PlatformInfo>& platforms)
{
    //m_warnings.clear();

    if (m_parameters.find("platform") != m_parameters.end())
        return;

    for (const auto& pi: platforms) {
        const char* opt = pi.cmdLineOption.c_str();
        if (m_oldParameters.find(opt) != m_oldParameters.end()) {
            m_parameters["platform"] = pi.objName.c_str(); // platforms should exist when using cmdLine
        }
    }
}


const std::string CmdLine::operator[](const std::string& key) const
{
    auto it = m_parameters.find(key);
    if (it != m_parameters.end())
        return it->second;
    else
        return "";
}


bool CmdLine::checkParam(const std::string& key) const
{
    return m_parameters.find(key) != m_parameters.end();
}
