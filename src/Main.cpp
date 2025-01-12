/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2025
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

#include <string>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <stringapiset.h>
#endif

#include "Pal.h"

#include "CmdLine.h"
#include "Emulation.h"

using namespace std;

Emulation* g_emulation = nullptr;

#ifdef _WIN32

void getWinCmdLine(int& argc, char**& argv)
{
    int nArgs;
    LPWSTR *wargs = CommandLineToArgvW(GetCommandLineW(), &nArgs);

    int* argSizes = new int[nArgs];

    int bytesNeeded = 0;
    for (int i = 0; i < nArgs; i++) {
        argSizes[i] = WideCharToMultiByte(CP_UTF8, 0, wargs[i], -1, 0, 0, 0, 0);
        bytesNeeded += argSizes[i];
    }
    bytesNeeded += nArgs * sizeof(char*);

    void* buf = new char[bytesNeeded];

    argc = nArgs;
    argv = reinterpret_cast<char**>(buf);

    char* args = reinterpret_cast<char*>(buf) + nArgs * sizeof(char*);

    for (int i = 0; i < nArgs; i++) {
        argv[i] = args;
        args += WideCharToMultiByte(CP_UTF8, 0, wargs[i], -1, args, argSizes[i], 0, 0);
    }

    delete[] argSizes;
}

#endif


void displayCmdLineHelp()
{
    palMsgBox("Usage: " EXE_NAME " [options]\n\n"\
            "Options are:\n"\
            " --platform <platform_name>\n"\
            " --conf-file <conf_file>\n"\
            " --post-conf <post_conf_file>\n"\
            " --run <file_to_run>\n"\
            " --load <file_to_load>\n"\
            " --disk-a <image_file>\n"\
            " --disk-b <image_file>\n"\
            " --disk-c <image_file>\n"\
            " --disk-d <image_file>\n"\
            " --hdd <image_file>\n"\
            " --edd <image_file>\n"\
            " --edd2 <image_file>\n\n"\
            "For more help see \"Emu80 v4 Manual.rtf\"\n");
}

int main (int argc, char** argv)
{
#ifdef _WIN32
    getWinCmdLine(argc, argv);
#endif

    CmdLine cmdLine(argc, argv);

    if (!palInit(argc, argv)) {
#ifdef _WIN32
        delete[] argv;
#endif
        return 1;
    }

#ifdef _WIN32
    delete[] argv;
#endif

    if (cmdLine.checkParam("help")) {
        displayCmdLineHelp();
        return 0;
    }

    auto warnings = cmdLine.getWarnings();
    if (!warnings.empty())
        palMsgBox("Warnings:\n\n" + warnings + "\nFor brief help: " EXE_NAME " --help");

    new Emulation(cmdLine); // g_emulation присваивается в конструкторе

    palStart();
    palExecute();

    delete g_emulation;

    palQuit();

    return 0;
}
