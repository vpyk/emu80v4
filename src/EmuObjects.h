/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2025
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

#ifndef EMUOBJECTS_H
#define EMUOBJECTS_H

#include <cstdint>

#include <vector>
#include <list>
#include <string>
#include <map>
#include <functional>

#include "Parameters.h"


typedef std::function<void(uint32_t)> SetFunc;
typedef std::function<void(int, uint32_t)> SetFuncIndexed;
typedef std::function<void(uint32_t, uint32_t)> SetFuncMasked;

class EmuObject;


class EmuInput {
public:
    EmuInput(SetFunc* setFunc) : m_setFunc(setFunc) {}
    EmuInput(SetFuncIndexed* setFuncIndexed) : m_setFuncIndexed(setFuncIndexed) {}
    EmuInput(SetFuncMasked* setFuncMasked) : m_setFuncMasked(setFuncMasked) {}
    ~EmuInput();

    void setValue(uint32_t value);
    void setValue(int index, uint32_t value);
    void setMaskedValue(uint32_t value, uint32_t mask);

private:
    SetFunc* m_setFunc = nullptr;
    SetFuncIndexed* m_setFuncIndexed = nullptr;
    SetFuncMasked* m_setFuncMasked = nullptr;
};


struct EmuConnectionParams {
    enum class OutputType {
        Ordinary,
        Masked,
        Indexed
    };

    OutputType type = OutputType::Ordinary;
    int index = 0;
    uint32_t andMask = 0xFFFFFFFF;
    uint32_t xorMask = 0;
    int shift = 0;
    uint32_t outputMask = 0xFFFFFFFF;
    int outputShift = 0;
};


struct EmuConnection {
    EmuInput* input = nullptr;
    EmuConnectionParams params;

    // for future use
    //EmuObject* inputObj;
    //std::string inputName;
};


class EmuOutput {
public:
    void addConnection(EmuConnection connection);
    void setValue(uint32_t value);

private:
    std::vector<EmuConnection> m_connections;
};


class Platform;

class EmuObject
{
    public:
        EmuObject();
        virtual ~EmuObject();

        EmuInput* getInputByName(const std::string& inputName);

        bool connect(const std::string& outputName, EmuObject* targetObject, const std::string& inputName, EmuConnectionParams params);
        bool connect(const std::string& outputName, EmuObject* targetObject, const std::string& inputName);

        virtual void initConnections() {}

        void setName(std::string name);
        std::string getName();

        int getKDiv() {return m_kDiv;}

        virtual void setFrequency(int64_t freq); // лучше бы в одном из производных классов, но пусть пока будет здесь
        virtual void init() {}
        virtual void shutdown() {}

        virtual void reset() {}

        virtual void setPlatform(Platform* platform) {m_platform = platform;}
        Platform* getPlatform() {return m_platform;}

        virtual bool setProperty(const std::string& propertyName, const EmuValuesList& values);
        virtual std::string getPropertyStringValue(const std::string& propertyName);

        virtual std::string getDebugInfo() {return "";}
        virtual void notify(EmuObject* /*sender*/, int /*data*/) {}

    protected:
        EmuOutput* registerOutput(const std::string outputName);

        EmuInput* registerInput(const std::string inputName, SetFunc* setFunc);
        EmuInput* registerIndexedInput(const std::string inputName, SetFuncIndexed* setFuncIndexed);
        EmuInput* registerMaskedInput(const std::string inputName, SetFuncMasked* setFuncMasked);

        int m_kDiv = 1;
        Platform* m_platform = nullptr;
        static EmuObject* findObj(const std::string& objName);

    private:
        std::string m_name;

        std::map<std::string, EmuOutput*> m_outputMap;
        std::map<std::string, EmuInput*> m_inputMap;
};


#define REG_INPUT(name, func) registerInput(name, new SetFunc(std::bind(&func, this, std::placeholders::_1)));
#define REG_INDEXED_INPUT(name, func) registerIndexedInput(name, new SetFuncIndexed(std::bind(&func, this, std::placeholders::_1, std::placeholders::_2)));
#define REG_MASKED_INPUT(name, func) registerMaskedInput(name, new SetFuncMasked(std::bind(&func, this, std::placeholders::_1, std::placeholders::_2)));

#define REG_OUTPUT(name, output) output = registerOutput(name);


class AddressableDevice : public EmuObject
{
    public:
        //AddressableDevice();
        virtual ~AddressableDevice() {} // !!!

        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;

        virtual void writeByte(int addr, uint8_t value) = 0;
        virtual uint8_t readByte(int) {return 0xFF;}

        uint8_t readByteEx(int addr, int& tag);
        void writeByteEx(int addr, uint8_t value, int& tag);

        void setAddrMask(int mask) {m_addrMask = mask;}

    protected:
        int m_addrMask = 0;

        bool m_supportsTags = false;
        int m_tag = 0;
        static int m_lastTag;

    private:
};


class IActive
{
    public:
        IActive();
        virtual ~IActive();
        uint64_t getClock() {return m_curClock;}
        //void setClock(uint64_t clock) {m_curClock = clock;}
        void pause() {m_isPaused = true; m_curClock = -1;}
        void resume() {m_isPaused = false;}
        void syncronize(uint64_t curClock) {m_curClock = curClock;}
        void syncronize();
        inline bool isPaused() {return m_isPaused;}
        virtual void operate() = 0;

    protected:
        //int m_kDiv = 1;
        uint64_t m_curClock = 0;
        bool m_isPaused = false;
};


class ActiveDevice : public EmuObject, public IActive
{

};


class ParentObject : public EmuObject
{
    public:
        virtual void addChild(EmuObject* child) = 0;

};


class EmuObjectGroup : public EmuObject
{
    public:
        bool setProperty(const std::string& propertyName, const EmuValuesList& values) override;
        std::string getPropertyStringValue(const std::string& propertyName) override;
        void addItem(EmuObject* item);

        static EmuObject* create(const EmuValuesList&) {return new EmuObjectGroup();}

    private:
        std::list<EmuObject*> m_objectList;
};

#endif // EMUOBJECTS_H
