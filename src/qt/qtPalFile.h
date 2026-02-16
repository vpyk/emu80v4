/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017–2022
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

#ifndef QTPALFILE_H
#define QTPALFILE_H

#include <string>

#include <QFile>

class PalFile
{
    public:
        PalFile() = default;
        ~PalFile();

        bool open(const std::string &fileName, const std::string &mode = "r");
        void close();
        bool isOpen() const;
        bool eof() const;
        uint8_t read8();
        uint16_t read16();
        uint32_t read32();
        void write8(uint8_t value);
        void write16(uint16_t value);
        void write32(uint32_t value);
        int64_t getSize() const;
        int64_t getPos() const;
        void seek(int position);
        void skip(int len);

        static bool create(const std::string &fileName);
        static bool del(const std::string &fileName);
        static bool mkDir(const std::string &dirName);
        static bool moveRename(const std::string &src, const std::string &dst);

    private:
        QFile* m_file = nullptr;
};

#endif // QTPALFILE_H
