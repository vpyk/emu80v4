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

#ifndef WASMEMUCALLS_H
#define WASMEMUCALLS_H

#include <cstdint>

#include <emscripten.h>

#include "wasmPal.h"

void wasmSetWindowId(void* window);

void wasmDeleteUserFile();

int wasmReadFromFile(int offset, int sizeToRead, uint8_t* buffer);
uint8_t* wasmReadFile(int &fileSize);

extern "C" {
    void EMSCRIPTEN_KEEPALIVE wasmEmuSysReq(int id);
    void EMSCRIPTEN_KEEPALIVE wasmEmuExitFullscreenMode();
    int EMSCRIPTEN_KEEPALIVE wasmEmuAllocateFileBuf(int size);
    uint8_t* EMSCRIPTEN_KEEPALIVE wasmEmuGetFileBufPtr(int fileId);

    void EMSCRIPTEN_KEEPALIVE wasmEmuRunPlatform(const char* platform);
    void EMSCRIPTEN_KEEPALIVE wasmEmuOpenFile(int fileId);
    char* EMSCRIPTEN_KEEPALIVE wasmEmuGetPropertyValue(char* objName, char* propName);
    void EMSCRIPTEN_KEEPALIVE wasmEmuSetPropertyValue(char* objName, char* propName, char* value);
}

#endif // WASMEMUCALLS_H
