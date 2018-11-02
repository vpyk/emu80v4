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

//#include <iostream>
#include <fstream>
#include <sstream>

/*#include <algorithm>
#include <cctype>
#include <locale>*/

#include <string.h>

#include "Pal.h"

#include "EmuObjects.h"
#include "Emulation.h"
#include "EmuWindow.h"
#include "ConfigReader.h"
#include "Cpu8080.h"
#include "CpuZ80.h"
#include "Platform.h"
#include "KbdLayout.h"
#include "KbdLayout.h"
#include "RkKeyboard.h"
#include "Ppi8255Circuit.h"
#include "RkPpi8255Circuit.h"
#include "Pit8253.h"
#include "Pit8253Sound.h"
#include "Ppi8255.h"
#include "Dma8257.h"
#include "Crt8275.h"
#include "Rk86.h"
#include "Mikrosha.h"
#include "Apogey.h"
#include "Partner.h"
#include "Orion.h"
#include "Specialist.h"
#include "Mikro80.h"
#include "Ut88.h"
#include "Eureka.h"
#include "Globals.h"
#include "SoundMixer.h"
#include "RkFdd.h"
#include "FileLoader.h"
#include "TapeRedirector.h"
#include "WavReader.h"
#include "RkTapeHooks.h"
#include "MsxTapeHooks.h"
#include "CloseFileHook.h"
#include "RkRomDisk.h"
#include "FdImage.h"
#include "Fdc1793.h"
#include "RamDisk.h"
#include "EmuConfig.h"
#include "GenericModules.h"
#include "RkSdController.h"


using namespace std;

ConfigReader::ConfigReader(string configFileName, string platformName)
{
    if (platformName == "")
        m_prefix = platformName;
    else
        m_prefix = platformName + ".";

    m_configFileName = configFileName;

    list<string> palDefines;
    palGetPalDefines(palDefines);
    for (auto it = palDefines.begin(); it != palDefines.end(); it++)
        m_varMap[*it] = "";

    openFile();
}


void ConfigReader::openFile()
{
    int fileSize;
    uint8_t* buf = palReadFile(m_configFileName, fileSize);
    if (!buf) {
        //logPrefix();
        //emuLog << "warning: can't open include file" << "\n";
        m_curLine = 0;
        m_inputStream = new istringstream("");
        return;
    }
    uint8_t* dataPtr = buf;
    if (fileSize >= 3 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF) {
        // UTF-8 BOM
        dataPtr +=3;
        fileSize -= 3;
    }
    string s((const char*)dataPtr, fileSize);
    if(buf)
        delete[] buf;
    m_curLine = 0;
    m_inputStream = new istringstream(s);
}


ConfigReader::~ConfigReader()
{
    if (m_inputStream)
        delete m_inputStream;
}


/*
// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}*/


