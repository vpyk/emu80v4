/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2024
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

#include "wasmEmuCalls.h"

#include "../Shortcuts.h"
#include "../PalWindow.h"
#include "../Pal.h"
#include "../EmuCalls.h"

using namespace std;


static int curFileId = 0;

static void* window = nullptr;

static uint8_t* userFileBuf = nullptr;
static int userFileSize = 0;


void wasmEmuSysReq(int id)
{
    emuSysReq((PalWindow*)(window), static_cast<SysReq>(id));
}


int wasmEmuAllocateFileBuf(int size)
{
    if (userFileBuf)
        delete[] userFileBuf;

    userFileBuf = new uint8_t[size];
    userFileSize = size;

    return ++curFileId;
}


uint8_t* wasmEmuGetFileBufPtr(int fileId)
{
    if (fileId != curFileId)
        return nullptr;

    return userFileBuf;
}


void wasmEmuOpenFile(int /*fileId*/)
{
    emuDropFile((PalWindow*)window, "$"); // special file name
}


void wasmEmuExitFullscreenMode()
{
    emuExitFullscreenMode(static_cast<PalWindow*>(window));
}


void wasmEmuRunPlatform(const char* platform)
{
    emuSelectPlatform(platform);
}


void wasmSetWindowId(void* window)
{
    ::window = window;
}


void wasmDeleteUserFile()
{
    if (userFileBuf)
        delete[] userFileBuf;
    userFileBuf = nullptr;
}

int wasmReadFromFile(int offset, int sizeToRead, uint8_t* buffer)
{
    if (!userFileBuf)
        return 0;

    if (sizeToRead > userFileSize)
        sizeToRead = userFileSize;
    memcpy(buffer, userFileBuf + offset, sizeToRead);

    wasmDeleteUserFile();

    return sizeToRead;
}


uint8_t* wasmReadFile(int &fileSize)
{
    if (!userFileBuf)
        return nullptr;

    fileSize = userFileSize;
    uint8_t* res = userFileBuf;
    userFileBuf = nullptr;
    return res;
}


EM_JS(void, jsUpdateConfig, (), {
    if (typeof window.top.updateConfig === "function")
        window.top.updateConfig();
});

void palUpdateConfig()
{
    jsUpdateConfig();
}


EM_JS(void, jsQuitRequest, (), {
    if (typeof window.top.quitRequest === "function")
        window.top.quitRequest();
});

void palRequestForQuit()
{
    jsQuitRequest();
}


char* wasmEmuGetPropertyValue(char* objName, char* propName)
{
    const int MAX_RES_LEN = 32;
    static char buf[MAX_RES_LEN];
    string resultStr = emuGetPropertyValue(objName, propName);
    int resultLen = resultStr.length();
    strncpy(buf, resultStr.c_str(), MAX_RES_LEN - 1);

    return buf;
}


void wasmEmuSetPropertyValue(char* objName, char* propName, char* value)
{
    emuSetPropertyValue(objName, propName, value);
}
