/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2018
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

#include <sstream>

#ifdef __WIN32__
#include <mem.h>
#include <windows.h>
#include <commdlg.h>
#endif // WIN32

#include "litePal.h"

using namespace std;

#ifdef __WIN32__

string palOpenFileDialog(string title, string filter, bool write, PalWindow* window)
{
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = 0;

    wchar_t* filterStr = new wchar_t[filter.size() * 4 + 1];
    MultiByteToWideChar(CP_UTF8, 0, filter.c_str(), -1, filterStr, filter.size() * 4);
    filterStr[wcslen(filterStr) + 1] = L'\0';
    unsigned len = wcslen(filterStr);
    for (unsigned i = 0; i < len; i++)
        if (filterStr[i] == L'|')
            filterStr[i] = L'\0';
    ofn.lpstrFilter = filterStr;

    wchar_t* titleStr = new wchar_t[title.size() * 4];
    MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, titleStr, title.size() * 4);
    titleStr[wcslen(filterStr) + 1] = L'\0';
    ofn.lpstrTitle = titleStr;

    wchar_t str[MAX_PATH + 1] = L"";
    ofn.lpstrFile = str;

    ofn.nFilterIndex = 1;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = write ? 0 : OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    //ofn.lpstrDefExt = "rk";
    if (write ? GetOpenFileNameW(&ofn) : GetSaveFileNameW(&ofn)) {
        int len = wcslen(str);
        char* utf8str = new char[len * 4]; // с запасом
        WideCharToMultiByte(CP_UTF8, 0, str, -1, utf8str, len * 4, 0, 0);
        string s = utf8str;
        delete[] utf8str;
        delete[] filterStr;
        delete[] titleStr;
        return s;
    }
    delete[] filterStr;
    delete[] titleStr;
    return "";
}

#else

std::string palOpenFileDialog(std::string, std::string, bool, PalWindow*) {
    return "";
}

#endif

bool palChoosePlatform(std::vector<PlatformInfo>&, int&, bool&, bool, PalWindow*) {
    return false;
}


void palGetPalDefines(std::list<std::string>& defineList)
{
    defineList.push_back("SDL");
}


void palSetRunFileName(std::string) {
}

void palShowConfigWindow(int) {
}

void palUpdateConfig() {
}

void palAddTabToConfigWindow(int, std::string) {
}

void palRemoveTabFromConfigWindow(int) {
}

void palAddRadioSelectorToTab(int, int, std::string, std::string, std::string, SelectItem*, int) {
}

void palSetTabOptFileName(int, string) {
}

void palWxProcessMessages() {
}

void palLog(std::string) {
}

EmuLog& EmuLog::operator<<(string s)
{
    palLog(s);
    return *this;
}


EmuLog& EmuLog::operator<<(const char* sz)
{
    string s = sz;
    palLog(s);
    return *this;
}


EmuLog& EmuLog::operator<<(int n)
{
    ostringstream oss;
    oss << n;
    string s = oss.str();
    palLog(s);
    return *this;
}

EmuLog emuLog;
