/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2018
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

#include <string.h>

#include <SDL2/SDL.h>

#include "sdlPal.h"
#include "sdlPalWindow.h"

#include "../Pal.h"
#include "../PalKeys.h"
#include "../EmuCalls.h"
#include "../EmuTypes.h"
#include "../Shortcuts.h"

using namespace std;

#define BUF_NUM 3

static string basePath;

static SDL_AudioDeviceID audioDevId;

const int audioBufferSize = 2048;
static int16_t audioBuffer[BUF_NUM][audioBufferSize];
static int audioBufferPos;
static int audioBufferNumIn;
static int audioBufferNumOut;

static uint16_t lastSample;

//static bool quitReq = false;

static void audioCallback(void* userdata, Uint8* stream, int len);


static int sampleRate = 48000;

static bool isRunning = false;


bool palSdlInit()
{
    // SDL 2.0.5 windows issue
    // https://forums.libsdl.org/viewtopic.php?p=52273
    // https://bugzilla.libsdl.org/show_bug.cgi?id=2089
    SDL_SetHint(SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING, "1");

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        return false;
    ::basePath = SDL_GetBasePath();

    return true;
}


// Перенести в palExecute
void palStart()
{
    SDL_AudioSpec spec;
    spec.freq = sampleRate;
    spec.format = AUDIO_S16;
    spec.channels = 1;
    spec.samples = 2048;
    spec.callback = audioCallback;

    audioBufferPos = 1024;
    audioBufferNumIn = 1;
    audioBufferNumOut = 0;
    audioDevId = SDL_OpenAudioDevice(NULL, false, &spec, &spec, 0);
    SDL_PauseAudioDevice(audioDevId, false);

    isRunning = true;
}


void palPause()
{
    SDL_PauseAudioDevice(audioDevId, true);
}


void palResume()
{
    SDL_PauseAudioDevice(audioDevId, false);
}

static bool palProcessEvents();

void palExecute()
{
    while (!palProcessEvents())
        emuEmulationCycle();
}


void palSdlQuit()
{
    SDL_CloseAudioDevice(audioDevId);

    SDL_Quit();
}


bool palSetSampleRate(int sampleRate)
{
    if (isRunning)
        return false;
    ::sampleRate = sampleRate;
    return true;
}


int palGetSampleRate()
{
    return ::sampleRate;
}


bool palSetFrameRate(int)
{
    // nothing to do in SDL version
    return true;
}


string palMakeFullFileName(string fileName)
{
    if (fileName[0] == '/' || fileName[0] == '\\' || (fileName.size() > 1 && fileName[1] == ':'))
        return fileName;
    string fullFileName(::basePath);
    fullFileName += fileName;
    return fullFileName;
}


int palReadFromFile(const string& fileName, int offset, int sizeToRead, uint8_t* buffer, bool useBasePath)
{
    string fullFileName;
    if (useBasePath)
        fullFileName = palMakeFullFileName(fileName);
    else
        fullFileName = fileName;

    int nBytesRead;
    SDL_RWops* file = SDL_RWFromFile(fullFileName.c_str(), "r");
    if (file != NULL) {
        SDL_RWseek(file, offset, RW_SEEK_SET);
        nBytesRead=SDL_RWread(file, buffer, 1, sizeToRead);
        SDL_RWclose(file);
        return nBytesRead;
    }
    else
        return 0;
}


uint8_t* palReadFile(const string& fileName, int &fileSize, bool useBasePath)
{
    string fullFileName;
    if (useBasePath)
        fullFileName = palMakeFullFileName(fileName);
    else
        fullFileName = fileName;

    SDL_RWops* file = SDL_RWFromFile(fullFileName.c_str(), "r");
    if (file != NULL) {
        fileSize = SDL_RWsize(file);
        if (fileSize < 0) {
            fileSize = 0;
            return nullptr;
        }

        uint8_t* buf = new uint8_t[fileSize];

        int nBytesRead = SDL_RWread(file, buf, 1, fileSize);
        SDL_RWclose(file);
        fileSize = nBytesRead;
        return buf;
    }
    else
        return nullptr;
}