static inline bool isspc(char c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

static void trim(string& s)
{
    while (s != "" && isspc(s[0]))
        s.erase(0, 1);
    while (s != "" && isspc(s[s.size() - 1]))
        s.erase(s.size() - 1, 1);
}

static const char* DELIM_DOTSPACE = " \t.=";
static const char* DELIM_COMMA = ",\"";
static const char* DELIM_QUOT = "\"";

static string getToken(string &s, string delimeters)
{
    trim(s);

    if (s == "" || s[0] == '#') {
        s = "";
        return "";
    }

/*    if (s == "" || (s[0] == '#' && s.substr(0, 8) != "#include")) {
        s = "";
        return "";
    }*/

    string res;

    string::size_type pos = s.find_first_of(delimeters);

    if (pos == string::npos) {
        // остаток строки
        res = s;
        s = "";
        return res;
    }

    if (pos == 0) {
        // одиночный символ
        res = s.substr(0, 1);
        s.erase(0, 1);
        trim(s);
        return res;
    }

    // слово
    res = s.substr(0, pos);
    s.erase(0, pos);
    trim(s);
    return res;
}


void ConfigReader::fillValuesList(string s, EmuValuesList* values)
{
    string token;
    while (s != "") {
        // подставляем переменные
        while (s[0] == '@') {
            token = getToken(s, DELIM_COMMA);

            string var = token.substr(1);
            auto it = m_varMap.find(var);
            if (it != m_varMap.end())
                s = it->second + s;
            else {
                s = "";
                logPrefix();
                emuLog << "variable @" << var << " not found" << "\n";
            }
        }

        token = getToken(s, DELIM_COMMA);

        if (token == "\"") {
            token = getToken(s, DELIM_QUOT);
            getToken(s, DELIM_QUOT);
        }
        if (token != "") {
            if (token.substr(0, 1) == "&")
                token = m_prefix + token.substr(1);
            values->addValue(token);
        }

        token = getToken(s, DELIM_COMMA);
    }
}


void ConfigReader::stop()
{
    delete m_inputStream;

    if (m_stateStack.empty())
        m_inputStream = nullptr;
    else {
        ConfigReaderState crs;
        crs = m_stateStack.top();
        m_stateStack.pop();
        m_configFileName = crs.configFileName;
        m_curLine = crs.curLine;
        m_inputStream = crs.inputStream;
    }
}


bool ConfigReader::getNextLine(string& typeName, string& objName, string& propName, EmuValuesList* values)
{
    if (!m_inputStream)
        return false;

    string s, token;
    bool res = true;

    while(true) {

        // читаем строку
        if (!getline(*m_inputStream, s)) {
            stop();
            if (!m_inputStream) {
                res = false;
                break;
            }
            continue;
        }

        m_curLine++;

        token = getToken(s, DELIM_DOTSPACE);

        // пропускаем пустые строки или строки с комменатриями
        if (token == "")
            continue;

        string first, second;
        first = token;

        if (first == "ifdef" || first == "ifndef") {
            bool cond = (first == "ifdef");
            EmuValuesList defines;
            fillValuesList(s, &defines);
            if (defines.size() != 1) {
                logPrefix();
                emuLog << "error: invalid if(n)def syntax" << "\n";
                stop();
                return false;
            }
            string var = defines[0].asString();
            auto it = m_varMap.find(var);
            if (it != m_varMap.end())
                m_ifCondition = cond;
                //m_condStack.push(cond);
            else
                m_ifCondition = !cond;
                //m_condStack.push(!cond);
            continue;
        }

        if (first == "else") {
            // else implementation
            /*if (m_condStack.empty()) {
                logPrefix();
                emuLog << "error: else without if(n)def" << "\n";
                stop();
            }*/

            //bool cond = m_condStack.top();
            //m_condStack.pop();
            //m_condStack.push(!cond);
            m_ifCondition = !m_ifCondition;
            continue;
        }

        if (first == "endif") {
            // else implementation
            /*if (m_condStack.empty()) {
                logPrefix();
                emuLog << "error: endif without if(n)def" << "\n";
                stop();
            }*/

            m_ifCondition = true;
            //m_condStack.pop();
            continue;
        }

        if (first == "define") {
            EmuValuesList defines;
            fillValuesList(s, &defines);
            if (defines.size() != 1) {
                logPrefix();
                emuLog << "error: invalid define syntax" << "\n";
                stop();
                return false;
            }
            string var = defines[0].asString();
            m_varMap[var] = "";
            continue;
        }

        //if (!m_condStack.empty() && !m_condStack.top())
        if (!m_ifCondition)
            continue; // пропускаем из if...

        if (first == "include") {
            EmuValuesList includeFiles;
            fillValuesList(s, &includeFiles);
            if (includeFiles.size() == 0) {
                logPrefix();
                emuLog << "warning: invalid include syntax, ignoring" << "\n";
                continue;
            }

            ConfigReaderState crs;
            crs.configFileName = m_configFileName;
            crs.curLine = m_curLine;
            crs.inputStream = m_inputStream;
            m_stateStack.push(crs);

            m_configFileName = includeFiles[0].asString(); // пока включаем только один файл
            openFile();
            continue;
        }

        token = getToken(s, DELIM_DOTSPACE);

        if (first[0] == '@' && token == "=") {
            // Присвоение переменной
            string varName = first.substr(1);
            string::size_type sharpPos = s.find("#",0);
            if (sharpPos != string::npos)
                s = s.substr(0, sharpPos);
            m_varMap[varName] = s;
            continue;
        } else if (token == ".") {
            // присвоение значения
            token = getToken(s, DELIM_DOTSPACE);
            if (token == "" || token == "=") {
                logPrefix();
                emuLog << "property expected" << "\n";
                continue;
            }
            second = token;

            token = getToken(s, DELIM_DOTSPACE);
            if (token != "=") {
                logPrefix();
                emuLog << "\"=\" expected" << "\n";
                continue;
            }

            typeName = "";
            objName = first;
            propName = second;

            fillValuesList(s, values);
        } else {
            if (token == "" || token == "=") {
                logPrefix();
                emuLog << "object name expected" << "\n";
                stop();
                return false;
            }

            second = token;

            // создание объекта
            typeName = first;
            objName = second;
            propName = "";

            token = getToken(s, DELIM_DOTSPACE);
            if (token != "=" && token != "") {
                logPrefix();
                emuLog << "EOL or \"=\" expected" << "\n";
                stop();
                return false;
            }
            fillValuesList(s, values);
        }
        break;
    } // while (true)
    return res;
}


EmuObject* ConfigReader::createObject(string typeName, string objName, const EmuValuesList& parameters)
{
    EmuObject* obj = nullptr;

    if (typeName == "EmuWindow")
        obj = new EmuWindow();
    else if (typeName == "EmuObjectGroup")
        obj = new EmuObjectGroup();
    else if (typeName == "Platform")
        obj = new Platform(parameters[0].asString(), objName);
    else if (typeName == "AddrSpace")
        obj = new AddrSpace();
    else if (typeName == "AddrSpaceMapper") {
        if (parameters[0].isInt())
            obj = new AddrSpaceMapper(parameters[0].asInt());
    } else if (typeName == "AddrSpaceShifter") {
        if (parameters[1].isInt())
            obj = new AddrSpaceShifter(static_cast<AddressableDevice*>(g_emulation->findObject(parameters[0].asString())), parameters[1].asInt());
    } else if (typeName == "AddrSpaceInverter") {
        obj = new AddrSpaceInverter(static_cast<AddressableDevice*>(g_emulation->findObject(parameters[0].asString())));
    } else if (typeName == "Ram") {
        if (parameters[0].isInt()) {
            obj = new Ram(parameters[0].asInt());
        }
    } else if (typeName == "Rom") {
        if (parameters[1].isInt())
            obj = new Rom(parameters[1].asInt(), parameters[0].asString());
    } else if (typeName == "NullSpace") {
        if (parameters[0].isInt())
            obj = new NullSpace(parameters[0].asInt());
    } else if (typeName == "Cpu8080")
        obj = new Cpu8080();
    else if (typeName == "Cpu8080StatusWordSpace")
        obj = new Cpu8080StatusWordSpace(static_cast<Cpu8080*>(g_emulation->findObject(parameters[0].asString())));
    else if (typeName == "CpuZ80")
        obj = new CpuZ80();
    else if (typeName == "Ppi8255")
        obj = new Ppi8255();
    else if (typeName == "Dma8257")
        obj = new Dma8257();
    else if (typeName == "Crt8275")
        obj = new Crt8275();
    else if (typeName == "Pit8253")
        obj = new Pit8253();
    else if (typeName == "Fdc1793")
        obj = new Fdc1793();
    else if (typeName == "Pit8253SoundSource")
        obj = new Pit8253SoundSource();
    else if (typeName == "RkPit8253SoundSource")
        obj = new RkPit8253SoundSource();
    /*else if (typeName == "MikroshaPit8253SoundSource")
        obj = new MikroshaPit8253SoundSource();*/
    else if (typeName == "OrionRenderer")
        obj = new OrionRenderer();
    else if (typeName == "SpecRenderer")
        obj = new SpecRenderer();
    else if (typeName == "EurekaRenderer")
        obj = new EurekaRenderer();
    else if (typeName == "Mikro80Renderer")
        obj = new Mikro80Renderer();
    else if (typeName == "Ut88Renderer")
        obj = new Ut88Renderer();
    else if (typeName == "RkKeyboard")
        obj = new RkKeyboard();
    else if (typeName == "SpecKeyboard")
        obj = new SpecKeyboard();
    else if (typeName == "RkKeybLayout")
        obj = new RkKbdLayout();
    else if (typeName == "RkPpi8255Circuit")
        obj = new RkPpi8255Circuit();
    else if (typeName == "MikroshaCore")
        obj = new MikroshaCore();
    else if (typeName == "MikroshaRenderer")
        obj = new MikroshaRenderer();
    else if (typeName == "MikroshaPpi8255Circuit")
        obj = new MikroshaPpi8255Circuit();
    else if (typeName == "MikroshaPpi2Circuit")
        obj = new MikroshaPpi2Circuit();
    else if (typeName == "SpecPpi8255Circuit")
        obj = new SpecPpi8255Circuit();
    else if (typeName == "PartnerPpi8255Circuit")
        obj = new PartnerPpi8255Circuit();
    else if (typeName == "ApogeyCore")
        obj = new ApogeyCore();
    else if (typeName == "ApogeyRenderer")
        obj = new ApogeyRenderer();
    else if (typeName == "Rk86Core")
        obj = new Rk86Core();
    else if (typeName == "Rk86Renderer")
        obj = new Rk86Renderer();
    else if (typeName == "PartnerCore")
        obj = new PartnerCore();
    else if (typeName == "PartnerRenderer")
        obj = new PartnerRenderer();
    else if (typeName == "PartnerMcpgRenderer")
        obj = new PartnerMcpgRenderer();
    else if (typeName == "OrionCore")
        obj = new OrionCore();
    else if (typeName == "SpecCore")
        obj = new SpecCore();
    else if (typeName == "EurekaCore")
        obj = new EurekaCore();
    else if (typeName == "Mikro80Core")
        obj = new Mikro80Core();
    else if (typeName == "Ut88Core")
        obj = new Ut88Core();
    else if (typeName == "Mikro80TapeRegister")
        obj = new Mikro80TapeRegister();
    else if (typeName == "RkFddRegister")
        obj = new RkFddRegister();
    else if (typeName == "RkFddController")
        obj = new RkFddController();
    else if (typeName == "PartnerRamUpdater")
        obj = new PartnerRamUpdater();
    else if (typeName == "PartnerMcpgSelector")
        obj = new PartnerMcpgSelector();
    else if (typeName == "PartnerModuleSelector")
        obj = new PartnerModuleSelector();
    else if (typeName == "PartnerAddrSpace")
        obj = new PartnerAddrSpace(parameters[0].asString());
    else if (typeName == "PartnerAddrSpaceSelector")
        obj = new PartnerAddrSpaceSelector();
    else if (typeName == "PartnerFddControlRegister")
        obj = new PartnerFddControlRegister();
    else if (typeName == "OrionMemPageSelector")
        obj = new OrionMemPageSelector();
    else if (typeName == "OrionScreenSelector")
        obj = new OrionScreenSelector();
    else if (typeName == "OrionColorModeSelector")
        obj = new OrionColorModeSelector();
    else if (typeName == "OrionFddControlRegister")
        obj = new OrionFddControlRegister();
    else if (typeName == "OrionFddQueryRegister")
        obj = new OrionFddQueryRegister();
    else if (typeName == "SpecMxMemPageSelector")
        obj = new SpecMxMemPageSelector();
    else if (typeName == "SpecMxFddControlRegisters")
        obj = new SpecMxFddControlRegisters();
    else if (typeName == "SpecVideoRam")
        obj = new SpecVideoRam(parameters[0].asInt());
    else if (typeName == "SpecMxColorRegister")
        obj = new SpecMxColorRegister();
    else if (typeName == "RkFileLoader")
        obj = new RkFileLoader();
    else if (typeName == "SpecFileLoader")
        obj = new SpecFileLoader();
    else if (typeName == "SpecMxFileLoader")
        obj = new SpecMxFileLoader();
    else if (typeName == "OrionFileLoader")
        obj = new OrionFileLoader();
    else if (typeName == "TapeRedirector")
        obj = new TapeRedirector();
    else if (typeName == "FdImage")
        obj = new FdImage(parameters[0].asInt(), parameters[1].asInt(), parameters[2].asInt(), parameters[3].asInt());
    else if (typeName == "RkTapeOutHook")
        obj = new RkTapeOutHook(parameters[0].asInt());
    else if (typeName == "RkTapeInHook")
        obj = new RkTapeInHook(parameters[0].asInt());
    else if (typeName == "MsxTapeOutHook")
        obj = new MsxTapeOutHook(parameters[0].asInt());
    else if (typeName == "MsxTapeInHook")
        obj = new MsxTapeInHook(parameters[0].asInt());
    else if (typeName == "MsxTapeOutHeaderHook")
        obj = new MsxTapeOutHeaderHook(parameters[0].asInt());
    else if (typeName == "MsxTapeInHeaderHook")
        obj = new MsxTapeInHeaderHook(parameters[0].asInt());
    else if (typeName == "CloseFileHook")
        obj = new CloseFileHook(parameters[0].asInt());
    else if (typeName == "RkRomDisk")
        obj = new RkRomDisk(parameters[0].asString());
    else if (typeName == "SpecRomDisk")
        obj = new SpecRomDisk(parameters[0].asString());
    else if (typeName == "RkSdController")
        obj = new RkSdController(parameters[0].asString());
    else if (typeName == "ApogeyRomDisk")
        obj = new ApogeyRomDisk(parameters[0].asString());
    else if (typeName == "Ut88MemPageSelector")
        obj = new Ut88MemPageSelector();
    else if (typeName == "Ut88AddrSpaceMapper")
        obj = new Ut88AddrSpaceMapper();
    else if (typeName == "EurekaPpi8255Circuit")
        obj = new EurekaPpi8255Circuit(parameters[0].asString());
    else if (typeName == "RamDisk")
        obj = new RamDisk(parameters[0].asInt(), parameters[1].asInt());
    else if (typeName == "PeriodicInt8080")
        obj = new PeriodicInt8080(static_cast<Cpu8080Compatible*>(g_emulation->findObject(parameters[0].asString())), parameters[1].asInt(), parameters[2].asInt());
    else if (typeName == "PageSelector")
        obj = new PageSelector();
    else if (typeName == "Splitter")
        obj = new Splitter();
    else if (typeName == "Translator")
        obj = new Translator(static_cast<AddressableDevice*>(g_emulation->findObject(parameters[0].asString())));
    else if (typeName == "ConfigTab")
        obj = new EmuConfigTab(parameters[0].asString());
    else if (typeName == "ConfigRadioSelector")
        obj = new EmuConfigRadioSelector(parameters[0].asString(), parameters[1].asString(), parameters[2].asString());

    if (obj) {
        obj->setName(objName);
    }

    return obj;
}


void ConfigReader::logPrefix()
{
    emuLog << "File " << m_configFileName << ", line " << m_curLine << " : ";
}

void ConfigReader::processConfigFile(ParentObject* parent)
{
    string t,o,p;
    EmuValuesList v;
    bool res = getNextLine(t,o,p,&v);
    while (res) {
        //cout << t << " " << o << " " << p << endl;
        if (t != "" && o != "" && p == "") {
            if (g_emulation->findObject(m_prefix + o)) {
                logPrefix();
                emuLog << "Object " << o << " already exists!" << "\n";
            } else if (EmuObject* obj = createObject(t, m_prefix + o, v)) {
                obj->setPlatform(dynamic_cast<Platform*>(parent));
                parent->addChild(obj);
            } else {
                logPrefix();
                emuLog << "Can't create object " << t << " " << o << "\n";
                //break;
            }
        }
        else if (t == "" && o != "" && p != "") {
            EmuObject* obj = nullptr;
            if (m_prefix != "" && o == "platform") // подставляем вместо "platform" конкретное име текущей платформы
                o = m_prefix.substr(0, m_prefix.size() - 1); // убираем "."
            else
                o = m_prefix + o;
            obj = g_emulation->findObject(o);
            if (!obj)
                obj = g_emulation->findObject(o);
            if (!obj) {
                logPrefix();
                emuLog << "Object " << o << " not found" << "\n";
            } else if (!obj->setProperty(p, v)) {
                logPrefix();
                emuLog << "Set property " << o << "." << p <<  " failed\n";
            }
        }
        v.clearList();
        res = getNextLine(t,o,p,&v);
    }

}
