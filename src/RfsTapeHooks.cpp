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

#include "Emulation.h"
#include "WavReader.h"
#include "RfsTapeHooks.h"
#include "Cpu8080.h"
#include "TapeRedirector.h"

using namespace std;


bool RfsTapeOutHeaderHook::hookProc()
{
    if (!m_isEnabled || (m_hasSignature && !checkSignature()))
        return false;

    if (m_file->isCancelled())
        return false;

    if (!m_file->isOpen())
        m_file->openFile();

    if (m_file->isCancelled())
        return false;

    static_cast<Cpu8080Compatible*>(m_cpu)->ret();

    return true;
}


bool RfsTapeOutHook::hookProc()
{
    if (!m_isEnabled || (m_hasSignature && !checkSignature()))
        return false;

    if (g_emulation->getWavReader()->isPlaying())
        return false;

    if (m_file->isCancelled())
        return false;

    if (!m_file->isOpen())
        return false;

    Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_cpu);

    uint8_t outByte = cpu->getBC() & 0xFF;

    m_file->writeByte(outByte);

    cpu->ret();

    return true;
}


bool RfsTapeInHeaderHook::hookProc()
{
    if (!m_isEnabled || (m_hasSignature && !checkSignature()))
        return false;

    if (g_emulation->getWavReader()->isPlaying())
        return false;

    if (m_file->isCancelled())
        return false;

    if (!m_file->isOpen())
        m_file->openFile();

    if (m_file->isCancelled())
        return false;

    Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_cpu);

    cpu->setAF(cpu->getAF() & 0xFFFE); // clear C flag
    cpu->ret();

    return true;
}


bool RfsTapeInHook::hookProc()
{
    if (!m_isEnabled || (m_hasSignature && !checkSignature()))
        return false;

    if (g_emulation->getWavReader()->isPlaying())
        return false;

    if (m_file->isCancelled())
        return false;

    Cpu8080Compatible* cpu = static_cast<Cpu8080Compatible*>(m_cpu);
    uint16_t flags = cpu->getAF() & 0x00FE;

    if (!m_file->isOpen() || m_file->isEof()) {
        //cpu->setAF(flags | 0x0001 | 0x3F00); / /return error
        cpu->setAF(flags); // just return 0 if error - workaround for truncated files w/o last 2 bytes
        cpu->ret();
        return true;
    }

    uint8_t inByte = 0;

    if (m_file)
        inByte = m_file->readByte();

    if (m_file->isCancelled())
        return false;

    cpu->setAF(flags | (inByte << 8));
    cpu->ret();
    return true;
}
