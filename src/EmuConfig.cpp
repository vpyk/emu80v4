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

#include "EmuConfig.h"
#include "EmuConfig.h"
#include "Emulation.h"
#include "Platform.h"

#include "Pal.h"

using namespace std;


void EmuConfig::addPlatform(string platformName, string configFileName, string objName, string cmdLineOption)
{
    PlatformInfo pi;
    pi.platformName = platformName;
    pi.configFileName = configFileName;
    pi.objName = objName;
    pi.cmdLineOption = cmdLineOption;
    m_platformVector.push_back(pi);
}


void EmuConfig::addExtention(std::string extention, std::string objName)
{
    m_extentionMap[extention] = objName;
}


bool EmuConfig::choosePlatform(PlatformInfo& pi, string curPlatformName, bool& newWnd, bool setDefault)
{
    int pos = 0;
    for (unsigned i = 0; i < m_platformVector.size(); i++)
    if (m_platformVector[i].objName == curPlatformName) {
        pos = i;
        break;
    }

    if (palChoosePlatform(m_platformVector, pos, newWnd, setDefault))
    {
        pi = m_platformVector[pos];
        return true;
    }
    return false;
}


void EmuConfig::showConfigWindow(int configTabId)
{
    palShowConfigWindow(configTabId);
}


void EmuConfig::updateConfig()
{
    palUpdateConfig();
}


bool EmuConfig::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "addPlatform") {
        addPlatform(values[0].asString(), values[1].asString(), values[2].asString(), values[3].asString());
        return true;
    } else if (propertyName == "addExtention") {
        addExtention(values[0].asString(), values[1].asString());
        return true;
    } else if (propertyName == "runFileName") {
        palSetRunFileName(palMakeFullFileName(values[0].asString()));
        return true;
    }

    return false;
}


int EmuConfigTab::s_curId = 1;


EmuConfigTab::EmuConfigTab(string tabName)
{
    m_tabId = s_curId++;
    palAddTabToConfigWindow(m_tabId, tabName);
}


EmuConfigTab::~EmuConfigTab()
{
    palRemoveTabFromConfigWindow(m_tabId);
}


void EmuConfigTab::addControl(int column, EmuConfigControl* control)
{
    ControlInfo ci;
    ci.column = column;
    ci.control = control;
    m_configControlVector.push_back(ci);

    EmuConfigRadioSelector* ecrs = dynamic_cast<EmuConfigRadioSelector*>(control);
    if (ecrs)
        palAddRadioSelectorToTab(m_tabId, column, ecrs->getCaption(), ecrs->getObjName(), ecrs->getPropName(), ecrs->getItems(), ecrs->getNItems());
}


bool EmuConfigTab::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "optFileName") {
        palSetTabOptFileName(m_tabId, palMakeFullFileName(values[0].asString()));
        return true;
    } else if (propertyName == "addControl") {
        if (!values[0].isInt())
            return false;
        int col = values[0].asInt();
        if (col < 1 || col > 3)
            return false;
        EmuConfigControl* control = dynamic_cast<EmuConfigControl*>(g_emulation->findObject(values[1].asString()));
        if (!control)
            return false;
        addControl(col, control);
        return true;
    }
    return false;
}


EmuConfigRadioSelector::EmuConfigRadioSelector(string objName, string propName, string caption)
{
    m_objName = objName;
    m_propName = propName;
    m_caption = caption;
}


EmuConfigRadioSelector::~EmuConfigRadioSelector()
{
    // dtor
}


void EmuConfigRadioSelector::addItem(string value, string name)
{
    SelectItem si;
    si.name = name;
    si.value = value;
    si.selected = value == g_emulation->findObject(m_objName)->getPropertyStringValue(m_propName);
    m_selectItemVector.push_back(si);
}


SelectItem* EmuConfigRadioSelector::getItems()
{
    return m_selectItemVector.data();
}


int EmuConfigRadioSelector::getNItems()
{
    return m_selectItemVector.size();
}


string EmuConfigRadioSelector::getCaption()
{
    return m_caption;
}

string EmuConfigRadioSelector::getObjName()
{
    return m_objName;
}

string EmuConfigRadioSelector::getPropName()
{
    return m_propName;
}

bool EmuConfigRadioSelector::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "addItem") {
        if (values[0].asString() == "" || values[1].asString() == "")
            return false;
        addItem(values[0].asString(), values[1].asString());
        return true;
    }
    return false;
}
