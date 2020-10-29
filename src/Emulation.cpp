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
#include <algorithm>

#include "Pal.h"

#include "Globals.h"
#include "EmuObjects.h"
#include "Emulation.h"
#include "ConfigReader.h"
#include "Platform.h"
#include "EmuWindow.h"
#include "EmuConfig.h"
#include "SoundMixer.h"
#include "WavReader.h"
#include "FileLoader.h"

using namespace std;


Emulation::Emulation(int argc, char** argv)
{
    m_argc = argc;
    m_argv = argv;

    m_frameRate = 100;
    m_vsync = true;
    m_sampleRate = 48000;

    g_emulation = this;
    setName("emulation");

    addObject(this);

    m_config = new EmuConfig();
    m_config->setName("config");

    m_mixer = new SoundMixer;
    m_mixer->setName("soundMixer");

    m_wavReader = new WavReader;
    m_wavReader->setName("wavReader");

    ConfigReader cr("emu80.conf");
    cr.processConfigFile(this);
    getConfig()->updateConfig();

    if (m_platformList.empty()) {
        string defPlatformName = palGetDefaultPlatform();
        if (defPlatformName != "")
            runPlatform(defPlatformName);
        else {
            PlatformInfo pi;
            bool newWnd;
            if (!m_config->getPlatformInfos()->empty() && m_config->choosePlatform(pi, "", newWnd, true)) {
                Platform* platform = new Platform(pi.configFileName, pi.objName);
                m_platformList.push_back(platform);
                getConfig()->updateConfig();
                //m_activePlatform = platform;
            } else
                palRequestForQuit();
        }
    }

    checkPlatforms();
}

#include <typeinfo>

Emulation::~Emulation()
{
    // Удяляем платформы с дочерними объектами
    for (auto it = m_platformList.begin(); it != m_platformList.end(); it++)
        delete (*it);

    delete m_config;
    delete m_wavReader; // перед m_mixer!
    delete m_mixer;

    // Удяляем оставшиеся объекты
    list<EmuObject*> tempList = m_objectList; // второй список, так как в деструкторе удаление из основного списка
    for (auto it = tempList.begin(); it != tempList.end(); it++)
        if ((*it) != this)
            delete (*it);
}


void Emulation::checkPlatforms()
{
    // Delete platforms without windows (missing conf files)
    // and request for quit if no platform left
    for (auto it = m_platformList.begin(); it != m_platformList.end();)
        if (!(*it)->getWindow())
            m_platformList.erase(it++);
        else
            it++;

    if (m_platformList.empty())
        palRequestForQuit();
}


void Emulation::processCmdLine()
{
    // Command line options
    bool loadOnly = false;
    for (int i = 1; i < m_argc; i++) {
        if (string(m_argv[i]) == "-l")
            loadOnly = true;
    }

    // Command line file name
    string cmdLineFileName = "";
    for (int i = 1; i < m_argc; i++)
        if (m_argv[i][0] != '-') {
            cmdLineFileName = m_argv[i];
            break;
        }

    // Command line platform options
    string platformName = "";
    const std::vector<PlatformInfo>* platforms = m_config->getPlatformInfos();
    for (auto it = platforms->begin(); it != platforms->end(); it++) {
        for (int i = 1; i < m_argc; i++) {
            if (m_argv[i] == '-' + it->cmdLineOption) {
                platformName = it->objName;
                break;
            }
            if (platformName != "")
                break;
        }
    }

    // Extention based platform
    string cmdLineFileExt = "";
    if (platformName == "") {
        string::size_type dotPos = cmdLineFileName.find_last_of(".");
        if (dotPos != string::npos) {
            cmdLineFileExt = cmdLineFileName.substr(dotPos + 1);
            for (unsigned i = 0; i < cmdLineFileExt.size(); i++)
                cmdLineFileExt[i] = tolower(cmdLineFileExt[i]);
        }
    }
    if (cmdLineFileExt != "") {
        std::map<std::string, std::string>* extentionMap = m_config->getExtentionMap();
        auto it = extentionMap->find(cmdLineFileExt);
        if (it != extentionMap->end())
            platformName = it->second;
    }

    if (platformName != "") {
        runPlatform(platformName);
        m_platformCreatedFromCmdLine = true;
    }

    if (cmdLineFileName != "" && !m_platformList.empty()) {
        Platform* platform = *m_platformList.begin();
        FileLoader* loader = platform->getLoader();
        if (loader)
            loader->loadFile(cmdLineFileName, !loadOnly);
    }
}


