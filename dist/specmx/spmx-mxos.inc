# Тактовая частота
@FREQUENCY = 2000000

# Раскладка клавиатуры
@KBD = "original"

# Файл настроек
@OPT_FILE = "specmx/spmx-mxos.opt"



######## Window ########

EmuWindow window
window.caption = @NAME

# Default window size for window.windowStyle = fixed (default = 800, 600)
#window.defaultWindowSize = 768, 512
window.defaultWindowSize = 800, 600

# Window style: autosize, sizable, fixed, fullscreen (default = autosise)
window.windowStyle = fixed
#window.windowStyle = sizable

# Window scaling option: 1x, 2x, 3x, fit, bestFit, fitKeepAR (default = bestFit)
window.frameScale = 2x
#window.frameScale = fitKeepAR
#window.frameScale = bestFit

# Window fields mixing: none, mix, interlace, scanline (default = none)
#window.fieldsMixing = mix

# Image antialiasing (default = no)
window.antialiasing = no

# Aspect correction (default = no)
window.aspectCorrection = no
#window.aspectCorrection = yes



######## Platform ########

Ram ram0 = 0x9000
SpecVideoRam videoRam = 0x3000
Ram upperRam = 0x3FC0
Ram ramDisk1 = 0xFFC0
ifndef RAMDISK_1BANK
  Ram ramDisk2 = 0xFFC0
  Ram ramDisk3 = 0xFFC0
  Ram ramDisk4 = 0xFFC0
  Ram ramDisk5 = 0xFFC0
  Ram ramDisk6 = 0xFFC0
  Ram ramDisk7 = 0xFFC0
  Ram ramDisk8 = 0xFFC0
endif
Ram sysRam = 0x0020

Rom rom = @ROM, 0xC000

AddrSpace ramPage
ramPage.range = &ram0, 0x0000, 0x8FFF
ramPage.range = &videoRam, 0x9000, 0xBFFF
ramPage.range = &upperRam, 0xC000, 0xFFBF

AddrSpace romPage
romPage.range = &rom, 0x0000, 0xBFFF
romPage.range = &upperRam, 0xC000, 0xFFBF

# 0 - ROM, 1 - RAM, 2..10 - RAM DIsks
ifdef RAMDISK_1BANK
  AddrSpaceMapper lowerAddrSpace = 10
else
  AddrSpaceMapper lowerAddrSpace = 3
endif

lowerAddrSpace.page = 0, &romPage
lowerAddrSpace.page = 1, &ramPage
lowerAddrSpace.page = 2, &ramDisk1

ifndef RAMDISK_1BANK
  lowerAddrSpace.page = 3, &ramDisk2
  lowerAddrSpace.page = 4, &ramDisk3
  lowerAddrSpace.page = 5, &ramDisk4
  lowerAddrSpace.page = 6, &ramDisk5
  lowerAddrSpace.page = 7, &ramDisk6
  lowerAddrSpace.page = 8, &ramDisk7
  lowerAddrSpace.page = 9, &ramDisk8
endif


Ppi8255 ppi1
Ppi8255 ppi2
Fdc1793 fdc

SpecMxFddControlRegisters fddRegs
fddRegs.fdc = &fdc

Pit8253 pit
pit.frequency = 2000000
RkPit8253SoundSource sndSource
sndSource.pit = &pit

SpecMxMemPageSelector pageSelector
pageSelector.mapper = &lowerAddrSpace
ifdef RAMDISK_1BANK
  pageSelector.mode = 1bank
endif

SpecMxColorRegister colorReg
colorReg.videoRam = &videoRam

AddrSpace dev
dev.range = &ppi1, 0x00, 0x03
dev.range = &ppi2, 0x04, 0x07
dev.range = &fdc, 0x08, 0x0B
dev.range = &pit, 0x0C, 0x0F
dev.range = &fddRegs, 0x10, 0x13
dev.range = &colorReg, 0x18, 0x18
dev.range = &pageSelector, 0x1C, 0x1F

