/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2017
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

#include "Parameters.h"

using namespace std;

EmuValue::EmuValue()
{
    m_isInt = false;
    m_sValue = "";
}

EmuValue::EmuValue(const string& str)
{
    m_sValue = str;
    m_isInt = true;
    try {
        if (m_sValue.substr(0,2) == "0x") {
            string sValueHex = m_sValue.substr(2, m_sValue.size());
            istringstream iss(sValueHex);
            iss >> hex >> m_nValue;
        } else {
            istringstream iss(m_sValue);
            iss >> m_nValue;
        }
    }
    catch(...) {
        m_isInt = false;
    }
}


EmuValue::EmuValue(int64_t n)
{
    m_nValue = n;
    m_isInt = true;

    ostringstream oss;
    oss << n;
    m_sValue = oss.str();
}


int64_t EmuValue::asInt() const
{
    return m_isInt ? m_nValue : 0;
}


bool EmuValue::isInt() const
{
    return m_isInt;
}


const string& EmuValue::asString() const
{
    return m_sValue;
}


EmuValuesList::EmuValuesList(string value1)
{
    EmuValue* ev1 = new EmuValue(value1);
    m_values.push_back(ev1);
}


EmuValuesList::EmuValuesList(string value1, string value2)
{
    EmuValue* ev1 = new EmuValue(value1);
    EmuValue* ev2 = new EmuValue(value2);
    m_values.push_back(ev1);
    m_values.push_back(ev2);
}


EmuValuesList::EmuValuesList(string value1, string value2, string value3)
{
    EmuValue* ev1 = new EmuValue(value1);
    EmuValue* ev2 = new EmuValue(value2);
    EmuValue* ev3 = new EmuValue(value3);
    m_values.push_back(ev1);
    m_values.push_back(ev2);
    m_values.push_back(ev3);
}


/*
EmuValuesList::EmuValuesList(EmuValue& value1)
{
    m_values.push_back(value1);
}


EmuValuesList::EmuValuesList(EmuValue& value1, EmuValue& value2)
{
    m_values.push_back(value1);
    m_values.push_back(value2);
}


EmuValuesList::EmuValuesList(EmuValue& value1, EmuValue& value2, EmuValue& value3)
{
    m_values.push_back(value1);
    m_values.push_back(value2);
    m_values.push_back(value3);
}
*/

void EmuValuesList::addValue(string value)
{
    EmuValue* ev = new EmuValue(value);
    m_values.push_back(ev);
}


void EmuValuesList::clearList()
{
    for (auto it = m_values.begin(); it != m_values.end(); it++)
        delete (*it);
    m_values.clear();
}


int EmuValuesList::size() const
{
    return m_values.size();
}

const EmuValue& EmuValuesList::operator[](int n) const
{
    if (n < size())
        return *(m_values[n]);
    else
        return emptyValue;
}
