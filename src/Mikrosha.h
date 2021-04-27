/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2018
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

#ifndef MIKROSHA_H
#define MIKROSHA_H

#include "EmuObjects.h"
#include "PlatformCore.h"
#include "Pit8253Sound.h"
#include "RkPpi8255Circuit.h"
#include "Crt8275Renderer.h"

class MikroshaCore : public PlatformCore
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void draw() override;
        void vrtc(bool isActive) override;

        void attachCrtRenderer(Crt8275Renderer* crtRenderer);

        static EmuObject* create(const EmuValuesList&) {return new MikroshaCore();}

    private:
        Crt8275Renderer* m_crtRenderer = nullptr;
};


class MikroshaPit8253SoundSource : public Pit8253SoundSource
{
    public:
        int calcValue() override;

        void setGate(bool gate);

        //static EmuObject* create(const EmuValuesList&) {return new MikroshaPit8253SoundSource();}

    private:
        bool m_gate = false;
        int m_sumValue;

        void updateStats();
};


// Обвязка основного ВВ55 в Микроше
class MikroshaPpi8255Circuit : public RkPpi8255Circuit
{
    public:
        MikroshaPpi8255Circuit() : RkPpi8255Circuit() {}
        virtual ~MikroshaPpi8255Circuit();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        uint8_t getPortA() override;
        uint8_t getPortB() override;
        //uint8_t getPortC() override;
        void setPortA(uint8_t) override {}
        void setPortB(uint8_t value) override;
        void setPortC(uint8_t value) override;

        void attachPit(Pit8253* pit);

        static EmuObject* create(const EmuValuesList&) {return new MikroshaPpi8255Circuit();}

    private:
        Pit8253* m_pit = nullptr;
        MikroshaPit8253SoundSource* m_pitSoundSource = nullptr;
};


// Обвязка дополнительного ВВ55 в Микроше
class MikroshaPpi2Circuit : public Ppi8255Circuit
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        void setPortB(uint8_t value) override;

        void attachCrtRenderer(Crt8275Renderer* crtRenderer);

        static EmuObject* create(const EmuValuesList&) {return new MikroshaPpi2Circuit();}

    private:
        Crt8275Renderer* m_crtRenderer = nullptr;
};


class MikroshaRenderer : public Crt8275Renderer
{
    public:
        MikroshaRenderer();

        static EmuObject* create(const EmuValuesList&) {return new MikroshaRenderer();}

    protected:
        const uint8_t* getCurFontPtr(bool gpa0, bool gpa1, bool hglt) override;
        const uint8_t* getAltFontPtr(bool gpa0, bool gpa1, bool hglt) override;
        uint32_t getCurFgColor(bool gpa0, bool gpa1, bool hglt) override;
        uint32_t getCurBgColor(bool gpa0, bool gpa1, bool hglt) override;
        char16_t getUnicodeSymbol(uint8_t chr, bool gpa0, bool gpa1, bool hglt) override;

    private:
        const char16_t* c_mikroshaSymbols =
            u" ▘▝▀▗▚▐▜ ★ ↑  ↣↓▖▌▞▛▄▙▟█   ┃━↢● "
            u" !\"#¤%&'()*+,-./0123456789:;<=>?"
            u"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
            u"ЮАБЦДЕФГХИЙКЛМНОПЯРСТУЖВЬЫЗШЭЩЧЪ"
            u" ┛┏┓┣┫┳┻┃━ ╋┗ ↑↓→←♥♠♣♦★⚐··☉··✿··"
            u" !\"#$%&'()*+,-./0123456789:;<=>?"
            u"юабцдефгхийклмнопярстужвьызшэщч█"
            u"ЮАБЦДЕФГХИЙКЛМНОПЯРСТУЖВЬЫЗШЭЩЧ▇";
};


#endif // MIKROSHA_H
