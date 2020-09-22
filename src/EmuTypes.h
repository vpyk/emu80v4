﻿/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2019
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

#ifndef EMUTYPES_H
#define EMUTYPES_H

#include <string>


struct EmuPixelData
{
    int width;
    int height;
    uint32_t* pixelData;
    double aspectRatio;

    int prevWidth;
    int prevHeight;
    uint32_t* prevPixelData;
    double prevAspectRatio;

    unsigned frameNo = 0;
};


enum SysReq
{
    SR_NONE = 0,
    SR_CLOSE,
    SR_RESET,
    SR_MENU,
    SR_HELP,
    SR_LOAD,
    SR_LOADRUN,
    SR_LOADWAV,
    SR_ABOUT,
    SR_FULLSCREEN,
    SR_CHPLATFORM,
    SR_CHCONFIG,
    SR_CONFIG,
    SR_DEBUG,
    SR_QUERTY,
    SR_JCUKEN,
    SR_SMART,
    SR_DISKA,
    SR_DISKB,
    SR_FONT,
    SR_COLOR,
    SR_1X,
    SR_2X,
    SR_3X,
    SR_FIT,
    SR_MAXIMIZE,
    SR_CROPTOVISIBLE,
    SR_ASPECTCORRECTION,
    SR_WIDESCREEN,
    SR_ANTIALIASING,
    SR_EXIT,
    SR_SPEEDUP,
    SR_SPEEDNORMAL,
    SR_PAUSE,
    SR_PAUSEON,
    SR_PAUSEOFF,
    SR_SCREENSHOT,
    SR_MUTE,
    SR_LOADRAMDISK,
    SR_SAVERAMDISK
};


enum EmuWindowType {
    EWT_UNDEFINED,
    EWT_EMULATION,
    EWT_DEBUG
};

struct PlatformInfo
{
    std::string platformName;
    std::string configFileName;
    std::string objName;
    std::string cmdLineOption;
};


struct SelectItem
{
    std::string value;
    std::string name;
    bool selected;
};

enum TabIds
{
    TABID_NONE = 0,
    TABID_GENERAL = -1,
    TABID_HELP = -2
};

enum CodePage {
    CP_RK,
    CP_KOI8
};


#endif // EMUTYPES_H
