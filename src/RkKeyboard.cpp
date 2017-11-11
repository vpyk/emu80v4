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

#include "RkKeyboard.h"

using namespace std;

RkKeyboard::RkKeyboard()
{
    resetKeys();
}


void RkKeyboard::resetKeys()
{
    for (int i = 0; i < 8; i++)
        m_keys[i] = 0;
    m_mask = 0;
    m_ctrlKeys = 0;
}

void RkKeyboard::processKey(EmuKey key, bool isPressed)
{
    if (key == EK_NONE)
        return;

    int i, j;
    bool isFound = false;

    // Основная матрица
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            EmuKey tblKey;
            if (m_matrixKind != RKM_MIKROSHA)
                if (m_matrixKind != RKM_MIKRO80)
                    tblKey = m_keyMatrix[i][j];
                else
                    tblKey = m_keyMatrixMikro80[i][j];
            else
                tblKey = m_KeyMatrixMikrosha[j][i];
            if (key == tblKey) {
                isFound = true;
                break;
            }
        }
        if (isFound)
            break;
    }
    if (isFound) {
        if (isPressed)
            m_keys[i] |= (1 << j);
        else
            m_keys[i] &= ~(1 << j);
    return;
    }

    // Управляющие клавиши
    const EmuKey* keys;
    switch  (m_matrixKind) {
        case RKM_PARTNER:
            keys = m_ctrlKeyMatrixPartner;
            break;
        case RKM_MIKROSHA:
            keys = m_ctrlKeyMatrixMikrosha;
            break;
        case RKM_MIKRO80:
            keys = m_ctrlKeyMatrixMikro80;
            break;
        case RKM_RK:
        default:
            keys = m_ctrlKeyMatrixRk;
            break;
    }
    for (i = 0; i < 8; i++) {
        if (keys[i] == key) {
            isFound = true;
            break;
        }
    }

    if (isFound) {
        if (isPressed)
            m_ctrlKeys |= (1 << i);
        else
            m_ctrlKeys &= ~(1 << i);
    return;
    }

}



void RkKeyboard::setMatrixMask(uint8_t mask)
{
    m_mask = ~mask;
}



uint8_t RkKeyboard::getMatrixData()
{
    uint8_t val = 0;
    uint8_t mask = m_mask;
    for (int i=0; i<8; i++) {
        if (mask & 1)
            val |= m_keys[i];
        mask >>= 1;
    }

    // !!!
    if (m_matrixKind == RKM_MIKRO80)
        val |= 0x80;

    return ~val;
}



uint8_t RkKeyboard::getCtrlKeys()
{
    return ~m_ctrlKeys;
}


bool RkKeyboard::setProperty(const string& propertyName, const EmuValuesList& values)
{
    if (EmuObject::setProperty(propertyName, values))
        return true;

    if (propertyName == "matrix") {
        if (values[0].asString() == "rk") {
            m_matrixKind = RKM_RK;
            return true;
        } else if (values[0].asString() == "partner") {
            m_matrixKind = RKM_PARTNER;
            return true;
        } else if (values[0].asString() == "mikrosha") {
            m_matrixKind = RKM_MIKROSHA;
            return true;
        } else if (values[0].asString() == "mikro80") {
            m_matrixKind = RKM_MIKRO80;
            return true;
        } else
            return false;
    }
    return false;
}