void Emulation::runPlatform(const string& platformName)
{
    const std::vector<PlatformInfo>* platformVector = m_config->getPlatformInfos();
    for (unsigned i = 0; i < platformVector->size(); i++)
        if ((*platformVector)[i].objName == platformName) {
            Platform* newPlatform = new Platform((*platformVector)[i].configFileName, platformName);
            addChild(newPlatform);
            break;
        }
}


void Emulation::newPlatform(const string& platformName)
{
    // Удаляем все платформы
    for (auto it = m_platformList.begin(); it != m_platformList.end(); it++)
        delete (*it);
    m_platformList.clear();
    runPlatform(platformName);
}

void Emulation::registerActiveDevice(IActive* device)
{
    m_activeDevVector.push_back(device);
    nDevices++;
    m_activeDevices = m_activeDevVector.data();
    inCycle = false;
}


void Emulation::unregisterActiveDevice(IActive* device)
{
    m_activeDevVector.erase(remove(m_activeDevVector.begin(), m_activeDevVector.end(), device));
    nDevices--;
    m_activeDevices = m_activeDevVector.data();
    inCycle = false;
}


void Emulation::addObject(EmuObject* obj)
{
    m_objectList.push_back(obj);
}


void Emulation::removeObject(EmuObject* obj)
{
    m_objectList.remove(obj);
}


EmuObject* Emulation::findObject(string name)
{
    for (auto it = m_objectList.begin(); it != m_objectList.end(); it++)
        if ((*it)->getName() == name)
            return *it;
    return nullptr;
}


void Emulation::addChild(EmuObject* child)
{
    if (Platform* pl = dynamic_cast<Platform*>(child))
        m_platformList.push_back(pl);
};


void Emulation::exec(uint64_t ticks)
{
    if (m_isPaused)
        return;

    uint64_t toTime = m_curClock + ticks - m_clockOffset;

    while (m_curClock < toTime && !m_debugReqCpu) {
        uint64_t time = -1;
        IActive* curDev = nullptr;

        inCycle = true;
        IActive* device;
        uint64_t tm;
        for (int i = 0; i < nDevices && inCycle; i++) {
            device = m_activeDevices[i];
            if (!device->isPaused()) {
                tm = device->getClock();
                if (tm < time) {
                    time = tm;
                    curDev = device;
                }
            }
        }

        m_curClock = time;
        curDev->operate();
    }
    m_clockOffset = m_curClock - toTime;

    if (m_debugReqCpu) {
        m_clockOffset = 0;
        // show debugger
        for (auto it = m_platformList.begin(); it != m_platformList.end(); it++)
        if ((*it)->getCpu() == m_debugReqCpu) {
            (*it)->showDebugger();
            break;
        }
    }

}


void Emulation::draw()
{
    for (auto it = m_platformList.begin(); it != m_platformList.end(); it++) {
        (*it)->draw();
    }
}


void Emulation::processKey(EmuWindow* wnd, PalKeyCode keyCode, bool isPressed, unsigned unicodeKey)
{
    // нужно отправлять клавишу только активной платформе
    Platform* platform = platformByWindow(wnd);
    if (platform)
        platform->processKey(keyCode, isPressed, unicodeKey);
    else if (keyCode != PK_NONE)
        wnd->processKey(keyCode, isPressed);
}


void Emulation::resetKeys(EmuWindow* wnd)
{
    Platform* platform = platformByWindow(wnd);
    if (platform)
        platform->resetKeys();
}


