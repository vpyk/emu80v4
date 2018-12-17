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

#include "Pal.h"

#include "CloseFileHook.h"
#include "Emulation.h"
#include "TapeRedirector.h"

using namespace std;

void CloseFileHook::addTapeRedirector(TapeRedirector* fr)
{
    m_frVector.push_back(fr);
    m_nFr++;
    m_frs = m_frVector.data();
}


bool CloseFileHook::hookProc()
{
    if (!m_isEnabled || m_hasSignature && !checkSignature())
        return false;

    for (int i=0; i<m_nFr; i++)
        m_frs[i]->closeFile();

    return false;
}


bool CloseFileHook::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (CpuHook::setProperty(propertyName, values))
        return true;

    if (propertyName == "addTapeRedirector") {
        addTapeRedirector(static_cast<TapeRedirector*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


ElapsedTimer::ElapsedTimer()
{
    pause();
}

void ElapsedTimer::start(unsigned ms)
{
    uint64_t curTime = g_emulation->getCurClock();
    m_curClock = curTime + ms * palGetCounterFreq() / 1000;
    resume();
}


void ElapsedTimer::stop()
{
    pause();
}


void ElapsedTimer::operate()
{
    pause();
    onElapse();
}


CloseFileTimer::CloseFileTimer(TapeRedirector* tr)
{
    m_tr = tr;
}


void CloseFileTimer::onElapse()
{
    m_tr->closeFile();
}
