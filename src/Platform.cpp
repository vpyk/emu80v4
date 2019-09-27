/*
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

#include <sstream>

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
#include "FdImage.h"
#include "FileLoader.h"
#include "Keyboard.h"
#include "RamDisk.h"
#include "Debugger.h"

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

    // ищем объект - рендерер, только первый
    for (auto it = m_objList.begin(); it != m_objList.end(); it++)
        if ((m_renderer = dynamic_cast<CrtRenderer*>(*it)))
            break;

    // ищем объекты - образы дисков A и B
    for (auto it = m_objList.begin(); it != m_objList.end(); it++) {
        FdImage* img = dynamic_cast<FdImage*>(*it);
        if (img) {
            if (img->getLabel() == "A")
                m_diskA = img;
            else if (img->getLabel() == "B")
                m_diskB = img;
        }

    // ищем объект - загрузчик, должен быть единственным
    for (auto it = m_objList.begin(); it != m_objList.end(); it++)
        if ((m_loader = dynamic_cast<FileLoader*>(*it)))
            break;
    }

    // ищем объект - RAM-диск, должен быть единственным
    for (auto it = m_objList.begin(); it != m_objList.end(); it++)
        if ((m_ramDisk = dynamic_cast<RamDisk*>(*it)))
            break;

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

    init();

    if (m_window)
        m_window->show();

    reset();
}


void Platform::init()
{
    for (auto it = m_objList.begin(); it != m_objList.end(); it++)
        (*it)->init();
}


void Platform::reset()
{
    for (auto it = m_objList.begin(); it != m_objList.end(); it++)
        (*it)->reset();
}


Platform::~Platform()
{
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


void Platform::loadFile(string fileName)
{
    if (m_loader)
        m_loader->loadFile(fileName, true);  // добавить параметр из конфигурации вместо true !!!
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
