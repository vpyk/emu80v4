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

#include <algorithm>

#include "Cpu.h"
#include "CpuHook.h"
#include "Emulation.h"
#include "PlatformCore.h"

using namespace std;

Cpu::Cpu()
{
    m_addrSpace = nullptr;
    m_ioAddrSpace = nullptr;
}



void Cpu::attachAddrSpace(AddressableDevice* as)
{
    m_addrSpace = as;
}



void Cpu::attachIoAddrSpace(AddressableDevice* as)
{
    m_ioAddrSpace = as;
}



void Cpu::attachCore(PlatformCore* core)
{
    m_core = core;
}


void Cpu::addHook(CpuHook* hook)
{
    m_hookVector.push_back(hook);
    m_nHooks++;
    hook->setCpu(this);
}


void Cpu::removeHook(CpuHook* hook)
{
    m_hookVector.erase(remove(m_hookVector.begin(), m_hookVector.end(), hook), m_hookVector.end());
    m_nHooks--; // добавить проверку на существование!
}


bool Cpu::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "addrSpace") {
        attachAddrSpace(static_cast<AddressableDevice*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "ioAddrSpace") {
        attachIoAddrSpace(static_cast<AddressableDevice*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "core") {
        attachCore(static_cast<PlatformCore*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "addHook") {
        addHook(static_cast<CpuHook*>(g_emulation->findObject(values[0].asString())));
        return true;
    } else if (propertyName == "startAddr" && values[0].isInt()) {
        setStartAddr(values[0].asInt());
        return true;
    }

    return false;
}


void Cpu8080Compatible::addHook(CpuHook* hook)
{
    Cpu::addHook(hook);
    m_hookAddrVector.push_back(hook->getHookAddr());
    m_hookAddresses = m_hookAddrVector.data();
}


void Cpu8080Compatible::removeHook(CpuHook* hook)
{
    Cpu::removeHook(hook);
    //m_hookAddrVector.erase(remove(m_hookAddrVector.begin(), m_hookAddrVector.end(), hook->getHookAddr()), m_hookAddrVector.end());
    // удаляем только один элемент, если несколько одинаковых
    for (auto it = m_hookAddrVector.begin(); it != m_hookAddrVector.end(); it++)
    if (*it == hook->getHookAddr()) {
        m_hookAddrVector.erase(it);
        break;
    }
    m_hookAddresses = m_hookAddrVector.data();
}


void Cpu8080Compatible::hrq(int ticks) {
    m_curClock += ticks;
}


int Cpu8080Compatible::io_input(int port)
{
    if (m_ioAddrSpace)
        return m_ioAddrSpace->readByte(port);
    else
        return m_addrSpace->readByte((port & 0xff) << 8 | (port & 0xff));
}


void Cpu8080Compatible::io_output(int port, int value)
{
    if (m_ioAddrSpace)
        m_ioAddrSpace->writeByte(port, value);
    else
        m_addrSpace->writeByte((port & 0xff) <<8 | (port & 0xff), value);
}