void Emulation::sysReq(EmuWindow* wnd, SysReq sr)
{
    Platform* platform = platformByWindow(wnd);

    // enable debug if in paused state
    if (sr == SR_DEBUG)
        m_isPaused = false;

    switch(sr) {
        case SR_EXIT:
            //for (auto it = m_platformList.begin(); it != m_platformList.end(); it++)
                //delete (*it);
            //m_platformList.clear(); // лишнее
            palRequestForQuit();
            break;
        case SR_CLOSE:
            if (platform) {
                m_platformList.remove(platform);
                delete platform;
                m_lastActivePlatform = nullptr;
                if (m_platformList.empty())
                    palRequestForQuit();
            } else
                wnd->closeRequest();
            break;
        case SR_CONFIG: {
                int tab = TABID_NONE;
                if (platform)
                    tab = platform->getDefConfigTabId();
                m_config->showConfigWindow(tab);
                break;
            }
        case SR_HELP:
            m_config->showConfigWindow(TABID_HELP);
            break;
        case SR_CHPLATFORM:
            {
                PlatformInfo pi;
                string curPlatformName = "";
                if (!platform)
                    platform = m_lastActivePlatform;
                if (platform)
                    curPlatformName = platform->getBaseName();
                bool newWnd;
                if (m_config->choosePlatform(pi, curPlatformName, newWnd, false, wnd)) {
                    // Удяляем активную платформу (как опция можно все - закомментировано)
                    if (!newWnd) {
                        m_platformList.remove(platform);
                        delete platform;
                        //for (auto it = m_platformList.begin(); it != m_platformList.end(); it++)
                            //delete (*it);
                        //m_platformList.clear();
                    }
                    Platform* newPlatform = new Platform(pi.configFileName, pi.objName);
                    m_platformList.push_back(newPlatform);
                    //m_activePlatform = platform;
                    checkPlatforms();
                }
            }
            break;
        case SR_CHCONFIG:
            {
                if (!platform)
                break;

                string curPlatformName = platform->getBaseName();
                if (palChooseConfiguration(curPlatformName, wnd)) {
                    m_platformList.remove(platform);
                    delete platform;

                    const std::vector<PlatformInfo>* platformVector = m_config->getPlatformInfos();
                    for (unsigned i = 0; i < platformVector->size(); i++)
                        if ((*platformVector)[i].objName == curPlatformName) {
                            Platform* newPlatform = new Platform((*platformVector)[i].configFileName, curPlatformName);
                            m_platformList.push_back(newPlatform);
                            checkPlatforms();
                            break;
                        }
                }
            }
            break;
        case SR_PAUSEON:
            m_isPaused = true;
            break;
        case SR_PAUSEOFF:
            m_isPaused = false;
            break;
        case SR_PAUSE:
            m_isPaused = !m_isPaused;
            break;
        case SR_SPEEDUP:
            setSpeedUpFactor(4);
            break;
        case SR_SPEEDNORMAL:
            setSpeedUpFactor(1);
            break;
        case SR_LOADWAV:
            if (m_wavReader) {
                m_wavReader->chooseAndLoadFile();
            }
            break;
        case SR_MUTE:
            m_mixer->toggleMute();
            break;
        default:
            if (wnd)
                wnd->sysReq(sr);
            if (platform)
                platform->sysReq(sr);
    }
}


void Emulation::mainLoopCycle()
{
    if (m_prevSysClock == 0) // first run
        m_prevSysClock = palGetCounter() - palGetCounterFreq() / 60;

    draw();
    if (m_frameRate > 0) {
        int64_t delay = palGetCounterFreq() / m_frameRate - (palGetCounter() - m_prevSysClock);
        if (delay > 0)
            palDelay(delay);
    }

    m_sysClock = palGetCounter();
    unsigned dt = m_sysClock - m_prevSysClock;
    if (dt > palGetCounterFreq() / 10) // 0.1 s
        dt = palGetCounterFreq() / 10;
    uint64_t ticks = m_frequency * m_speedUpFactor * dt / palGetCounterFreq();
    m_prevSysClock = m_sysClock;
    exec(ticks);
}


void Emulation::setFrequency(int64_t freq)
{
    m_frequency = freq;
    m_mixer->setFrequency(freq);
}


// установка частоты кадров, 0 - max
void Emulation::setFrameRate(int frameRate)
{
    m_frameRate = frameRate;
    // для изменения "на лету" добавить перебор всех окон и пересоздание рендерера при необходимости
    palSetFrameRate(frameRate);
}


