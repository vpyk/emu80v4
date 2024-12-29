/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2024
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

#ifndef QTPAL_H
#define QTPAL_H

#include <stdbool.h>
#include <string>
#include <list>
#include <vector>
#include <map>

#include "../EmuTypes.h"
#include "../PalKeys.h"

class PalWindow;

bool palQtInit(int& argc, char** argv);
void palQtQuit();

const std::string& palGetBasePath();

void palStart();
void palPause();
void palResume();

void palExecute();

uint64_t palGetCounter();
uint64_t palGetCounterFreq();
void palDelay(uint64_t time);

bool palSetSampleRate(int sampleRate);
int palGetSampleRate();

//bool palSetFrameRate(int frameRate);
bool palSetVsync(bool vsync);

std::string palMakeFullFileName(std::string fileName);
int palReadFromFile(const std::string& fileName, int first, int size, uint8_t* buffer, bool useBasePath = true);
uint8_t* palReadFile(const std::string& fileName, int &fileSize, bool useBasePath = true);

//bool palProcessEvents();

void palRequestForQuit();

void palPlaySample(int16_t sample); // mono, not used for now
void palPlaySample(int16_t left, int16_t right); // stereo

std::string palOpenFileDialog(std::string title, std::string filter, bool write, PalWindow* window = nullptr);

void palCopyTextToClipboard(const char* text);
std::string palGetTextFromClipboard();

bool palChoosePlatform(std::vector<PlatformInfo>& pi, int& pos, bool& newWnd, bool setDef = false, PalWindow* wnd = nullptr);
bool palChooseConfiguration(std::string platformName, PalWindow* wnd);
void palSetRunFileName(std::string runFileName);
void palShowConfigWindow(int curTabId = 0);
void palUpdateConfig();
std::string palGetDefaultPlatform();
void palGetPalDefines(std::list<std::string>& difineList);
void palGetPlatformDefines(std::string platformName, std::map<std::string, std::string>& definesMap);

void palAddTabToConfigWindow(int tabId, std::string tabName);
void palRemoveTabFromConfigWindow(int tabId);
void palAddRadioSelectorToTab(int tabId, int column, std::string caption, std::string object, std::string property, SelectItem* items, int nItems);
void palSetTabOptFileName(int tabId, std::string optFileName);

void palRegisterSetPropValueCallbackFunc(bool (*func)(const std::string&, const std::string&, const std::string&));
void palRegisterGetPropertyStringValueFunc(std::string (*func)(const std::string&, const std::string&));

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


#endif // QTPAL_H