static PalKeyCode TranslateScanCode(SDL_Scancode scanCode)
{
    switch (scanCode) {
        case SDL_SCANCODE_A:
            return PK_A;
        case SDL_SCANCODE_B:
            return PK_B;
        case SDL_SCANCODE_C:
            return PK_C;
        case SDL_SCANCODE_D:
            return PK_D;
        case SDL_SCANCODE_E:
            return PK_E;
        case SDL_SCANCODE_F:
            return PK_F;
        case SDL_SCANCODE_G:
            return PK_G;
        case SDL_SCANCODE_H:
            return PK_H;
        case SDL_SCANCODE_I:
            return PK_I;
        case SDL_SCANCODE_J:
            return PK_J;
        case SDL_SCANCODE_K:
            return PK_K;
        case SDL_SCANCODE_L:
            return PK_L;
        case SDL_SCANCODE_M:
            return PK_M;
        case SDL_SCANCODE_N:
            return PK_N;
        case SDL_SCANCODE_O:
            return PK_O;
        case SDL_SCANCODE_P:
            return PK_P;
        case SDL_SCANCODE_Q:
            return PK_Q;
        case SDL_SCANCODE_R:
            return PK_R;
        case SDL_SCANCODE_S:
            return PK_S;
        case SDL_SCANCODE_T:
            return PK_T;
        case SDL_SCANCODE_U:
            return PK_U;
        case SDL_SCANCODE_V:
            return PK_V;
        case SDL_SCANCODE_W:
            return PK_W;
        case SDL_SCANCODE_X:
            return PK_X;
        case SDL_SCANCODE_Y:
            return PK_Y;
        case SDL_SCANCODE_Z:
            return PK_Z;

        case SDL_SCANCODE_1:
            return PK_1;
        case SDL_SCANCODE_2:
            return PK_2;
        case SDL_SCANCODE_3:
            return PK_3;
        case SDL_SCANCODE_4:
            return PK_4;
        case SDL_SCANCODE_5:
            return PK_5;
        case SDL_SCANCODE_6:
            return PK_6;
        case SDL_SCANCODE_7:
            return PK_7;
        case SDL_SCANCODE_8:
            return PK_8;
        case SDL_SCANCODE_9:
            return PK_9;
        case SDL_SCANCODE_0:
            return PK_0;

        case SDL_SCANCODE_RETURN:
            return PK_ENTER;
        case SDL_SCANCODE_ESCAPE:
            return PK_ESC  ;
        case SDL_SCANCODE_BACKSPACE:
            return PK_BSP;
        case SDL_SCANCODE_TAB:
            return PK_TAB  ;
        case SDL_SCANCODE_SPACE:
            return PK_SPACE;

        case SDL_SCANCODE_MINUS:
            return PK_MINUS;
        case SDL_SCANCODE_EQUALS:
            return PK_EQU;
        case SDL_SCANCODE_LEFTBRACKET:
            return PK_LBRACKET;
        case SDL_SCANCODE_RIGHTBRACKET:
            return PK_RBRACKET;
        case SDL_SCANCODE_BACKSLASH:
        case SDL_SCANCODE_NONUSHASH:
        case SDL_SCANCODE_NONUSBACKSLASH:
            return PK_BSLASH;
        case SDL_SCANCODE_SEMICOLON:
            return PK_SEMICOLON ;
        case SDL_SCANCODE_APOSTROPHE:
            return PK_APOSTROPHE;
        case SDL_SCANCODE_GRAVE:
            return PK_TILDE     ;
        case SDL_SCANCODE_COMMA:
            return PK_COMMA     ;
        case SDL_SCANCODE_PERIOD:
            return PK_PERIOD    ;
        case SDL_SCANCODE_SLASH:
            return PK_SLASH     ;

        case SDL_SCANCODE_CAPSLOCK:
            return PK_CAPSLOCK;

        case SDL_SCANCODE_F1:
            return PK_F1 ;
        case SDL_SCANCODE_F2:
            return PK_F2 ;
        case SDL_SCANCODE_F3:
            return PK_F3 ;
        case SDL_SCANCODE_F4:
            return PK_F4 ;
        case SDL_SCANCODE_F5:
            return PK_F5 ;
        case SDL_SCANCODE_F6:
            return PK_F6 ;
        case SDL_SCANCODE_F7:
            return PK_F7 ;
        case SDL_SCANCODE_F8:
            return PK_F8 ;
        case SDL_SCANCODE_F9:
            return PK_F9 ;
        case SDL_SCANCODE_F10:
            return PK_F10;
        case SDL_SCANCODE_F11:
            return PK_F11;
        case SDL_SCANCODE_F12:
            return PK_F12;

        case SDL_SCANCODE_PRINTSCREEN:
            return PK_PRSCR;
        case SDL_SCANCODE_SCROLLLOCK:
            return PK_SCRLOCK ;
        case SDL_SCANCODE_PAUSE:
            return PK_PAUSEBRK;

        case SDL_SCANCODE_INSERT:
            return PK_INS;
        case SDL_SCANCODE_HOME:
            return PK_HOME ;
        case SDL_SCANCODE_PAGEUP:
            return PK_PGUP ;
        case SDL_SCANCODE_DELETE:
            return PK_DEL;
        case SDL_SCANCODE_END:
            return PK_END;
        case SDL_SCANCODE_PAGEDOWN:
            return PK_PGDN ;
        case SDL_SCANCODE_RIGHT:
            return PK_RIGHT;
        case SDL_SCANCODE_LEFT:
            return PK_LEFT ;
        case SDL_SCANCODE_DOWN:
            return PK_DOWN ;
        case SDL_SCANCODE_UP:
            return PK_UP ;

        case SDL_SCANCODE_NUMLOCKCLEAR:
            return PK_NUMLOCK ;
        case SDL_SCANCODE_KP_DIVIDE:
            return PK_KP_DIV;
        case SDL_SCANCODE_KP_MULTIPLY:
            return PK_KP_MUL;
        case SDL_SCANCODE_KP_MINUS:
            return PK_KP_MINUS;
        case SDL_SCANCODE_KP_PLUS:
            return PK_KP_PLUS ;
        case SDL_SCANCODE_KP_ENTER:
            return PK_KP_ENTER;
        case SDL_SCANCODE_KP_1:
            return PK_KP_1;
        case SDL_SCANCODE_KP_2:
            return PK_KP_2;
        case SDL_SCANCODE_KP_3:
            return PK_KP_3;
        case SDL_SCANCODE_KP_4:
            return PK_KP_4;
        case SDL_SCANCODE_KP_5:
            return PK_KP_5;
        case SDL_SCANCODE_KP_6:
            return PK_KP_6;
        case SDL_SCANCODE_KP_7:
            return PK_KP_7;
        case SDL_SCANCODE_KP_8:
            return PK_KP_8;
        case SDL_SCANCODE_KP_9:
            return PK_KP_9;
        case SDL_SCANCODE_KP_0:
            return PK_KP_0;
        case SDL_SCANCODE_KP_PERIOD:
            return PK_KP_PERIOD;

        case SDL_SCANCODE_LCTRL:
            return PK_LCTRL;
        case SDL_SCANCODE_LSHIFT:
            return PK_LSHIFT;
        case SDL_SCANCODE_LALT:
            return PK_LALT;
        case SDL_SCANCODE_LGUI:
            return PK_LWIN;
        case SDL_SCANCODE_RCTRL:
            return PK_RCTRL;
        case SDL_SCANCODE_RSHIFT:
            return PK_RSHIFT;
        case SDL_SCANCODE_RALT:
            return PK_RALT;
        case SDL_SCANCODE_RGUI:
            return PK_RWIN;
        case SDL_SCANCODE_APPLICATION:
            return PK_MENU;

        default:
            return PK_NONE;
    }
}


