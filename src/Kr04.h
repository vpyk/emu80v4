/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2021
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

#ifndef KR04_H
#define KR04_H

#include "PlatformCore.h"
#include "AddrSpace.h"
#include "Crt8275Renderer.h"
#include "Ppi8255Circuit.h"
#include "Pit8253Sound.h"
#include "FileLoader.h"
#include "RkKeyboard.h"
#include "CpuWaits.h"

class GeneralSoundSource;
class AddrSpaceMapper;
class Cpu8080Compatible;

class Kr04Pit8253SoundSource;

class Kr04Core : public PlatformCore
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void draw() override;

        void vrtc(bool isActive) override;
        void hrtc(bool isActive, int lc) override;

        void attachCrtRenderer(Crt8275Renderer* crtRenderer);

        static EmuObject* create(const EmuValuesList&) {return new Kr04Core();}

    private:
        Cpu8080Compatible* m_cpu = nullptr;
        Crt8275Renderer* m_crtRenderer = nullptr;
        Pit8253* m_pit = nullptr;
        Kr04Pit8253SoundSource* m_kr04PitSoundSource = nullptr;
};


class Kr04PpiColor8255Circuit : public Ppi8255Circuit
{
    public:
        //bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        void setPortA(uint8_t value) override;
        void setPortB(uint8_t value) override;
        void setPortC(uint8_t value) override;

        static EmuObject* create(const EmuValuesList&) {return new Kr04PpiColor8255Circuit();}

        uint32_t translateColor(int rgb);

    private:
        const uint8_t c_colors[4] = {0, 85, 170, 255};

        uint8_t m_portA = 0;
        uint8_t m_portB = 0;
        uint8_t m_portC = 0;
};


class Kr04Renderer : public Crt8275Renderer
{
    enum Kr04ColorMode {
        KCM_MONO,
        KCM_COLOR,
        KCM_COLORMODULE
    };

    public:
        Kr04Renderer();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;
        void toggleColorMode() override;

        static EmuObject* create(const EmuValuesList&) {return new Kr04Renderer();}

        void setHiRes(bool hiRes) {m_hiRes = hiRes;}

    private:
        const uint8_t c_d46[32] = {
            0x35, 0x6F, 0x35, 0x6F, 0x7E, 0x7E, 0x7E, 0x7E, 0x37, 0x7B, 0x6D, 0x7E, 0x37, 0x7B, 0x6D, 0x7E,
            0x30, 0x6A, 0x30, 0x6A, 0x79, 0x79, 0x79, 0x79, 0x30, 0x7C, 0x6A, 0x79, 0x30, 0x7C, 0x6A, 0x79
        };

        //const uint8_t c_bwPalette[8] = {84, 112, 136, 165, 203, 231, 255, 0};
        //const uint8_t c_bwPalette[8] = {0, 36, 67, 103, 152, 188, 219, 255};
        const uint8_t c_bwPalette[8] = {0, 0, 0, 0, 82, 143, 194, 255};

        void setColorMode(Kr04ColorMode colorMode);

        void customDrawSymbolLine(uint32_t* linePtr, uint8_t symbol, int line, bool lten, bool vsp, bool rvv, bool gpa0, bool gpa1, bool hglt) override;
        char16_t getUnicodeSymbol(uint8_t chr, bool gpa0, bool gpa1, bool hglt) override;

        AddrSpace* m_memory = nullptr   ;
        Kr04PpiColor8255Circuit* m_colorCircuit = nullptr;

        Kr04ColorMode m_colorMode = KCM_COLORMODULE;
        bool m_hiRes = false;
};


class Kr04Keyboard : public Ms7007Keyboard
{
    public:
        void processKey(EmuKey key, bool isPressed) override;
        static EmuObject* create(const EmuValuesList&) {return new Kr04Keyboard();}
};


class Kr04KbdLayout : public RkKbdLayout
{
    public:
        Kr04KbdLayout() {/*m_separateRusLat = true;*/}

        static EmuObject* create(const EmuValuesList&) {return new Kr04KbdLayout();}

    protected:
        EmuKey translateKey(PalKeyCode keyCode) override;
        EmuKey translateUnicodeKey(unsigned unicodeKey, PalKeyCode key, bool& shift, bool& lang) override;
};


class Kr04Ppi8255Circuit : public Ppi8255Circuit
{
public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        // derived from Ppi8255Circuit
        uint8_t getPortB() override;
        uint8_t getPortC() override;
        void setPortA(uint8_t value) override;
        void setPortC(uint8_t value) override;
        void setPortCLoMode(bool isInput) override;

        // Подключение объекта - клавиатуры
        void attachKeyboard(Kr04Keyboard* kbd);

        static EmuObject* create(const EmuValuesList&) {return new Kr04Ppi8255Circuit();}

    private:
        // Источник звука - вывод на магнитофон
        GeneralSoundSource* m_tapeSoundSource;

        // Клавиатура типа РК86
        Kr04Keyboard* m_kbd = nullptr;

        AddrSpaceMapper* m_mapper;

        Kr04Renderer* m_renderer = nullptr;

        bool m_portCLoInputMode;
};


class Kr04Pit8253SoundSource : public Pit8253SoundSource
{
    public:
        int calcValue() override;

        static EmuObject* create(const EmuValuesList&) {return new Kr04Pit8253SoundSource();}

        void tuneupPit() override;
};


class Kr04FileLoader : public RkFileLoader
{
    public:
        Kr04FileLoader();

        static EmuObject* create(const EmuValuesList&) {return new Kr04FileLoader();}

    private:
        void afterReset() override;

};


class Kr04CpuWaits : public CpuWaits
{
public:
    int getCpuWaitStates(int memTag, int, int normalClocks) override;

    static EmuObject* create(const EmuValuesList&) {return new Kr04CpuWaits();}
};


#endif // KR04_H
