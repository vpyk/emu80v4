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

#ifndef RAMDISK_H
#define RAMDISK_H

#include <string>

#include "EmuObjects.h"

class AddressableDevice;


class RamDisk : public EmuObject
{
    public:
        RamDisk(unsigned nPages, unsigned pageSize = 0);
        ~RamDisk();

        void attachPage(unsigned pageNo, AddressableDevice* as);

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        void init() override;
        void shutdown() override;

        void loadFromFile();
        void saveToFile();

        void openFile();
        void saveFileAs();

        static EmuObject* create(const EmuValuesList& parameters) {return new RamDisk(parameters[0].asInt(), parameters[1].asInt());} // add checks !

    private:
        unsigned m_nPages;
        unsigned m_defPageSize;

        AddressableDevice** m_pages = nullptr;

        std::string m_filter;

        std::string m_fileName;
        bool m_autoLoad = false;
        bool m_autoSave = false;
};


#endif // RAMDISK_H
