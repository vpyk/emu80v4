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
#include <iostream>

#ifdef __WIN32__
    #include <mem.h>
    #include <windows.h>
    #include <commdlg.h>
#else
    #include <dirent.h>
    #include <sys/stat.h>
    #include <time.h>
#endif // __WIN32__

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


void palGetDirContent(const string& dir, list<PalFileInfo*>& fileList)
{
    string utf8Mask = dir;

    if (utf8Mask[utf8Mask.size() - 1] != '/' && utf8Mask[utf8Mask.size() - 1] != '\\')
        utf8Mask += "/";

    utf8Mask += "/*";

    wchar_t* wideMask = new wchar_t[utf8Mask.size() * 4];
    MultiByteToWideChar(CP_UTF8, 0, utf8Mask.c_str(), -1, wideMask, utf8Mask.size() * 4);

    WIN32_FIND_DATAW fd;
    HANDLE hf;
    hf = FindFirstFileW(wideMask, &fd);

    if (hf != INVALID_HANDLE_VALUE) {
        do {
            /*if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;*/
            char* utf8Name = new char[wcslen(fd.cFileName) * 4 + 4];
            WideCharToMultiByte(CP_UTF8, 0, fd.cFileName, -1, utf8Name, wcslen(fd.cFileName) * 4 + 4, 0, 0);

            if (string(utf8Name) != "." && string(utf8Name) != "..") {
                PalFileInfo* newFile = new PalFileInfo;
                newFile->fileName = utf8Name;
                newFile->isDir = fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
                newFile->size = fd.nFileSizeLow;
                FILETIME fileTime = fd.ftLastWriteTime;
                SYSTEMTIME stUtc, stLocal;
                FileTimeToSystemTime(&fileTime, &stUtc);
                SystemTimeToTzSpecificLocalTime(NULL, &stUtc, &stLocal);
                newFile->year = stLocal.wYear;
                newFile->month = stLocal.wMonth;
                newFile->day = stLocal.wDay;
                newFile->hour = stLocal.wHour;
                newFile->minute = stLocal.wMinute;
                newFile->second = stLocal.wSecond;

                fileList.push_back(newFile);
            }

            delete[] utf8Name;
        } while (FindNextFileW(hf, &fd) != 0);
        FindClose(hf);
    }

    delete[] wideMask;
}


void palUpdateConfig() {
}

#else

#ifndef PAL_WASM
std::string palOpenFileDialog(std::string, std::string, bool, PalWindow*) {
    return "";
}

void palUpdateConfig() {
}
#endif //!PAL_WASM

void palGetDirContent(const string& dir, list<PalFileInfo*>& fileList)
{
    DIR* pDir;
    dirent* dp;

    string utf8Dir = dir;

    if (utf8Dir[utf8Dir.size() - 1] != '/' && utf8Dir[utf8Dir.size() - 1] != '\\')
        utf8Dir += "/";

    if ((pDir = opendir(utf8Dir.c_str()))) {
        while ((dp = readdir(pDir))) {
            //if ((dp->d_name == '.') || (dp->d_name == '..')
            //    continue;
            PalFileInfo* newFile = new PalFileInfo;

            newFile->fileName = dp->d_name;

            if (newFile->fileName == "." || newFile->fileName == "..")
                continue;

            string fullPath = utf8Dir + newFile->fileName;

            struct stat entryInfo;
            if( stat( fullPath.c_str(), &entryInfo ) != 0 )
                continue;

            newFile->isDir = S_ISDIR( entryInfo.st_mode);
            newFile->size = (uint32_t)entryInfo.st_size;


            struct tm *fileDateTime;

            fileDateTime = gmtime(&(entryInfo.st_mtime));

            newFile->year = fileDateTime->tm_year + 1900;
            newFile->month = fileDateTime->tm_mon + 1;
            newFile->day = fileDateTime->tm_mday;
            newFile->hour = fileDateTime->tm_hour;
            newFile->minute = fileDateTime->tm_min;
            newFile->second = fileDateTime->tm_sec;

            fileList.push_back(newFile);
        }
    }
}

#endif


bool palChoosePlatform(std::vector<PlatformInfo>&, int&, bool&, bool, PalWindow*) {
    return false;
}


bool palChooseConfiguration(std::string platformName, PalWindow* wnd) {
    return false;
}


void palGetPalDefines(std::list<std::string>& defineList)
{
    defineList.push_back("SDL");
}


void palGetPlatformDefines(std::string platformName, std::map<std::string, std::string>& definesMap)
{

}


void palSetRunFileName(std::string) {
}

void palShowConfigWindow(int) {
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

void palLog(std::string s) {
    cout << s << endl;
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


void palMsgBox(string msg, bool)
{
    cout << msg << endl;
}


EmuLog emuLog;
