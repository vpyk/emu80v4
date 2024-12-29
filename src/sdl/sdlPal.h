/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2024
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

#ifndef SDLPAL_H
#define SDLPAL_H

#include <stdbool.h>
#include <string>
#include <map>
#include <list>

#include "../EmuTypes.h"
#include "../PalKeys.h"

class PalWindow;

bool palSdlInit();
void palSdlQuit();

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

bool palSetFrameRate(int frameRate);
bool palSetVsync(bool vsync);

std::string palMakeFullFileName(std::string fileName);
int palReadFromFile(const std::string& fileName, int first, int size, uint8_t* buffer, bool useBasePath = true);
uint8_t* palReadFile(const std::string& fileName, int &fileSize, bool useBasePath = true);

#ifndef PAL_WASM
void palRequestForQuit();
#endif //PAL_WASM

void palPlaySample(int16_t sample);
void palPlaySample(int16_t left, int16_t right);

std::string palGetDefaultPlatform();

void palCopyTextToClipboard(const char* text);
std::string palGetTextFromClipboard();

#ifdef PAL_WASM
std::string palOpenFileDialog(std::string title, std::string filter, bool write, PalWindow* window = nullptr);
#endif //PAL_WASM


#endif // SDLPAL_H