static bool palProcessEvents()
{
    palIdle();
    // workaround for wxWidgets events processing
    for (int i = 0; i < 10; i++)
        SDL_PumpEvents();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
                return true;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                {
                    if (!SDL_GetWindowFromID(event.key.windowID))
                        break; // могут остаться события, относящиеся к уже уделенному окну
                    PalKeyCode key = TranslateScanCode(event.key.keysym.scancode);
                    SysReq sr = TranslateKeyToSysReq(key, event.type == SDL_KEYDOWN, SDL_GetModState() & (KMOD_ALT | KMOD_GUI));
                    if (sr)
                        emuSysReq(PalWindow::windowById(event.key.windowID), sr);
                    else
                        emuKeyboard(PalWindow::windowById(event.key.windowID), key, event.type == SDL_KEYDOWN);
                    break;
                }
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED && SDL_GetWindowFromID(event.window.windowID))
                    emuFocusWnd(PalWindow::windowById(event.window.windowID));
                else if (event.window.event == SDL_WINDOWEVENT_CLOSE  && SDL_GetWindowFromID(event.window.windowID))
                    emuSysReq(PalWindow::windowById(event.window.windowID), SR_CLOSE);
                break;
            case SDL_DROPFILE:
                if (SDL_GetWindowFromID(event.drop.windowID))
                    emuDropFile(PalWindow::windowById(event.drop.windowID), event.drop.file);
                break;
        }
    }

    return false;
}


void palRequestForQuit()
{
    SDL_Event ev;
    ev.type = SDL_QUIT;
    SDL_PushEvent(&ev);
}


void audioCallback(void*, Uint8* stream, int len)
{
    //cout << audioBufferNumIn << ":" << audioBufferNumOut << " ";
    //cout << (audioBufferNumIn + BUF_NUM - audioBufferNumOut) % BUF_NUM << ":" << audioBufferPos << " ";

    if (len != audioBufferSize * 2)
        return; // error
    if (audioBufferNumIn == audioBufferNumOut) {
        //memset(stream, 0, len);
        for (int i = audioBufferPos; i < audioBufferSize; i++)
            ((uint16_t*)stream)[i] = ::lastSample;
        audioBufferPos = 0;
    } else {
        memcpy(stream, audioBuffer[audioBufferNumOut], len);
        audioBufferNumOut = (audioBufferNumOut + 1) % BUF_NUM;
    }
}


void palPlaySample(int16_t sample)
{
    ::lastSample = sample;
    audioBuffer[audioBufferNumIn][audioBufferPos++] = sample;
    if (audioBufferPos == audioBufferSize) {
        audioBufferPos = 0;
        audioBufferNumIn = (audioBufferNumIn + 1) % BUF_NUM;
    }
}


uint64_t palGetCounter()
{
    return SDL_GetPerformanceCounter();
}


uint64_t palGetCounterFreq()
{
    return SDL_GetPerformanceFrequency();
}


void palDelay(uint64_t time)
{
    SDL_Delay(time * 1000 / SDL_GetPerformanceFrequency());
}
