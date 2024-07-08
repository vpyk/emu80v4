# Файл настроек
@OPT_FILE = "orion/orion.opt"



######## Window ########

EmuWindow window
window.caption = @NAME

# Default window size for window.windowStyle = fixed (default = 800, 600)
window.defaultWindowSize = 800, 600

# Window style: autosize, resizable, fixed, fullscreen (default = autosise)
window.windowStyle = autosize
#window.windowStyle = resizable

# Window scaling option: 1x, 2x, 3x, 4x, 5x, fit, bestFit (default = bestFit)
window.frameScale = 2x
#window.frameScale = bestFit

# Window fields mixing: none, mix, interlace, scanline (default = none)
#window.fieldsMixing = mix

# Image smoothing (default = sharp)
window.smoothing = sharp

# Aspect correction (default = no)
window.aspectCorrection = no
#window.aspectCorrection = yes

# Wide screen (16:9): yes, no, custom (default = no)
window.wideScreen = no

# Screen format for window.wideScreen = custom
window.customScreenFormat = 1.111

# Square pixels mode when aspectCorrection = no
window.squarePixels = yes


    
######## Platform ########

platform.helpFile = @HELP_FILE

# Code page: rk, koi8 (default = rk)
platform.codePage = rk

# Fast reset
platform.fastReset = yes
platform.fastResetCpuTicks = 3000000



Ram ram0 = 0xF000
Ram ram1 = 0xF000
Ram ram2 = 0xF000
Ram ram3 = 0xF000

ifdef RAM_512K
  Ram ram4 = 0xF000
  Ram ram5 = 0xF000
  Ram ram6 = 0xF000
  Ram ram7 = 0xF000
endif

Ram sysRam = 0x400

ifndef RAM_512K
  AddrSpaceMapper ram = 4
else
  AddrSpaceMapper ram = 8
endif

ram.page = 0, &ram0
ram.page = 1, &ram1
ram.page = 2, &ram2
ram.page = 3, &ram3

ifdef RAM_512K
  ram.page = 4, &ram4
  ram.page = 5, &ram5
  ram.page = 6, &ram6
  ram.page = 7, &ram7
endif

Rom rom = @ROM, 0x800

Ppi8255 ppi1
Ppi8255 ppi2
Ppi8255 ppi3

OrionColorModeSelector colorModeSelector
OrionMemPageSelector pageSelector
pageSelector.mapper = &ram

ifndef RAM_512K
  pageSelector.bits = 2
else
  pageSelector.bits = 3
endif

OrionScreenSelector screenSelector

Fdc1793 fdc

OrionFddControlRegister fddReg
fddReg.fdc = &fdc
fddReg.type = standard

OrionFddControlRegister fddReg08
fddReg08.fdc = &fdc
fddReg08.type = spdos

OrionFddQueryRegister fddQReg
fddQReg.fdc = &fdc

ifdef HDD
  DiskImage hdd
  hdd.label = HDD
  hdd.filter = "Образы HDD DSDOS (*.ohi,*.hdd;*.img)|*.ohi,*.OHI,*.hdd;*.HDD;*.img;*.IMG|Все файлы (*.*)|*"

  ifdef HDD_IMAGE
    hdd.fileName = @HDD_IMAGE
  endif

  AtaDrive ataDrive
  ataDrive.geometry = lba
  ataDrive.diskImage = &hdd

  PpiAtaAdapter ataAdapter
  ataAdapter.ataDrive = &ataDrive
  ppi3.circuit = &ataAdapter
endif

ifdef EXT_ROMDISK
  RomDiskPageSelector romDiskPageSelector
  romDiskPageSelector.bits = 4
endif  

AddrSpace fddSpace
fddSpace.range = &fdc, 0x00, 0x03
fddSpace.range = &fddQReg, 0x04, 0x04
fddSpace.range = &fddReg08, 0x08, 0x08
fddSpace.range = &fdc, 0x10, 0x13
fddSpace.range = &fddReg, 0x14, 0x14
fddSpace.range = &fddReg, 0x20, 0x20

