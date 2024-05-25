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

#include <sstream>

#include "Globals.h"
#include "Platform.h"
#include "Emulation.h"
#include "EmuObjects.h"
#include "ConfigReader.h"
#include "EmuConfig.h"
#include "EmuWindow.h"
#include "Cpu.h"
#include "PlatformCore.h"
#include "KbdLayout.h"
#include "CrtRenderer.h"
#include "DiskImage.h"
#include "FileLoader.h"
#include "Keyboard.h"
#include "RamDisk.h"
#include "Debugger.h"
#include "KbdTapper.h"

using namespace std;

Platform::Platform(string configFileName, string name)
{
    string::size_type slashPos = configFileName.find_last_of("\\/");
    if (slashPos != string::npos)
        m_baseDir = configFileName.substr(0, slashPos) + "/";

    m_baseName = name;

    // Если платформа с таким именем уже есть, добавляем в конец "$" и номер, начиная от 1
    if (g_emulation->findObject(name)) {
        ostringstream oss;
        int i = 1;
        do
            oss << i++;
        while (g_emulation->findObject(name + "$" + oss.str()));
        name = name + "$" + oss.str();
    }

    setName(name);

    ConfigReader cr(configFileName, getName());
    cr.processConfigFile(this);

    // ищем объект-окно, должен быть единственным
    for (auto it = m_objList.begin(); it != m_objList.end(); it++)
        if ((m_window = dynamic_cast<EmuWindow*>(*it)))
            break;

    // ищем объект-прцессор, должен быть единственным
    for (auto it = m_objList.begin(); it != m_objList.end(); it++)
        if ((m_cpu = dynamic_cast<Cpu*>(*it)))
            break;

    // ищем объект-ядро, должен быть единственным
    for (auto it = m_objList.begin(); it != m_objList.end(); it++)
        if ((m_core = dynamic_cast<PlatformCore*>(*it)))
            break;

    // ищем объект - раскладку клавиатуры, должен быть единственным
    for (auto it = m_objList.begin(); it != m_objList.end(); it++)
        if ((m_kbdLayout = dynamic_cast<KbdLayout*>(*it)))
            break;

    // ищем объекты - рендереры, может быть два
    for (auto it = m_objList.begin(); it != m_objList.end(); it++) {
        auto renderer = dynamic_cast<CrtRenderer*>(*it);
        if (renderer) {
            if (!m_renderer)
                m_renderer = renderer;
            else {
                m_renderer2 = renderer;
                break;
            }
        }
    }

    // ищем объекты - образы дисков A и B
    for (auto it = m_objList.begin(); it != m_objList.end(); it++) {
        DiskImage* img = dynamic_cast<DiskImage*>(*it);
        if (img) {
            if (img->getLabel() == "A")
                m_diskA = img;
            else if (img->getLabel() == "B")
                m_diskB = img;
            else if (img->getLabel() == "C")
                m_diskC = img;
            else if (img->getLabel() == "D")
                m_diskD = img;
            else if (img->getLabel() == "HDD")
                m_hdd = img;
        }

    // ищем объект - загрузчик, должен быть единственным
    for (auto it = m_objList.begin(); it != m_objList.end(); it++)
        if ((m_loader = dynamic_cast<FileLoader*>(*it)))
            break;
    }

    // ищем объекты - RAM-диски
    for (auto it = m_objList.begin(); it != m_objList.end(); it++) {
        RamDisk* ramDisk;
        if ((ramDisk = dynamic_cast<RamDisk*>(*it))) {
            if (ramDisk->getLabel() != "EDD2")
                m_ramDisk = ramDisk;
            else
                m_ramDisk2 = ramDisk;
        }
    }

    // ищем объект - закладку в окне конфигурации, должен быть единственным
    for (auto it = m_objList.begin(); it != m_objList.end(); it++) {
        EmuConfigTab* tab = dynamic_cast<EmuConfigTab*>(*it);
        if (tab) {
            m_defConfigTabId = tab->getTabId();
            break;
        }
    }

    // ищем объект - клавиатуру, должен быть единственным
    for (auto it = m_objList.begin(); it != m_objList.end(); it++)
        if ((m_keyboard = dynamic_cast<Keyboard*>(*it)))
            break;

    // ищем объект - группу tapeGrp
    for (auto it = m_objList.begin(); it != m_objList.end(); it++) {
        EmuObjectGroup* grp = dynamic_cast<EmuObjectGroup*>(*it);
        if (grp) {
            name = grp->getName();
            if (name.substr(name.find_last_of(".")) == ".tapeGrp") {
                m_tapeGrp = grp;
                break;
            }
        }
    }

    // ищем объект - KbdTapper, должен быть единственным
    for (auto it = m_objList.begin(); it != m_objList.end(); it++)
        if ((m_kbdTapper = dynamic_cast<KbdTapper*>(*it)))
            break;

    Platform::init();

    Platform::reset();

    if (m_window)
        m_window->show();
}


