/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2024
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

#ifndef MIKRO80_H
#define MIKRO80_H

#include "EmuObjects.h"
#include "CrtRenderer.h"
#include "PlatformCore.h"

class Ram;
class GeneralSoundSource;


class Mikro80Renderer : public TextCrtRenderer, public IActive
{
    public:
        Mikro80Renderer();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        const char* getTextScreen() override;

        // derived from ActiveDevice
        void operate() override;

        void attachScreenMemory(Ram* screenMemory);

        static EmuObject* create(const EmuValuesList&) {return new Mikro80Renderer();}

    private:
        const uint8_t* m_screenMemory = nullptr;

        void primaryRenderFrame() override;
        void altRenderFrame() override;
};


class Mikro80Core : public PlatformCore
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void draw() override;

        void attachCrtRenderer(Mikro80Renderer* crtRenderer);

        static EmuObject* create(const EmuValuesList&) {return new Mikro80Core();}

private:
        Mikro80Renderer* m_crtRenderer = nullptr;
};


class Mikro80TapeRegister : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from AddressableDevice
        void writeByte(int nAddr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        static EmuObject* create(const EmuValuesList&) {return new Mikro80TapeRegister();}

    private:
        // Источник звука - вывод на магнитофон
        GeneralSoundSource* m_tapeSoundSource;
};


#endif // MIKRO80_H