AddrSpace addrSpace
addrSpace.range = &ram, 0x0000, 0xEFFF
addrSpace.range = &sysRam, 0xF000, 0xF3FF
addrSpace.range = &ppi1, 0xF400, 0xF4FF
addrSpace.range = &ppi2, 0xF500, 0xF5FF
addrSpace.range = &ppi3, 0xF600, 0xF6FF
addrSpace.range = &fddSpace, 0xF700, 0xF7FF
addrSpace.writeRange = &colorModeSelector, 0xF800, 0xF8FF
addrSpace.writeRange = &pageSelector, 0xF900, 0xF9FF
addrSpace.writeRange = &screenSelector, 0xFA00, 0xFAFF
addrSpace.readRange = &rom, 0xF800, 0xFFFF

ifdef EXT_ROMDISK
  addrSpace.writeRange = &romDiskPageSelector, 0xFE00, 0xFEFF
endif  

OrionRenderer crtRenderer
crtRenderer.screenMemory = &ram0
crtRenderer.colorMemory = &ram1
screenSelector.crtRenderer = &crtRenderer
colorModeSelector.crtRenderer = &crtRenderer

GeneralSoundSource beepSoundSource

OrionCore core
core.window = &window
core.crtRenderer = &crtRenderer
core.beepSoundSource = &beepSoundSource

ifdef Z80
  CpuZ80 cpu
else
  Cpu8080 cpu
endif

cpu.frequency = @FREQUENCY
cpu.startAddr = 0xF800
cpu.addrSpace = &addrSpace
cpu.core = &core

GeneralSoundSource tapeSoundSource
#tapeSoundSource.muted = yes

RkKbdLayout kbdLayout
RkKeyboard keyboard

RkPpi8255Circuit ppiCircuit
ppiCircuit.rkKeyboard = &keyboard
ppiCircuit.tapeSoundSource = &tapeSoundSource

ppi1.circuit = &ppiCircuit

ifndef EXT_ROMDISK
  RkRomDisk romDisk = @ROM_DISK
else
  ExtRkRomDisk romDisk = @ROM_DISK
endif

ppi2.circuit = &romDisk

ifdef EXT_ROMDISK
  romDiskPageSelector.romDisk = &romDisk
endif



######## File I/O ########

OrionFileLoader loader
loader.addrSpace = &ram0
loader.ramDiskAddrSpace = &ram1
loader.filter = "Файлы Ориона (*.rko, *.bru, *.ord, *.ori)|*.rko;*.bru;*.ord;*.RKO;*.BRU;*.ORD;*.ORI|Все файлы (*.*)|*"

ifdef USE_TAPE
  TapeRedirector tapeInFile
  tapeInFile.mode = "r"
  tapeInFile.filter = "Файлы Ориона (*.rko)|*.rko;*.RKO|Файлы RK-совместимых ПК (*.rk?)|*.rk;*.rk?;*.RK;*.RK?|Файлы BSM (*.bsm)|*.bsm;*.BSM|Файлы EDM (*.edm)|*.edm;*.EDM|Все файлы (*.*)|*"

  TapeRedirector tapeOutFile
  tapeOutFile.mode = "w"
  tapeOutFile.filter = "Файлы Ориона (*.rko)|*.rko|Файлы RK-совместимых ПК (*.rk)|*.rk|Файлы BSM (*.bsm)|*.bsm|Файлы EDM (*.edm)|*.edm|Все файлы (*.*)|*"

  RkTapeOutHook tapeOutHook = @F80C_ADDR
  tapeOutHook.tapeRedirector = &tapeOutFile
  cpu.addHook = &tapeOutHook

  RkTapeInHook tapeInHook = @F806_ADDR
  tapeInHook.tapeRedirector = &tapeInFile
  cpu.addHook = &tapeInHook

  CloseFileHook closeFileHook = @F803_ADDR
  closeFileHook.addTapeRedirector = &tapeInFile
  closeFileHook.addTapeRedirector = &tapeOutFile
  cpu.addHook = &closeFileHook

  EmuObjectGroup tapeGrp
  tapeGrp.addItem = &tapeOutHook
  tapeGrp.addItem = &tapeInHook
  tapeGrp.addItem = &closeFileHook
  tapeGrp.enabled = yes
endif

FdImage diskA = 80, 2, 5, 1024
diskA.label = A
diskA.filter = "Образы дисков Ориона (*.odi)|*.odi;*.ODI|Все файлы (*.*)|*"
#diskA.fileName = "disk9.odi"

FdImage diskB = 80, 2, 5, 1024
diskB.label = B
diskB.filter = "Образы дисков Ориона (*.odi)|*.odi;*.ODI|Все файлы (*.*)|*"

