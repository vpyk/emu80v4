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

// Ppi8255CircuitRk.cpp
// Реализация класса обвязки основного КР580ВВ55 на РК86 и Апогее

#include "Emulation.h"
#include "Platform.h"
#include "PlatformCore.h"
#include "SoundMixer.h"
#include "RkKeyboard.h"
#include "Ppi8255Circuit.h"
#include "RkPpi8255Circuit.h"
#include "WavReader.h"

using namespace std;


RkPpi8255Circuit::RkPpi8255Circuit()
{
    m_tapeSoundSource = new GeneralSoundSource;
    //g_emulation->getSoundMixer()->addSoundSource(m_tapeSoundSource);
}



RkPpi8255Circuit::~RkPpi8255Circuit()
{
    delete m_tapeSoundSource;
}



uint8_t RkPpi8255Circuit::getPortA()
{
    return 0xFF;
}



uint8_t RkPpi8255Circuit::getPortB()
{
    return m_kbd->getMatrixData();
}



uint8_t RkPpi8255Circuit::getPortC()
{
    return (m_kbd->getCtrlKeys() & 0xEF) | (g_emulation->getWavReader()->getCurValue() ? 0x10 : 0x00);
}



void RkPpi8255Circuit::setPortA(uint8_t value)
{
    m_kbd->setMatrixMask(value);
}



void RkPpi8255Circuit::setPortC(uint8_t value)
{
    m_tapeSoundSource->setValue(value & 1);
    m_platform->getCore()->tapeOut(value & 1);
}



void RkPpi8255Circuit::attachRkKeyboard(RkKeyboard* kbd)
{
    m_kbd = kbd;
}


bool RkPpi8255Circuit::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "rkKeyboard") {
        attachRkKeyboard(static_cast<RkKeyboard*>(g_emulation->findObject(values[0].asString())));
        return true;
    }

    return false;
}