AddrSpace addrSpace 
addrSpace.range = &lowerAddrSpace, 0x0000, 0xFFBF
addrSpace .range = &sysRam, 0xFFC0, 0xFFDF
addrSpace .range = &dev, 0xFFE0, 0xFFFF


SpecRenderer crtRenderer
crtRenderer.screenMemory = &videoRam
crtRenderer.colorMode = mx

SpecCore core
core.window = &window
core.crtRenderer = &crtRenderer

Cpu8080 cpu
cpu.frequency = @FREQUENCY
cpu.startAddr = 0x0000
cpu.addrSpace = &addrSpace
cpu.core = &core

RkKeybLayout kbdLayout
SpecKeyboard keyboard
keyboard.matrix = @KBD

SpecPpi8255Circuit ppiCircuit
ppiCircuit.specKeyboard = &keyboard
ppi1.circuit = &ppiCircuit

RkRomDisk romDisk = @EXT_ROM
ppi2.circuit = &romDisk



######## File I/O ########

#SpecMxFileLoader loader
#loader.addrSpace = &ram0
#loader.ramDiskAddrSpace = &ramDisk1
#loader.filter = "Файлы Специалиста-MX (*.cpu)|*.cpu;*.CPU|Все файлы (*.*)|*"

FdImage diskA = 80, 2, 5, 1024
diskA.label = A
diskA.filter = "Образы дисков Спецмалиста (*.odi)|*.odi;*.ODI|Все файлы (*.*)|*"
diskA.readOnly = yes

FdImage diskB = 80, 2, 5, 1024
diskB.label = B
diskB.filter = "Образы дисков Специалиста (*.odi)|*.odi;*.ODI|Все файлы (*.*)|*"

fdc.fdImage = 0, &diskA
fdc.fdImage = 1, &diskB




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
scaleSelector.addItem = fit, "Растянуть во все окно"
scaleSelector.addItem = fitKeepAR, "Растянуть с сохранением пропорций"
scaleSelector.addItem = bestFit, "Растянуть с целым коэффициентом"
configTab.addControl = 1, &scaleSelector

#window.windowStyle
ConfigRadioSelector styleSelector = &window, windowStyle, "Размер окна"
styleSelector.addItem = autosize, "Автоматически подстраивать"
styleSelector.addItem = sizable, "Изменяемый пользователем"
styleSelector.addItem = fixed, "Фиксированный"
configTab.addControl = 1, &styleSelector

#window.fieldsMixing
#ConfigRadioSelector mixSelector = &window, fieldsMixing, "Смешивание полукадров"
#mixSelector.addItem = none, "Выкл."
#mixSelector.addItem = mix, "Вкл."
#configTab.addControl = 2, &mixSelector

#window.aspectCorrection
ConfigRadioSelector arSelector = &window, aspectCorrection, "Учитывать Aspect Ratio"
arSelector.addItem = no, "Нет (1:1)"
arSelector.addItem = yes, "Оригинальный AR"
configTab.addControl = 2, &arSelector

#window.antialiasing
ConfigRadioSelector aaSelector = &window, antialiasing, "Сглаживание"
aaSelector.addItem = no, "Выкл."
aaSelector.addItem = yes, "Вкл."
configTab.addControl = 1, &aaSelector

#kayboard.matrix
ConfigRadioSelector kbdSelector = &keyboard, matrix, "Тип клавиатуры"
kbdSelector.addItem = original, "Оригинальная"
kbdSelector.addItem = ramfos, "RAMFOS"
configTab.addControl = 2, &kbdSelector

#kbdLayout.layout
ConfigRadioSelector laSelector = &kbdLayout, layout, "Раскладка клавиатуры"
laSelector.addItem = qwerty, "Qwerty"
laSelector.addItem = jcuken, "Йцукен"
laSelector.addItem = smart, "«Умная» раскладка"
configTab.addControl = 2, &laSelector
