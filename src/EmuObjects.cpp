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

#include "Globals.h"

#include "EmuObjects.h"
#include "Emulation.h"

using namespace std;


// ##### EmuInput methods #####

EmuInput::~EmuInput()
{
    if (m_setFunc)
        delete m_setFunc;
}


void EmuInput::setValue(int index, uint32_t value)
{
    if (m_setFuncIndexed)
        (*m_setFuncIndexed)(index, value);
}


void EmuInput::setValue(uint32_t value)
{
    if (m_setFunc)
        (*m_setFunc)(value);
}


// ##### EmuOutput methods #####

void EmuOutput::addConnection(EmuConnection connection)
{
    m_connections.push_back(connection);
}


void EmuOutput::setValue(uint32_t value)
{
    for (auto& conn: m_connections) {
        if (conn.params.indexed)
            conn.input->setValue(conn.params.index, ((value ^ conn.params.xorMask) & conn.params.andMask) >> conn.params.shift);
        else
            conn.input->setValue(((value ^ conn.params.xorMask) & conn.params.andMask) >> conn.params.shift);
    }
}


// ##### EmuObject methods #####

EmuObject::EmuObject()
{
    if (g_emulation)
        g_emulation->addObject(this);
}


EmuObject::~EmuObject()
{
    for (auto& inputItem: m_inputMap)
        delete inputItem.second;

    for (auto& outputItem: m_outputMap)
        delete outputItem.second;

    if (this != g_emulation)
        g_emulation->removeObject(this);
}


void EmuObject::setName(string name)
{
    m_name = name;
}


string EmuObject::getName()
{
    return m_name;
}


void EmuObject::setFrequency(int64_t freq)
{
    m_kDiv = (g_emulation->getFrequency() + freq / 2) / freq;
}


bool EmuObject::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (propertyName == "name") {
        setName(values[0].asString());
        return true;
    } else if (propertyName == "frequency")
        if (values[0].isInt()) {
            setFrequency(values[0].asInt());
            return true;
    }
    return false;
}

string EmuObject::getPropertyStringValue(const string& propertyName)
{
    if (propertyName == "name")
        return getName();

    return "";
}


EmuObject* EmuObject::findObj(const std::string& objName)
{
    return g_emulation->findObject(objName);
}


EmuOutput* EmuObject::registerOutput(const std::string outputName)
{
    EmuOutput* output = new EmuOutput();
    m_outputMap.insert(make_pair(outputName, output));
    return output;
}


EmuInput* EmuObject::registerInput(const std::string inputName, SetFunc* setFunc)
{
    EmuInput* input = new EmuInput(setFunc);
    m_inputMap.insert(make_pair(inputName, input));
    return input;
}


EmuInput* EmuObject::registerIndexedInput(const std::string inputName, SetFuncIndexed* setFuncIndexed)
{
    EmuInput* input = new EmuInput(setFuncIndexed);
    m_inputMap.insert(make_pair(inputName, input));
    return input;//m_curInputId;
}


EmuInput* EmuObject::getInputByName(const std::string& inputName)
{
    if (m_inputMap.find(inputName) == m_inputMap.end())
        return nullptr; // input not found

    return m_inputMap[inputName];
}


bool EmuObject::connect(const std::string& outputName, EmuObject* targetObject, const std::string& inputName, EmuConnectionParams params)
{
    if (m_outputMap.find(outputName) == m_outputMap.end())
        return false; // output not found
    EmuOutput* output = m_outputMap[outputName];

    EmuInput* input = targetObject->getInputByName(inputName);
    if (!input)
        return false;

    EmuConnection conn;
    conn.input = input;
    conn.params = params;

    // for future use
    //conn.inputObj = targetObject;
    //conn.inputName = inputName;

    output->addConnection(conn);

    return true;
}


bool EmuObject::connect(const std::string& outputName, EmuObject* targetObject, const std::string& inputName)
{
    EmuConnectionParams params;
    return connect(outputName, targetObject, inputName, params);
}


IActive::IActive()
{
    m_curClock = g_emulation->getCurClock();
    g_emulation->registerActiveDevice(this);
}


IActive::~IActive()
{
    g_emulation->unregisterActiveDevice(this);
}


void IActive::syncronize()
{
    m_curClock = g_emulation->getCurClock();
}


//bool IActive::isPaused()
//{
//    return (m_curClock == -1);
//}

int AddressableDevice::m_lastTag;

uint8_t AddressableDevice::readByteEx(int addr, int& tag)
{
    AddressableDevice::m_lastTag = 0;
    uint8_t read = readByte(addr);
    tag = AddressableDevice::m_lastTag;
    return read;
}


void AddressableDevice::writeByteEx(int addr, uint8_t value, int& tag)
{
    AddressableDevice::m_lastTag = 0;
    writeByte(addr, value);
    tag = AddressableDevice::m_lastTag;
}


bool AddressableDevice::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "addrMask" && values[0].isInt()) {
            setAddrMask(values[0].asInt());
            return true;
    } else if (propertyName == "tag" && m_supportsTags && values[0].isInt()) {
            m_tag = values[0].asInt();
            return true;
    } else if (propertyName == "poke" && values[0].isInt() && values[1].isInt()) {
        writeByte(values[0].asInt(), values[1].asInt());
        return true;
    }

    return false;
}


void EmuObjectGroup::addItem(EmuObject* item)
{
    m_objectList.push_back(item);
}


bool EmuObjectGroup::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "addItem") {
        addItem(g_emulation->findObject(values[0].asString()));
        return true;
    }

    bool res = true;
    for (auto it = m_objectList.begin(); it != m_objectList.end(); it++)
        res = res && (*it)->setProperty(propertyName, values);

    return res;
}


string EmuObjectGroup::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = EmuObject::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (!m_objectList.empty())
        return (*(m_objectList.begin()))->getPropertyStringValue(propertyName);

    return "";
}
