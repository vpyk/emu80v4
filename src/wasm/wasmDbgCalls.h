/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2025
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

#ifndef WASMDBGCALLS_H
#define WASMDBGCALLS_H

#include <emscripten.h>

void palDebugReqest();

extern "C" {
    void EMSCRIPTEN_KEEPALIVE wasmDbgPause();
    void EMSCRIPTEN_KEEPALIVE wasmDbgRun();
    void EMSCRIPTEN_KEEPALIVE wasmDbgStepIn();
    void EMSCRIPTEN_KEEPALIVE wasmDbgStepOver();
    void EMSCRIPTEN_KEEPALIVE wasmDbgStepOut();
    void EMSCRIPTEN_KEEPALIVE wasmDbgSetBreakpoints(int* bps, int size);
    void EMSCRIPTEN_KEEPALIVE wasmDbgDelBreakpoints(int* bps, int size);
    void EMSCRIPTEN_KEEPALIVE wasmDbgSetRegister(int regId, int value);
    void EMSCRIPTEN_KEEPALIVE wasmDbgWriteByte(int addr, int value);
    void* EMSCRIPTEN_KEEPALIVE wasmDbgGetState();
    void EMSCRIPTEN_KEEPALIVE wasmDbgFreeState();
}

#endif // WASMDBGCALLS_H
