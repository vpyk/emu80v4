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

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <stack>
#include <map>

#include "EmuObjects.h"


static EmuValuesList emptyValues("", "", "");

class ConfigReader : public EmuObject
{
    public:
        ConfigReader(std::string configFileName, std::string platformName = "");
        virtual ~ConfigReader();

        //bool setProperty(const string& propertyName, const EmuValuesList& values) override;

        bool processConfigFile(ParentObject* parent);

    private:
        struct ConfigReaderState {
            std::istringstream* inputStream;
            int curLine;
            std::string configFileName;
        };

        std::string m_prefix;
        std::string m_configFileName;
        std::istringstream* m_inputStream;
        int m_curLine;
        std::map<std::string, std::string> m_varMap;
        std::stack<ConfigReaderState> m_stateStack;
        std::stack<bool> m_condStack; // стек условий
        int m_condLevel = 0; // уровень условий
        bool m_ifCondition = true; // условное выполнение

        void openFile();
        bool getNextLine(std::string& typeName, std::string& objName, std::string& propname, EmuValuesList* values);
        void fillValuesList(std::string s, EmuValuesList* values);
        EmuObject* createObject(std::string typeName, std::string objName, const EmuValuesList& parameters = ::emptyValues);
        void logPrefix();
        void stop();
};

#endif // CONFIG_H
