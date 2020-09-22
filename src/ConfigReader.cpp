﻿/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2020
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

/*#include <iostream>
#include <algorithm>
#include <cctype>
#include <locale>*/

#include <fstream>
#include <sstream>

#include <string.h>

#include "Pal.h"

#include "EmuObjects.h"
#include "Emulation.h"
#include "ConfigReader.h"
#include "Platform.h"
#include "ObjectFactory.h"


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

    if (platformName != "") {
        map<string, string> platformDefines;
        palGetPlatformDefines(platformName, platformDefines);
        for (auto it = platformDefines.begin(); it != platformDefines.end(); it++)
            m_varMap[it->first] = it->second;
    }

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

static string deQuote(string s)
{
    trim(s);
    if (s.length() >= 2 && s[0] == '\"' && s[s.length() - 1] == '\"')
        return s.substr(1, s.length() - 2);
    else
        return s;
}

static const char* DELIM_DOTSPACE = " \t.=";
static const char* DELIM_COMMA = ",\"";
static const char* DELIM_QUOT = "\"";
static const char* DELIM_SPACE = " \t";

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
            if (token.substr(0, 1) == "&") {
                if (token == "&platform")
                    token = m_prefix.substr(0, m_prefix.size() - 1); // убираем "."
                else
                    token = m_prefix + token.substr(1);
            }
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
            if (!m_ifCondition) {
                m_condLevel++;
                continue;
            }
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
            m_condStack.push(m_ifCondition);
            if (it != m_varMap.end())
                m_ifCondition = cond;
            else
                m_ifCondition = !cond;
            continue;
        }

        if (first == "if") {
            if (!m_ifCondition) {
                m_condLevel++;
                continue;
            }
            second = getToken(s, DELIM_DOTSPACE);
            auto it = m_varMap.find(second);
            /*if (it == m_varMap.end()) {
                logPrefix();
                emuLog << "variable @" << second << " not found" << "\n";
                stop();
                return false;
            }
            string val = it->second;*/

            string val = it != m_varMap.end() ? it->second : "";

            token = getToken(s, DELIM_SPACE);
            if (token != "==" && token != "!=") {
                logPrefix();
                emuLog << "error: invalid if syntax" << "\n";
                stop();
                return false;
            }
            bool cond = (token == "==");

            string::size_type sharpPos = s.find("#",0);
            if (sharpPos != string::npos)
                s = s.substr(0, sharpPos);

            m_condStack.push(m_ifCondition);
            m_ifCondition = deQuote(s) == deQuote(val);
            if (!cond)
                m_ifCondition = !m_ifCondition;
            continue;
        }

        if (first == "else") {
            // else implementation
            if (m_condLevel)
                continue;

            if (m_condStack.empty()) {
                logPrefix();
                emuLog << "error: else without if(n)def" << "\n";
                stop();
            }

            m_ifCondition = !m_ifCondition;
            continue;
        }

        if (first == "endif") {
            if (m_condLevel) {
                m_condLevel--;
                continue;
            }

            if (m_condStack.empty()) {
                logPrefix();
                emuLog << "error: endif without if(n)def" << "\n";
                stop();
            }

            m_ifCondition = m_condStack.top();
            m_condStack.pop();
            continue;
        }

        if (!m_ifCondition)
            continue;

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
    obj = ObjectFactory::get()->createObject(typeName, parameters);
    if (obj)
        obj->setName(objName);
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
            if (m_prefix != "" && o == "platform") // подставляем вместо "platform" конкретное имя текущей платформы
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
