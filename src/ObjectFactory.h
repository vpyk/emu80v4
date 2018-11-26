/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2018
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

#ifndef OBJECTFACTORY_H
#define OBJECTFACTORY_H

#include <string>
#include <map>

#include "EmuObjects.h"

typedef EmuObject* (*CreateObjectFunc)(const EmuValuesList& parameters);

class ObjectFactory
{
    public:
        ~ObjectFactory() {m_objectMap.clear();}

        static ObjectFactory *get()
        {
            static ObjectFactory instance;
            return &instance;
        }

        EmuObject* createObject(const std::string& objectClassName, const EmuValuesList& parameters);

    private:
        ObjectFactory();
        ObjectFactory(const ObjectFactory&) { }
        ObjectFactory &operator=(const ObjectFactory&) {return *this;}

        void reg(const std::string& objectClassName, CreateObjectFunc pfnCreate);

        std::map<std::string, CreateObjectFunc> m_objectMap;
};

#endif // OBJECTFACTORY
