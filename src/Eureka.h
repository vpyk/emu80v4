/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2018-2024
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

#ifndef EUREKA_H
#define EUREKA_H


#include "Specialist.h"


class GeneralSoundSource;

class EurekaCore : public SpecCore
{
    public:
        void inte(bool isActive) override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        static EmuObject* create(const EmuValuesList&) {return new EurekaCore();}

    private:
        GeneralSoundSource* m_inteSoundSource;
};


class EurekaRenderer : public CrtRenderer, public IActive
{
    const uint32_t eurekaPalette[4] = {
        0x000000, 0x0000FF, 0xFF0000, 0x00FF00,
    };

    public:
        EurekaRenderer();
        void renderFrame() override;

        void toggleCropping() override;

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;

        // derived from ActiveDevice
        void operate() override;

        inline void attachVideoRam(Ram* videoRam) {m_videoRam = videoRam->getDataPtr();}
        inline void setColorMode(bool colorMode) {m_colorMode = colorMode;}

        static EmuObject* create(const EmuValuesList&) {return new EurekaRenderer();}

    private:
        const uint8_t* m_videoRam = nullptr;
        bool m_colorMode = false;
        bool m_showBorder = false;
};


class EurekaPpi8255Circuit : public SpecPpi8255Circuit
{
    public:
        EurekaPpi8255Circuit(std::string romDiskName);
        ~EurekaPpi8255Circuit();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void setPortA(uint8_t value) override;
        void setPortC(uint8_t value) override;
        uint8_t getPortB() override;

        // Подключение объекта - рендерера
        inline void attachEurekaRenderer(EurekaRenderer* renderer) {m_renderer = renderer;}

        static EmuObject* create(const EmuValuesList& parameters) {return new EurekaPpi8255Circuit(parameters[0].asString());}

    private:
        SpecRomDisk* m_romDisk;
        bool m_useRomDisk = true;
        EurekaRenderer* m_renderer = nullptr;
};


#endif // EUREKA_H