void Platform::init()
{
    for (auto it = m_objList.begin(); it != m_objList.end(); it++)
        (*it)->init();
}


void Platform::shutdown()
{
    for (auto it = m_objList.begin(); it != m_objList.end(); it++)
        (*it)->shutdown();
}


void Platform::reset()
{
    for (auto it = m_objList.begin(); it != m_objList.end(); it++)
        (*it)->reset();
}


Platform::~Platform()
{
    Platform::shutdown();

    for (auto it = m_objList.begin(); it != m_objList.end(); it++)
        delete *it;

    if (m_dbgWindow)
        delete m_dbgWindow;
}


void Platform::addChild(EmuObject* child)
{
    m_objList.push_back(child);
}


void Platform::sysReq(SysReq sr)
{
    switch (sr) {
        case SR_RESET:
            reset();
            if (m_fastReset && m_fastResetCpuTicks) {
                Cpu* cpu = getCpu();
                cpu->disableHooks();
                g_emulation->exec((int64_t)cpu->getKDiv() * m_fastResetCpuTicks); // no 2d parameter: no fast reset when debugger is active
                cpu->enableHooks();
            }
            updateDebugger();
            break;
        case SR_QUERTY:
            if (m_kbdLayout) {
                m_kbdLayout->setQwertyMode();
                g_emulation->getConfig()->updateConfig();
            }
            break;
        case SR_JCUKEN:
            if (m_kbdLayout) {
                m_kbdLayout->setJcukenMode();
                g_emulation->getConfig()->updateConfig();
            }
            break;
        case SR_SMART:
            if (m_kbdLayout) {
                m_kbdLayout->setSmartMode();
                g_emulation->getConfig()->updateConfig();
            }
            break;
        case SR_FONT:
            if (m_renderer) {
                m_renderer->toggleRenderingMethod();
                g_emulation->getConfig()->updateConfig();
            }
            break;
        case SR_CROPTOVISIBLE:
            if (m_renderer) {
                m_renderer->toggleCropping();
                g_emulation->getConfig()->updateConfig();
            }
            break;
        case SR_COLOR:
            if (m_renderer) {
                m_renderer->toggleColorMode();
                g_emulation->getConfig()->updateConfig();
            }
            break;
        case SR_COPYTXT:
            if (m_renderer) {
                const char* text = m_renderer->getTextScreen();
                if (text)
                    palCopyTextToClipboard(text);
            }
            break;
        case SR_PASTE:
            if (m_kbdTapper && m_kbdLayout->getMode() == KbdLayout::KLM_SMART) {
                m_kbdTapper->typeText(palGetTextFromClipboard());
            }
            break;
        case SR_DISKA:
            // open disk A image
            if (m_diskA)
                m_diskA->chooseFile();
            break;
        case SR_DISKB:
            // open disk B image
            if (m_diskB)
                m_diskB->chooseFile();
            break;
        case SR_DISKC:
            // open floppy disk C image
            if (m_diskC)
                m_diskC->chooseFile();
            break;
        case SR_DISKD:
            // open floppy disk D image
            if (m_diskD)
                m_diskD->chooseFile();
            break;
        case SR_HDD:
            // open HDD/CF image
            if (m_hdd)
                m_hdd->chooseFile();
        break;
        case SR_LOAD:
            if (m_loader) {
                m_loader->chooseAndLoadFile();
            }
            break;
        case SR_LOADRUN:
            if (m_loader) {
                m_loader->chooseAndLoadFile(true);
            }
            break;
        case SR_DEBUG:
            // show debugger
            g_emulation->debugRequest(m_cpu);
            //showDebugger();
            break;
        case SR_LOADRAMDISK:
            if (m_ramDisk)
                m_ramDisk->loadFromFile();
            break;
        case SR_SAVERAMDISK:
            if (m_ramDisk)
                m_ramDisk->saveToFile();
            break;
        case SR_OPENRAMDISK:
            if (m_ramDisk)
                m_ramDisk->openFile();
            break;
        case SR_SAVERAMDISKAS:
            if (m_ramDisk)
                m_ramDisk->saveFileAs();
            break;
        case SR_LOADRAMDISK2:
            if (m_ramDisk2)
                m_ramDisk2->loadFromFile();
            break;
        case SR_SAVERAMDISK2:
            if (m_ramDisk2)
                m_ramDisk2->saveToFile();
            break;
        case SR_OPENRAMDISK2:
            if (m_ramDisk2)
                m_ramDisk2->openFile();
            break;
        case SR_SAVERAMDISK2AS:
            if (m_ramDisk2)
                m_ramDisk2->saveFileAs();
            break;
        case SR_FASTRESET:
            if (m_fastResetCpuTicks) {
                m_fastReset = !m_fastReset;
                g_emulation->getConfig()->updateConfig();
            }
            break;
        case SR_TAPEHOOK:
            if (m_tapeGrp) {
                //EmuValuesList param;
                string val = m_tapeGrp->getPropertyStringValue("enabled");
                if (val == "yes")
                    val = "no";
                else if (val == "no")
                    val = "yes";
                else
                    break;

                m_tapeGrp->setProperty("enabled", val);
                g_emulation->getConfig()->updateConfig();
            }
        default:
            break;
    }
}


