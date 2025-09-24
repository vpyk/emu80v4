/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2025
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

// platform.h

#ifndef PLATFORM_H
#define PLATFORM_H

#include <map>

#include "PalKeys.h"
#include "EmuTypes.h"
#include "EmuObjects.h"

class EmuWindow;
class Cpu;
class FileLoader;
class RamDisk;
class PlatformCore;
class KbdLayout;
class CrtRenderer;
class Keyboard;
class DiskImage;
class DebugWindow;
class KbdTapper;
class IDebugger;


class Platform : public ParentObject
{
    public:
        //Platform();
        Platform(std::string configFileName, std::string name = "");
        virtual ~Platform();
        void addChild(EmuObject* child) override;
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;
        void init() override;
        void shutdown() override;
        void reset() override;

        void sysReq(SysReq sr);
        virtual void draw();
        void processKey(PalKeyCode keyCode, bool isPressed, unsigned unicodeKey = 0);
        void resetKeys();
        bool loadFile(std::string fileName, bool run = true);
        void mouseDrag(int x, int y);
        void updateScreenOnce();

        const std::string& getBaseDir() {return m_baseDir;}

        EmuWindow* getWindow() {return m_window;}
        Cpu* getCpu() {return m_cpu;}
        FileLoader* getLoader() {return m_loader;}
        PlatformCore* getCore() {return m_core;}
        KbdLayout* getKbdLayout() {return m_kbdLayout;}
        CrtRenderer* getRenderer() {return m_renderer;}
        Keyboard* getKeyboard() {return m_keyboard;}

        IDebugger* getDebugger() {return m_debugger;}

        void showDebugger();
        void updateDebugger();
        void reqScreenUpdateForDebug();
        std::string getAllDebugInfo();
        CodePage getCodePage() {return m_codePage;}
        bool getMuteTapeFlag() {return m_muteTape;}

        const std::string& getBaseName() {return m_baseName;}
        int getDefConfigTabId() {return m_defConfigTabId;}

    private:
        std::string m_baseDir;
        std::list<EmuObject* >m_objList;
        std::string m_baseName;

        PlatformCore* m_core = nullptr;
        Cpu* m_cpu = nullptr;
        EmuWindow* m_window = nullptr;
        KbdLayout* m_kbdLayout = nullptr;
        CrtRenderer* m_renderer = nullptr;
        CrtRenderer* m_renderer2 = nullptr;
        DiskImage* m_diskA = nullptr;
        DiskImage* m_diskB = nullptr;
        DiskImage* m_diskC = nullptr;
        DiskImage* m_diskD = nullptr;
        DiskImage* m_hdd = nullptr;
        FileLoader* m_loader = nullptr;
        Keyboard* m_keyboard = nullptr;
        RamDisk* m_ramDisk = nullptr;
        RamDisk* m_ramDisk2 = nullptr;
        EmuObjectGroup* m_tapeGrp = nullptr;
        KbdTapper* m_kbdTapper = nullptr;

        int m_defConfigTabId = 0;

        IDebugger* m_debugger = nullptr;

        std::string m_helpFile = "";
        CodePage m_codePage = CP_RK;
        bool m_muteTape = false;
        bool m_fastReset = false;
        int m_fastResetCpuTicks = 0;
};


#endif  // PLATFORM_H
