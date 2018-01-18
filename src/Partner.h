/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2017
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

#include "EmuObjects.h"
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
        void reset()  override {m_mapNum = 0;};

        void writeByte(int addr, uint8_t value) override;
        uint8_t readByte(int addr) override;

        void setMemBlock(int blockNum, AddressableDevice* memBlock);

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

        void writeByte(int, uint8_t value) override {m_partnerAddrSpace->m_mapNum = (value & 0xf0) >> 4;};
        uint8_t readByte(int)  override {return 0xff;};

    private:
        PartnerAddrSpace* m_partnerAddrSpace = nullptr;
};



class PartnerModuleSelector : public AddressableDevice
{
    public:
        virtual bool setProperty(const std::string& propertyName, const EmuValuesList& values);

        void attachAddrSpaceMappers(AddrSpaceMapper* romWinAddrSpaceMapper, AddrSpaceMapper* ramWinAddrSpaceMapper, AddrSpaceMapper* devWinAddrSpaceMapper);

        virtual void writeByte(int addr, uint8_t value);
        virtual uint8_t readByte(int) {return 0xff;};

    private:
        AddrSpaceMapper* m_romWinAddrSpaceMapper = nullptr;
        AddrSpaceMapper* m_ramWinAddrSpaceMapper = nullptr;
        AddrSpaceMapper* m_devWinAddrSpaceMapper = nullptr;
};


class PartnerMcpgSelector : public AddressableDevice
{
    public:
        virtual void reset() {m_isMcpgEnabled = false;};
        virtual void writeByte(int addr, uint8_t value);
        virtual uint8_t readByte(int) {return 0xff;};

        bool getMcpgEnabled() {return m_isMcpgEnabled;};

    private:
        bool m_isMcpgEnabled = false;
};


class PartnerRamUpdater : public ActiveDevice
{
    public:
        virtual bool setProperty(const std::string& propertyName, const EmuValuesList& values);

        virtual void operate();

        void attachDma(Dma8257* dma, int channel);

    private:
        Dma8257* m_dma;
        int m_dmaChannel;
};



class PartnerCore : public PlatformCore
{
    public:
        PartnerCore();
        virtual ~PartnerCore();

        virtual bool setProperty(const std::string& propertyName, const EmuValuesList& values);
        virtual void reset();

        virtual void draw();

        virtual void inte(bool isActive);
        virtual void vrtc(bool isActive);
        virtual void hrtc(bool isActive, int lc);
        //virtual void tapeOut(bool isActive);

        void setBeepGate(bool isSet);

        void attachCpu(Cpu8080Compatible* cpu);
        void attach8275Renderer(Crt8275Renderer* crtRenderer);
        void attach8275McpgRenderer(Crt8275Renderer* crtMcpgRenderer);
        void attachMcpgSelector(PartnerMcpgSelector* mcpgSelector);

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
        virtual bool setProperty(const std::string& propertyName, const EmuValuesList& values);

        virtual uint8_t getPortC();
        virtual void setPortC(uint8_t value);

        void attachCore(PartnerCore* core);

    private:
        PartnerCore* m_core = nullptr;
};


// Отрисовщик экрана
class PartnerRenderer : public Crt8275Renderer
{
    public:
        PartnerRenderer();

        virtual bool setProperty(const std::string& propertyName, const EmuValuesList& values);

    protected:
        virtual const uint8_t* getCurFontPtr(bool gpa0, bool gpa1, bool hglt);
        virtual const uint8_t* getAltFontPtr(bool gpa0, bool gpa1, bool hglt);
        virtual uint32_t getCurFgColor(bool gpa0, bool gpa1, bool hglt);
        virtual uint32_t getCurBgColor(bool gpa0, bool gpa1, bool hglt);
};


// Отрисовщик экрана МЦПГ
class PartnerMcpgRenderer : public Crt8275Renderer
{
    public:
        PartnerMcpgRenderer();

        virtual bool setProperty(const std::string& propertyName, const EmuValuesList& values);

        void attachMcpgRam(Ram* mcpgRam);

    protected:
        virtual uint32_t getCurFgColor(bool gpa0, bool gpa1, bool hglt);
        virtual uint32_t getCurBgColor(bool gpa0, bool gpa1, bool hglt);
        virtual void customDrawSymbolLine(uint32_t* linePtr, uint8_t symbol, int line, bool lten, bool vsp, bool rvv, bool gpa0, bool gpa1, bool hglt);

    private:
        const uint8_t* m_fontPtr = nullptr;
};


class PartnerFddControlRegister : public AddressableDevice
{
    public:
        virtual bool setProperty(const std::string& propertyName, const EmuValuesList& values);

        inline void attachFdc1793(Fdc1793* fdc) {m_fdc = fdc;};

        virtual void writeByte(int addr, uint8_t value);
        virtual uint8_t readByte(int) {return 0xff;};

    private:
        Fdc1793* m_fdc = nullptr;
};

#endif // PARTNER_H
