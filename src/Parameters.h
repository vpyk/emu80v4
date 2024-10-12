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

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <cstdint>

#include <vector>
#include <string>


class EmuValue {
    public:
        EmuValue();
        EmuValue(const std::string& str);
        EmuValue(int64_t n);
        //EmuValue& operator=(string str);
        //EmuValue& operator=(int n);
        const std::string& asString() const;
        int64_t asInt() const;
        bool isInt() const;
        double asFloat() const;
        bool isFloat() const;
        //bool isString() const;
    private:
        std::string m_sValue;
        int64_t m_nValue = 0;
        double m_fValue = 0.0;
        bool m_isInt = false;
        bool m_isFloat = false;
};

class EmuValuesList {
    public:
        EmuValuesList() {}
        EmuValuesList(std::string value1);
        EmuValuesList(std::string value1, std::string value2);
        EmuValuesList(std::string value1, std::string value2, std::string value3);
        //EmuValuesList(EmuValue& value1);
        //EmuValuesList(EmuValue& value1, EmuValue& value2);
        //EmuValuesList(EmuValue& value1, EmuValue& value2, EmuValue& value3);
        const EmuValue& operator[](int index) const;
        void addValue(std::string value);
        int size() const;
        void clearList();
    private:
        std::vector<EmuValue*> m_values;
        EmuValue emptyValue;
};


#endif // PARAMETERS_H
