/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2018
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

// Platform Abstraction Layer (lite version)

#ifndef LITEPAL_H
#define LITEPAL_H

#include <string>
#include <vector>
#include <list>
#include <map>

#include "../EmuTypes.h"

class PalWindow;

#ifndef PAL_WASM
std::string palOpenFileDialog(std::string title, std::string filter, bool write, PalWindow* window = nullptr);
void palUpdateConfig();
#endif //!PAL_WASM

bool palChoosePlatform(std::vector<PlatformInfo>& pi, int& pos, bool& newWnd, bool setDef = false, PalWindow* wnd = nullptr);
bool palChooseConfiguration(std::string platformName, PalWindow* wnd);
void palSetRunFileName(std::string runFileName);
void palShowConfigWindow(int curTabId = 0);
void palGetPalDefines(std::list<std::string>& difineList);
void palGetPlatformDefines(std::string platformName, std::map<std::string, std::string>& definesMap);

void palAddTabToConfigWindow(int tabId, std::string tabName);
void palRemoveTabFromConfigWindow(int tabId);
void palAddRadioSelectorToTab(int tabId, int column, std::string caption, std::string object, std::string property, SelectItem* items, int nItems);
void palSetTabOptFileName(int tabId, std::string optFileName);

void palWxProcessMessages();

void palLog(std::string s);

void palMsgBox(std::string msg, bool critical = false);

class EmuLog
{
    public:
        EmuLog& operator<<(std::string s);
        EmuLog& operator<<(const char* sz);
        EmuLog& operator<<(int n);
};

extern EmuLog emuLog;

struct PalFileInfo {
    std::string fileName;
    //char shortLatinFileName[11];
    bool isDir;
    unsigned size;
    int second;
    int minute;
    int hour;
    int day;
    int month;
    int year;
};

void palGetDirContent(const std::string& dir, std::list<PalFileInfo*>& fileList);

#endif // LITEPAL_H
