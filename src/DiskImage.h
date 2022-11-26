/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2022
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

#ifndef DISKIMAGE_H
#define DISKIMAGE_H

#include "PalFile.h"

#include "EmuObjects.h"


const int DISKIMAGE_NOTIFY_FILEOPENED = 1;

class DiskImage : public EmuObject
{
public:
    DiskImage();
    virtual ~DiskImage();

    bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
    std::string getPropertyStringValue(const std::string& propertyName) override;

    bool assignFileName(std::string fileName);
    void chooseFile();
    void close();
    int64_t getSize();
    void setWriteProtection(bool isWriteProtected);
    inline void setLabel(std::string label) {m_label = label;}
    inline std::string getLabel() {return m_label;}

    bool getWriteProtectStatus();
    bool getImagePresent() {return m_file.isOpen();}

    void setCurOffset(int offset);
    bool read(uint8_t* buf, int len);
    bool write(uint8_t* buf, int len);
    uint8_t read8();

    void setOwner(EmuObject* owner) {m_owner = owner;}

    static EmuObject* create(const EmuValuesList&) {return new DiskImage();}

protected:
    bool m_isWriteProtected = false;
    std::string m_fileName;
    std::string m_permanentFileName;
    bool m_autoMount = false;
    std::string m_filter;
    PalFile m_file;
    std::string m_label;

private:
    EmuObject* m_owner = nullptr;
};


class FdImage : public DiskImage
{
    public:
        FdImage(int nTracks, int nHeads, int nSectors, int sectorSize);

        void reset() override;
        //bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        //std::string getPropertyStringValue(const std::string& propertyName) override;

        void setCurTrack(int track);
        void setCurHead(int head);
        void startSectorAccess(int sector);

        int readSectorAddress();
        bool getReadyStatus();

        uint8_t readNextByte();
        uint8_t readByte(int offset);

        void writeNextByte(uint8_t bt);
        void writeByte(int offset, uint8_t bt);

        static EmuObject* create(const EmuValuesList& parameters) {return new FdImage(parameters[0].asInt(), parameters[1].asInt(), parameters[2].asInt(), parameters[3].asInt());} // add check!

    private:
        int m_nTracks;
        int m_nHeads;
        int m_nSectors;
        int m_sectorSize;

        int m_curTrack;
        int m_curHead;
        int m_curSector;
        int m_curSectorOffset;

        void seek(int offset);
};

#endif // DISKIMAGE_H
