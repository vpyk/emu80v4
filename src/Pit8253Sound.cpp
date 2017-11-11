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

#include "Pit8253.h"
#include "Pit8253Sound.h"

#include "Emulation.h"
#include "Platform.h"
#include "Globals.h"

using namespace std;

void Pit8253SoundSource::attachPit(Pit8253* pit)
{
    m_pit = pit;
    pit->getCounter(2)->setExtClockMode(true);
}


int Pit8253SoundSource::calcValue()
{
    int res = 0;

    if (m_pit) {
        m_pit->updateState();
        for (int i = 0; i < 3; i++) {
            res += m_pit->getCounter(i)->getAvgOut();
            m_pit->getCounter(i)->resetStats();
            //res += m_pit->getOut(i) ? 4096 : 0;
        }
    }

    return res;
}


bool Pit8253SoundSource::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "pit") {
        attachPit(static_cast<Pit8253*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}


int RkPit8253SoundSource::calcValue()
{
    int res = 0;

    if (m_pit) {
        m_pit->getCounter(0)->updateState();
        m_pit->getCounter(1)->updateState();

        //m_pit->getCounter(2)->operateForTicks(m_pit->getCounter(1)->getSumOutTicks());
        int t = m_pit->getCounter(1)->getSumOutTicks();
        Pit8253Counter* cnt = m_pit->getCounter(2);
        cnt->operateForTicks(t);

        if (!m_pit->getCounter(2)->getOut())
            res += m_pit->getCounter(0)->getAvgOut();

        m_pit->getCounter(0)->resetStats();
        m_pit->getCounter(1)->resetStats();
        m_pit->getCounter(2)->resetStats();
    }

    return res;
}
