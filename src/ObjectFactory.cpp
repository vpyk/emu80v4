/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2018
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

#include "ObjectFactory.h"

#include "EmuObjects.h"
#include "EmuWindow.h"
#include "AddrSpace.h"
#include "Memory.h"
#include "Cpu8080.h"
#include "CpuZ80.h"
#include "Ppi8255.h"
#include "Dma8257.h"
#include "Crt8275.h"
#include "Pit8253.h"
#include "Fdc1793.h"
#include "Pit8253Sound.h"
#include "Orion.h"
#include "Specialist.h"
#include "Eureka.h"
#include "Mikro80.h"
#include "Ut88.h"
#include "RkKeyboard.h"
#include "KbdLayout.h"
#include "RkPpi8255Circuit.h"
#include "Mikrosha.h"
#include "Partner.h"
#include "Apogey.h"
#include "Rk86.h"
#include "RkFdd.h"
#include "FileLoader.h"
#include "TapeRedirector.h"
#include "FdImage.h"
#include "RkTapeHooks.h"
#include "MsxTapeHooks.h"
#include "CloseFileHook.h"
#include "RkRomDisk.h"
#include "RkSdController.h"
#include "RamDisk.h"
#include "GenericModules.h"
#include "Pk8000.h"
#include "Psg3910.h"
#include "PpiAtaAdapter.h"
#include "AtaDrive.h"

#include "EmuConfig.h"

using namespace std;

#define REG_EMU_CLASS(name) reg(#name, &name::create);

