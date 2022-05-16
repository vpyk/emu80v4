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

#include <sstream>

#include "Emulation.h"
#include "WavReader.h"
#include "RkTapeHooks.h"
#include "Cpu8080.h"
#include "TapeRedirector.h"

using namespace std;


void RkTapeOutHook::reset()
{
    if (m_suspendPeriod)
        m_suspendEndTime = g_emulation->getCurClock() + g_emulation->getFrequency() * m_suspendPeriod / 1000;
    else
        m_suspendEndTime = 0;
}


bool RkTapeOutHook::hookProc()
{
    if (g_emulation->getCurClock() < m_suspendEndTime)
        return false;

    if (!m_isEnabled || (m_hasSignature && !checkSignature()))
        return false;

    if (m_file->isCancelled())
        return false;

    if (!m_file->isOpen()) {
        m_file->openFile();
        m_isSbFound = false;
    }

    if (m_file->isCancelled())
        return false;

    Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_cpu);

    uint8_t outByte;
    if (m_regA)
        outByte = (cpu->getAF() & 0xFF00) >> 8;
    else
        outByte = cpu->getBC() & 0xFF;

    if (m_isSbFound)
        m_file->writeByte(outByte);

    if (outByte == 0xE6)
        m_isSbFound = true;

    static_cast<Cpu8080Compatible*>(m_cpu)->ret();

    return true;
}


void RkTapeInHook::reset()
{
    if (m_suspendPeriod)
        m_suspendEndTime = g_emulation->getCurClock() + g_emulation->getFrequency() * m_suspendPeriod / 1000;
    else
        m_suspendEndTime = 0;
}


bool RkTapeInHook::hookProc()
{
    if (!m_isEnabled || (m_hasSignature && !checkSignature()))
        return false;

    if (g_emulation->getWavReader()->isPlaying())
        return false;

    if (g_emulation->getCurClock() < m_suspendEndTime)
        return false;

    if (m_file->isCancelled())
        return false;

    uint8_t inByte = 0;
    uint8_t sb = 0xE6;

    Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_cpu);

    uint16_t af = cpu->getAF();

    if (m_file) {
        if (m_file->getPos() == 0 && m_file->peekByte() == 0xE6)
            inByte = m_file->readByte(); // если файл с первым синхробайтом (*.gam), то пропускаем его

        //if ((af & 0xff00) != 0x0800 && m_file->getPos() != 0 && inByte != 0xe6)
        if ((af & 0x8000) && m_file->getPos() != 0 && inByte != 0xe6)
            m_file->waitForSequence(&sb, 1);

        inByte = m_file->readByte();
    }

    if (m_file->isCancelled())
        return false;

    cpu->setAF((af & 0xFE) | (inByte << 8));

    static_cast<Cpu8080Compatible*>(m_cpu)->ret();

    return true;
}


bool RkTapeOutHook::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (CpuHook::setProperty(propertyName, values))
        return true;

    if (propertyName == "outReg") {
        if (values[0].asString() == "A" || values[0].asString() == "C") {
            m_regA = values[0].asString() == "A";
            return true;
        }
    } else if (propertyName == "suspendAfterResetForMs" && values[0].isInt()) {
        m_suspendPeriod = values[0].asInt();
        return true;
    }

    return false;
}


bool RkTapeInHook::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (CpuHook::setProperty(propertyName, values))
        return true;

    if (propertyName == "suspendAfterResetForMs" && values[0].isInt()) {
        m_suspendPeriod = values[0].asInt();
        return true;
    }

    return false;
}


string RkTapeInHook::getPropertyStringValue(const string& propertyName)
{
    string res;

    res = CpuHook::getPropertyStringValue(propertyName);
    if (res != "")
        return res;

    if (propertyName == "suspendAfterResetForMs") {
        stringstream stringStream;
        stringStream << m_suspendPeriod;
        stringStream >> res;
    }
    return res;
}