// установка vsync
void Emulation::setVsync(bool vsync)
{
    m_vsync = vsync;
    palSetVsync(vsync);
}


void Emulation::setSampleRate(int sampleRate)
{
    if (palSetSampleRate(sampleRate)) {
        m_sampleRate = sampleRate;
        m_mixer->setFrequency(m_frequency);
    }
}


// Установка фокуса на окно
void Emulation::setWndFocus(EmuWindow* wnd)
{
    wnd->focusChanged(true);
    if (Platform* platform = platformByWindow(wnd))
        m_lastActivePlatform = platform;
}

void Emulation::restoreFocus()
{
    if (m_lastActivePlatform)
        m_lastActivePlatform->getWindow()->bringToFront();
}


void Emulation::dropFile(EmuWindow* wnd, const string& fileName)
{
    Platform* platform = platformByWindow(wnd);
    if (platform)
        platform->loadFile(fileName);
}


void Emulation::setSpeedUpFactor(unsigned speed)
{
    m_speedUpFactor = speed;
    m_mixer->setFrequency(m_frequency * m_speedUpFactor);
}


Platform* Emulation::platformByWindow(EmuWindow* window)
{
    if (!window)
        return nullptr;

    for (auto it = m_platformList.begin(); it != m_platformList.end(); it++)
        if (window == (*it)->getWindow())
            return (*it);

    return nullptr;
}


bool Emulation::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    /*if (propertyName == "frequency" && values[0].isInt()) {
        setFrequency(values[0].asInt());
        return true;
    } else*/
    if (propertyName == "maxFps" && values[0].isInt()) {
        setFrameRate(values[0].asInt());
        return true;
    } else if (propertyName == "vsync") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            setVsync(values[0].asString() == "yes");
            return true;
        }
    } else if (propertyName == "sampleRate" && values[0].isInt()) {
        setSampleRate(values[0].asInt());
        return true;
    } else if (propertyName == "volume" && values[0].isInt()) {
        m_mixer->setVolume(values[0].asInt());
        return true;
    } else if (propertyName == "runPlatform") {
        if (!m_platformCreatedFromCmdLine) // если уже было создано окно из командной строки, больше не создаем
            runPlatform(values[0].asString());
        return true;
    } else if (propertyName == "processCmdLine") {
        processCmdLine();
        return true;
    } else if (propertyName == "debug8080MnemoUpperCase") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_debuggerOptions.mnemo8080UpperCase = values[0].asString() == "yes";
            return true;
        }
    } else if (propertyName == "debugZ80MnemoUpperCase") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_debuggerOptions.mnemoZ80UpperCase = values[0].asString() == "yes";
            return true;
        }
    } else if (propertyName == "debugForceZ80Mnemonics") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_debuggerOptions.forceZ80Mnemonics = values[0].asString() == "yes";
            return true;
        }
    } else if (propertyName == "debugSwapF5F9") {
        if (values[0].asString() == "yes" || values[0].asString() == "no") {
            m_debuggerOptions.swapF5F9 = values[0].asString() == "yes";
            return true;
        }
    }

    return false;
}


string Emulation::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "volume") {
        stringstream stringStream;
        stringStream << m_mixer->getVolume();
        stringStream >> res;
    } else if (propertyName == "debug8080MnemoUpperCase")
        res = m_debuggerOptions.mnemo8080UpperCase ? "yes" : "no";
    else if (propertyName == "debugZ80MnemoUpperCase")
        res = m_debuggerOptions.mnemoZ80UpperCase ? "yes" : "no";
    else if (propertyName == "debugForceZ80Mnemonics")
        res = m_debuggerOptions.forceZ80Mnemonics ? "yes" : "no";
    else if (propertyName == "debugSwapF5F9")
        res = m_debuggerOptions.swapF5F9 ? "yes" : "no";
    /* else if (propertyName == "frameRate") {
        stringstream stringStream;
        stringStream << m_frameRate;
        stringStream >> res;
    } else if (propertyName == "sampleRate") {
        stringstream stringStream;
        stringStream << m_sampleRate;
        stringStream >> res;
    }*/
    return res;
}
