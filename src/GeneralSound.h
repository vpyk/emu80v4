/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2025
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

#ifndef GENERALSOUND_H
#define GENERALSOUND_H

#include "EmuObjects.h"
#include "SoundMixer.h"


class GsPorts : public AddressableDevice
{
public:
    //void reset() override;

    void initConnections() override;

    void writeByte(int addr, uint8_t value) override;
    uint8_t readByte(int addr) override;

    //bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

    static EmuObject* create(const EmuValuesList&) {return new GsPorts();}

private:
    uint8_t m_command = 0;
    uint8_t m_dataOut = 0;
    uint8_t m_dataIn = 0;
    uint8_t m_status = 0;

    EmuOutput* m_dataOutput = nullptr;
    EmuOutput* m_commandOutput = nullptr;
    EmuOutput* m_statusOutput = nullptr;

    void setDataIn(uint8_t dataIn);
    void setStatus(uint8_t status);
};


class GsInternalPorts : public AddressableDevice
{
public:
    //void reset() override;

    void initConnections() override;

    void writeByte(int addr, uint8_t value) override;
    uint8_t readByte(int addr) override;

    bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

    static EmuObject* create(const EmuValuesList&) {return new GsInternalPorts();}

private:
    uint8_t m_command = 0;
    uint8_t m_dataOut = 0;
    uint8_t m_dataIn = 0;
    uint8_t m_status = 0;
    int m_memPage = 0;
    uint8_t m_pageMask = 0x0f;

    EmuOutput* m_dataOutput = nullptr;
    EmuOutput* m_statusOutput = nullptr;
    EmuOutput* m_memPageOutput = nullptr;

    EmuOutput* m_volumeOutputs[4] = {nullptr, nullptr, nullptr, nullptr};

    void setDataIn(uint8_t dataIn);
    void setStatus(uint8_t status);
    void setCommand(uint8_t command);
};


class GsSoundMem : public AddressableDevice
{
public:
    //void reset() override;

    void initConnections() override;

    void writeByte(int addr, uint8_t value) override;
    uint8_t readByte(int addr) override;

    bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

    static EmuObject* create(const EmuValuesList&) {return new GsSoundMem();}

private:
    AddressableDevice* m_as;

    EmuOutput* m_sampleOutputs[4] = {nullptr, nullptr, nullptr, nullptr};
};


class GsSoundSource : public SoundSource
{
public:
    void initConnections() override;

    //bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
    //std::string getPropertyStringValue(const std::string& propertyName) override;

    int calcValue() override;
    StereoSample getSample() override;
    int getMinimumSampleValue() override;

    static EmuObject* create(const EmuValuesList&) {return new GsSoundSource();}

private:
    uint64_t m_initClock = 0;
    uint64_t m_prevClock = 0;
    uint8_t m_curValues[4] = {0, 0, 0, 0};
    int m_curVolumes[4] = {0, 0, 0, 0};
    int m_sumVals[4] = {0, 0, 0, 0};

    void setSample(int ch, uint8_t sample);
    void setVolume(int ch, uint8_t volume);
    void updateStats();
};


#endif // GENERALSOUND_H