void Platform::processKey(PalKeyCode keyCode, bool isPressed, unsigned unicodeKey)
{
    if (m_kbdLayout)
        m_kbdLayout->processKey(keyCode, isPressed, unicodeKey);
}


void Platform::resetKeys()
{
    if (m_kbdLayout)
        m_kbdLayout->resetKeys();
}


void Platform::mouseDrag(int x, int y)
{
    if (m_renderer)
        m_renderer->mouseDrag(x, y);
}


bool Platform::loadFile(string fileName, bool run)
{
    if (m_loader) {
        m_loader->loadFile(fileName, run);
        updateDebugger();
        return true;
    }
    return false;
}


void Platform::draw()
{
    if (m_core)
        m_core->draw();

    if (m_dbgWindow)
        m_dbgWindow->draw();
}


void Platform::showDebugger()
{
    if (m_cpu->getType() == Cpu::CPU_8080 || m_cpu->getType() == Cpu::CPU_Z80) {
        if (!m_dbgWindow) {
            m_dbgWindow = new DebugWindow(this);
            m_dbgWindow->initDbgWindow();
            m_dbgWindow->setCaption("Debug: " + m_window->getCaption());
        }
        m_dbgWindow->startDebug();
    }
}


void Platform::updateDebugger()
{
    if (m_dbgWindow)
        m_dbgWindow->update();
}


void Platform::reqScreenUpdateForDebug()
{
    if (m_renderer)
        m_renderer->prepareDebugScreen();
    if (m_renderer2)
        m_renderer2->prepareDebugScreen();
}


bool Platform::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "helpFile") {
        m_helpFile = values[0].asString();
        return true;
    } else if (propertyName == "codePage") {
        if (values[0].asString() == "rk") {
            m_codePage = CP_RK;
            return true;
        } else if (values[0].asString() == "koi8") {
            m_codePage = CP_KOI8;
            return true;
        }
    } else if (propertyName == "muteTape") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_muteTape = values[0].asString() == "yes";
            return true;
        }
    } else if (propertyName == "fastReset") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_fastReset = values[0].asString() == "yes";
            return true;
        }
    } else if (propertyName == "fastResetCpuTicks") {
            m_fastResetCpuTicks = values[0].asInt();
            return true;
    }
    return false;
}


string Platform::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "helpFile")
        return m_helpFile;
    else if (propertyName == "codePage")
        return m_codePage == CP_RK ? "rk" : "koi8";
    else if (propertyName == "muteTape")
        return m_muteTape ? "yes" : "no";
    else if (propertyName == "fastReset")
        return m_fastResetCpuTicks ? m_fastReset ? "yes" : "no" : "";

    return "";
}


string Platform::getAllDebugInfo()
{
    string res = "";
    for (auto it = m_objList.begin(); it != m_objList.end(); it++) {
        string s = (*it)->getDebugInfo();
        if (s != "") {
            if (res != "")
                res += "\n\n";
            res = res + s;
        }
    }
    return res;
}


void Platform::updateScreenOnce()
{
    if (m_renderer)
        m_renderer->updateScreenOnce();
//    if (m_renderer2)
//        m_renderer2->updateScreenOnce();
    updateDebugger();
}