ObjectFactory::ObjectFactory()
{
    //REG_EMU_CLASS(Platform)
    REG_EMU_CLASS(EmuWindow)
    REG_EMU_CLASS(EmuObjectGroup)
    REG_EMU_CLASS(AddrSpace)
    REG_EMU_CLASS(AddrSpaceMapper)
    REG_EMU_CLASS(AddrSpaceShifter)
    REG_EMU_CLASS(AddrSpaceInverter)
    REG_EMU_CLASS(Ram)
    REG_EMU_CLASS(Rom)
    REG_EMU_CLASS(NullSpace)
    REG_EMU_CLASS(Cpu8080)
    REG_EMU_CLASS(Cpu8080StatusWordSpace)
    REG_EMU_CLASS(CpuZ80)
    REG_EMU_CLASS(Ppi8255)
    REG_EMU_CLASS(Dma8257)
    REG_EMU_CLASS(Crt8275)
    REG_EMU_CLASS(Pit8253)
    REG_EMU_CLASS(Fdc1793)
    REG_EMU_CLASS(Pit8253SoundSource)
    REG_EMU_CLASS(RkPit8253SoundSource)
    //REG_EMU_CLASS(MikroshaPit8253SoundSource)
    REG_EMU_CLASS(OrionMemPageSelector)
    REG_EMU_CLASS(OrionRenderer)
    REG_EMU_CLASS(OrionScreenSelector)
    REG_EMU_CLASS(OrionColorModeSelector)
    REG_EMU_CLASS(OrionFddControlRegister)
    REG_EMU_CLASS(OrionFddQueryRegister)
    REG_EMU_CLASS(OrionCore)
    REG_EMU_CLASS(OrionFileLoader)
    REG_EMU_CLASS(SpecMxMemPageSelector)
    REG_EMU_CLASS(SpecVideoRam)
    REG_EMU_CLASS(SpecMxColorRegister)
    REG_EMU_CLASS(SpecRenderer)
    REG_EMU_CLASS(SpecCore)
    REG_EMU_CLASS(SpecMxFddControlRegisters)
    REG_EMU_CLASS(SpecKeyboard)
    REG_EMU_CLASS(SpecPpi8255Circuit)
    REG_EMU_CLASS(SpecRomDisk)
    REG_EMU_CLASS(SpecFileLoader)
    REG_EMU_CLASS(SpecMxFileLoader)
    REG_EMU_CLASS(EurekaCore)
    REG_EMU_CLASS(EurekaRenderer)
    REG_EMU_CLASS(EurekaPpi8255Circuit)
    REG_EMU_CLASS(Mikro80Renderer)
    REG_EMU_CLASS(Mikro80Core)
    REG_EMU_CLASS(Mikro80TapeRegister)
    REG_EMU_CLASS(Ut88Renderer)
    REG_EMU_CLASS(Ut88Core)
    REG_EMU_CLASS(Ut88AddrSpaceMapper)
    REG_EMU_CLASS(Ut88MemPageSelector)
    REG_EMU_CLASS(RkKeyboard)
    REG_EMU_CLASS(RkKbdLayout)
    REG_EMU_CLASS(RkPpi8255Circuit)
    REG_EMU_CLASS(MikroshaCore)
    REG_EMU_CLASS(MikroshaPpi8255Circuit)
    REG_EMU_CLASS(MikroshaPpi2Circuit)
    REG_EMU_CLASS(MikroshaRenderer)
    REG_EMU_CLASS(PartnerAddrSpace)
    REG_EMU_CLASS(PartnerAddrSpaceSelector)
    REG_EMU_CLASS(PartnerModuleSelector)
    REG_EMU_CLASS(PartnerMcpgSelector)
    REG_EMU_CLASS(PartnerRamUpdater)
    REG_EMU_CLASS(PartnerCore)
    REG_EMU_CLASS(PartnerPpi8255Circuit)
    REG_EMU_CLASS(PartnerRenderer)
    REG_EMU_CLASS(PartnerMcpgRenderer)
    REG_EMU_CLASS(PartnerFddControlRegister)
    REG_EMU_CLASS(ApogeyCore)
    REG_EMU_CLASS(ApogeyRenderer)
    REG_EMU_CLASS(ApogeyRomDisk)
    REG_EMU_CLASS(Rk86Core)
    REG_EMU_CLASS(Rk86Renderer)
    REG_EMU_CLASS(RkFddRegister)
    REG_EMU_CLASS(RkFddController)
    REG_EMU_CLASS(RkFileLoader)
    REG_EMU_CLASS(TapeRedirector)
    REG_EMU_CLASS(FdImage)
    REG_EMU_CLASS(RkTapeOutHook)
    REG_EMU_CLASS(RkTapeInHook)
    REG_EMU_CLASS(MsxTapeOutHook)
    REG_EMU_CLASS(MsxTapeOutHeaderHook)
    REG_EMU_CLASS(MsxTapeInHook)
    REG_EMU_CLASS(MsxTapeInHeaderHook)
    REG_EMU_CLASS(CloseFileHook)
    REG_EMU_CLASS(RkRomDisk)
    REG_EMU_CLASS(RkSdController)
    REG_EMU_CLASS(RamDisk)
    REG_EMU_CLASS(PeriodicInt8080)
    REG_EMU_CLASS(PageSelector)
    REG_EMU_CLASS(Splitter)
    REG_EMU_CLASS(Translator)
    REG_EMU_CLASS(Pk8000Renderer)
    REG_EMU_CLASS(Pk8000Core)
    REG_EMU_CLASS(Pk8000FileLoader)
    REG_EMU_CLASS(Pk8000ColorSelector)
    REG_EMU_CLASS(Pk8000TxtBufSelector)
    REG_EMU_CLASS(Pk8000SymGenBufSelector)
    REG_EMU_CLASS(Pk8000GrBufSelector)
    REG_EMU_CLASS(Pk8000ColBufSelector)
    REG_EMU_CLASS(Pk8000Keyboard)
    REG_EMU_CLASS(Pk8000Ppi8255Circuit1)
    REG_EMU_CLASS(Pk8000Ppi8255Circuit2)
    REG_EMU_CLASS(Pk8000Mode1ColorMem)
    REG_EMU_CLASS(Pk8000InputRegister)
    REG_EMU_CLASS(Pk8000KbdLayout)
    REG_EMU_CLASS(Pk8000FddControlRegister)
    REG_EMU_CLASS(Pk8000FdcStatusRegisters)
    REG_EMU_CLASS(Psg3910)
    REG_EMU_CLASS(Psg3910SoundSource)
    REG_EMU_CLASS(Pk8000CpuWaits)
    REG_EMU_CLASS(PpiAtaAdapter)
    REG_EMU_CLASS(AtaDrive)

    reg("ConfigTab", &EmuConfigTab::create);
    reg("ConfigRadioSelector", &EmuConfigRadioSelector::create);

    reg("RkKeybLayout", &RkKbdLayout::create); // compatibility issue, old configs may contain rkKeybLayout instead of rkKbdLayout
}


void ObjectFactory::reg(const string& objectClassName, CreateObjectFunc pfnCreate)
{
    m_objectMap[objectClassName] = pfnCreate;
}


EmuObject* ObjectFactory::createObject(const string& objectClassName, const EmuValuesList& parameters)
{
    auto it = m_objectMap.find(objectClassName);
    if (it != m_objectMap.end())
        return it->second(parameters);
    return nullptr;
}
