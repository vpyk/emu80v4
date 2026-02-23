/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2026
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

#ifndef PALWX_H
#define PALWX_H

#include <string>
#include <vector>
#include <list>
#include <map>

#include "../EmuTypes.h"

class PalWindow;

bool palWxInit(int argc, char** argv);
void palWxQuit();

std::string palOpenFileDialog(const std::string& title, const std::string& filter, bool write, PalWindow* window = nullptr);

bool palChoosePlatform(std::vector<PlatformInfo>& pi, int& pos, bool& newWnd, bool setDef = false, PalWindow* wnd = nullptr);
bool palChooseConfiguration(const std::string& platformName, PalWindow* wnd);
void palSetRunFileName(const std::string& runFileName);
void palShowConfigWindow(int curTabId = 0);
void palUpdateConfig();
void palGetPalDefines(std::list<std::string>& difineList);
void palGetPlatformDefines(const std::string& platformName, std::map<std::string, std::string>& definesMap);

void palAddTabToConfigWindow(int tabId, const std::string& tabName);
void palRemoveTabFromConfigWindow(int tabId);
void palAddRadioSelectorToTab(int tabId, int column, const std::string& caption, const std::string& object, const std::string& property, SelectItem* items, int nItems);
void palSetTabOptFileName(int tabId, std::string optFileName);

void palWxProcessMessages();

void palLog(const std::string& s);

void palMsgBox(const std::string& msg, bool critical = false);

class EmuLog
{
    public:
        EmuLog& operator<<(const std::string& s);
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

#endif // PALWX_H