fdc.fdImage = 0, &diskA
fdc.fdImage = 1, &diskB

RamDisk ramDisk = 3, 0xF000
ramDisk.filter = "Файлы RAM-диска Ориона (*.ramdisk)|*.ramdisk|Все файлы (*.*)|*"
ramDisk.page = 0, &ram1
ramDisk.page = 1, &ram2
ramDisk.page = 2, &ram3



######## Include Saved Options ########

include @OPT_FILE



######## Configuration Window ########

ConfigTab configTab = @NAME
configTab.optFileName = @OPT_FILE

#window.frameScale
ConfigRadioSelector scaleSelector = &window, frameScale, "Масштабирование"
scaleSelector.addItem = 1x, "1x"
scaleSelector.addItem = 2x, "2x"
scaleSelector.addItem = 3x, "3x"
scaleSelector.addItem = 4x, "4x"
scaleSelector.addItem = 5x, "5x"
scaleSelector.addItem = fit, "Растянуть во все окно"
scaleSelector.addItem = bestFit, "Растянуть с целым коэффициентом"
configTab.addControl = 1, &scaleSelector

#window.windowStyle
ConfigRadioSelector styleSelector = &window, windowStyle, "Размер окна"
styleSelector.addItem = autosize, "Автоматически подстраивать"
styleSelector.addItem = resizable, "Изменяемый пользователем"
styleSelector.addItem = fixed, "Фиксированный"
configTab.addControl = 1, &styleSelector

#window.fieldsMixing
ConfigRadioSelector mixSelector = &window, fieldsMixing, "Смешивание полукадров"
mixSelector.addItem = none, "Выкл."
mixSelector.addItem = scanline, "Имитация линий растра"
configTab.addControl = 2, &mixSelector

#window.smoothing
ConfigRadioSelector smSelector = &window, smoothing, "Сглаживание"
smSelector.addItem = nearest, "Выкл."
smSelector.addItem = bilinear, "Билинейное"
smSelector.addItem = sharp, "Границы пикселей (рекомендуется)"
configTab.addControl = 1, &smSelector

#window.aspectCorrection
ConfigRadioSelector arSelector = &window, aspectCorrection, "Учитывать Aspect Ratio"
arSelector.addItem = no, "Нет (1:1)"
arSelector.addItem = yes, "Оригинальный AR"
configTab.addControl = 2, &arSelector

#window.wideScreen
ConfigRadioSelector wsSelector = &window, wideScreen, "Формат экрана"
wsSelector.addItem = no, "4:3"
wsSelector.addItem = yes, "16:9"
wsSelector.addItem = custom, "Пользовательский"
configTab.addControl = 2, &wsSelector

#crtRenderer.visibleArea
ConfigRadioSelector cropSelector = &crtRenderer, visibleArea, "Отображать реальный бордюр"
cropSelector.addItem = no, "Нет"
cropSelector.addItem = yes, "Да"
configTab.addControl = 1, &cropSelector

#kbdLayout.layout
ConfigRadioSelector laSelector = &kbdLayout, layout, "Раскладка клавиатуры"
laSelector.addItem = qwerty, "Qwerty"
laSelector.addItem = jcuken, "Йцукен"
laSelector.addItem = smart, "«Умная» раскладка"
configTab.addControl = 2, &laSelector

#crtRenderer.colorMode
ConfigRadioSelector colorSelector = &crtRenderer, colorMode, "Режим цвета"
colorSelector.addItem = color, "Цветной"
colorSelector.addItem = mono, "Ч/б"
configTab.addControl = 2, &colorSelector

ifdef USE_TAPE
  ConfigRadioSelector tapeSelector = &tapeGrp, enabled, "Перехват магнитофона"
  tapeSelector.addItem = yes, "Вкл."
  tapeSelector.addItem = no, "Выкл."
  configTab.addControl = 3, &tapeSelector
endif

#platform.codePage
ConfigRadioSelector codePageSelector = &platform, codePage, "Таблица символов отладчика"
codePageSelector.addItem = rk, "РК-86"
codePageSelector.addItem = koi8, "КОИ-8"
configTab.addControl = 3, &codePageSelector

#platform.fastReset
ConfigRadioSelector fastResetSelector = &platform, fastReset, "Быстрый сброс"
fastResetSelector.addItem = yes, "Вкл."
fastResetSelector.addItem = no, "Выкл."
configTab.addControl = 3, &fastResetSelector
