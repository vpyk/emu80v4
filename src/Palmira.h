/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2022
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

#ifndef PALMIRA_H
#define PALMIRA_H

#include "Rk86.h"

class Ram;
class AddrSpaceMapper;

class PalmiraCore : public PlatformCore
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void draw() override;

        void vrtc(bool isActive) override;

        void attachCrtRenderer(Crt8275Renderer* crtRenderer);

        static EmuObject* create(const EmuValuesList&) {return new PalmiraCore();}

    private:
        Crt8275Renderer* m_crtRenderer = nullptr;
};


class PalmiraRenderer : public Rk86Renderer
{
public:
    PalmiraRenderer();

    bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
    std::string getPropertyStringValue(const std::string& propertyName) override;

    void setExtFontMode(bool extFont) {m_useExtFont = extFont;}

    static EmuObject* create(const EmuValuesList&) {return new PalmiraRenderer();}

protected:
    const uint8_t* getCurFontPtr(bool gpa0, bool gpa1, bool hglt) override;

private:
    bool m_useExtFont = false;
    uint8_t* m_extFontPtr = nullptr;

    void attachFontRam(Ram* fontRam);
};

class PalmiraConfigRegister : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void writeByte(int addr, uint8_t value) override;
        //uint8_t readByte(int) override;

        static EmuObject* create(const EmuValuesList&) {return new PalmiraConfigRegister();}

    private:
        AddrSpaceMapper* m_lowerMemMapper;
        AddrSpaceMapper* m_upperMemMapper;
        AddrSpaceMapper* m_fontMemMapper;
        PalmiraRenderer* m_renderer = nullptr;
};


#endif // PALMIRA_H
