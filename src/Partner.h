/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2018
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

#ifndef PARTNER_H
#define PARTNER_H

#include "AddrSpace.h"
#include "PlatformCore.h"
#include "RkPpi8255Circuit.h"
#include "Crt8275Renderer.h"

class EmuWindow;
class Ram;
class Dma8257;
class Cpu;
class Cpu8080Compatible;
class RkKeyboard;
class Ppi8255;
class SoundMixer;
class Fdc1793;


class PartnerAddrSpaceSelector;

class PartnerAddrSpace : public AddressableDevice
{
    public:
        PartnerAddrSpace(std::string fileName);
        virtual ~PartnerAddrSpace();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        void reset()  override {m_mapNum = 0;}

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        void setMemBlock(int blockNum, AddressableDevice* memBlock);

        static EmuObject* create(const EmuValuesList& parameters) {return new PartnerAddrSpace(parameters[0].asString());}

        friend PartnerAddrSpaceSelector;

    protected:

    private:
        uint8_t* m_buf = nullptr;
        AddressableDevice* m_memBlocks[9];
        int m_mapNum = 0;
};



class PartnerAddrSpaceSelector : public AddressableDevice
{
    public:
        //PartnerAddrSpaceSelector();
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void attachPartnerAddrSpace(PartnerAddrSpace* partnerAddrSpace) {m_partnerAddrSpace = partnerAddrSpace;}

        void writeByte(int, uint8_t value) override {m_partnerAddrSpace->m_mapNum = (value & 0xf0) >> 4;}
        uint8_t readByte(int)  override {return 0xff;}

        static EmuObject* create(const EmuValuesList&) {return new PartnerAddrSpaceSelector();}

    private:
        PartnerAddrSpace* m_partnerAddrSpace = nullptr;
};



class PartnerModuleSelector : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void attachAddrSpaceMappers(AddrSpaceMapper* romWinAddrSpaceMapper, AddrSpaceMapper* ramWinAddrSpaceMapper, AddrSpaceMapper* devWinAddrSpaceMapper);

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int) override {return 0xff;}

        static EmuObject* create(const EmuValuesList&) {return new PartnerModuleSelector();}

    private:
        AddrSpaceMapper* m_romWinAddrSpaceMapper = nullptr;
        AddrSpaceMapper* m_ramWinAddrSpaceMapper = nullptr;
        AddrSpaceMapper* m_devWinAddrSpaceMapper = nullptr;
};


class PartnerMcpgSelector : public AddressableDevice
{
    public:
        void reset() override {m_isMcpgEnabled = false;}
        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int) override {return 0xff;}

        bool getMcpgEnabled() {return m_isMcpgEnabled;}

        static EmuObject* create(const EmuValuesList&) {return new PartnerMcpgSelector();}

    private:
        bool m_isMcpgEnabled = false;
};


class PartnerRamUpdater : public ActiveDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void operate() override;

        void attachDma(Dma8257* dma, int channel);

        static EmuObject* create(const EmuValuesList&) {return new PartnerRamUpdater();}

private:
        Dma8257* m_dma;
        int m_dmaChannel;
};



class PartnerCore : public PlatformCore
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        void reset() override;

        void draw() override;

        void inte(bool isActive) override;
        void vrtc(bool isActive) override;
        void hrtc(bool isActive, int lc) override;
        //void tapeOut(bool isActive) override;

        void setBeepGate(bool isSet);

        void attachCpu(Cpu8080Compatible* cpu);
        void attach8275Renderer(Crt8275Renderer* crtRenderer);
        void attach8275McpgRenderer(Crt8275Renderer* crtMcpgRenderer);
        void attachMcpgSelector(PartnerMcpgSelector* mcpgSelector);

        static EmuObject* create(const EmuValuesList&) {return new PartnerCore();}

    private:
        Cpu8080Compatible* m_cpu = nullptr;
        Crt8275Renderer* m_crtRenderer = nullptr;
        Crt8275Renderer* m_crtMcpgRenderer = nullptr;
        PartnerMcpgSelector* m_mcpgSelector = nullptr;

        bool m_beep = false;
        bool m_beepGate = false;
        //bool m_tapeOut = false;

        bool m_intReq = false;

        GeneralSoundSource* m_beepSoundSource;
};



// Обвязка основного ВВ55 в Партнере
class PartnerPpi8255Circuit : public RkPpi8255Circuit
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        uint8_t getPortC() override;
        void setPortC(uint8_t value) override;

        void attachCore(PartnerCore* core);

        static EmuObject* create(const EmuValuesList&) {return new PartnerPpi8255Circuit();}

    private:
        PartnerCore* m_core = nullptr;
};


// Отрисовщик экрана
class PartnerRenderer : public Crt8275Renderer
{
    public:
        PartnerRenderer();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        static EmuObject* create(const EmuValuesList&) {return new PartnerRenderer();}

    protected:
        const uint8_t* getCurFontPtr(bool gpa0, bool gpa1, bool hglt) override;
        const uint8_t* getAltFontPtr(bool gpa0, bool gpa1, bool hglt) override;
        uint32_t getCurFgColor(bool gpa0, bool gpa1, bool hglt) override;
        uint32_t getCurBgColor(bool gpa0, bool gpa1, bool hglt) override;
};


// Отрисовщик экрана МЦПГ
class PartnerMcpgRenderer : public Crt8275Renderer
{
    public:
        PartnerMcpgRenderer();

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        void attachMcpgRam(Ram* mcpgRam);

        static EmuObject* create(const EmuValuesList&) {return new PartnerMcpgRenderer();}

    protected:
        uint32_t getCurFgColor(bool gpa0, bool gpa1, bool hglt) override;
        uint32_t getCurBgColor(bool gpa0, bool gpa1, bool hglt) override;
        void customDrawSymbolLine(uint32_t* linePtr, uint8_t symbol, int line, bool lten, bool vsp, bool rvv, bool gpa0, bool gpa1, bool hglt) override;

    private:
        const uint8_t* m_fontPtr = nullptr;
};


class PartnerFddControlRegister : public AddressableDevice
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        inline void attachFdc1793(Fdc1793* fdc) {m_fdc = fdc;}

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int) override {return 0xff;}

        static EmuObject* create(const EmuValuesList&) {return new PartnerFddControlRegister();}

    private:
        Fdc1793* m_fdc = nullptr;
};

#endif // PARTNER_H
