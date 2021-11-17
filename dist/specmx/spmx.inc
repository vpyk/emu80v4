# Файл настроек
@optFile = "specmx/spmx.opt"



######## Window ########

EmuWindow window
window.caption = @name

# Default window size for window.windowStyle = fixed (default = 800, 600)
#window.defaultWindowSize = 768, 512
window.defaultWindowSize = 800, 600

# Window style: autosize, sizable, fixed, fullscreen (default = autosise)
window.windowStyle = autosize
#window.windowStyle = sizable

# Window scaling option: 1x, 2x, 3x, 4x, 5x, 2x3, 3x5, 4x6, fit, bestFit, fitKeepAR (default = bestFit)
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

# Wide screen (16:9): yes, no, custom (default = no)
window.wideScreen = no

# Screen format for window.wideScreen = custom
window.customScreenFormat = 1.111



######## Platform ########

platform.helpFile = @HELP_FILE

# Code page: rk, koi8 (default = rk)
platform.codePage = koi8

Ram ram0 = 0x9000
SpecVideoRam videoRam = 0x3000
Ram upperRam = 0x3FC0
Ram ramDisk1 = 0xFFC0
Ram ramDisk2 = 0xFFC0
Ram ramDisk3 = 0xFFC0
Ram ramDisk4 = 0xFFC0
Ram ramDisk5 = 0xFFC0
Ram ramDisk6 = 0xFFC0
Ram ramDisk7 = 0xFFC0
Ram ramDisk8 = 0xFFC0
Ram sysRam = 0x0020

Rom rom = @rom, 0xC000

AddrSpace ramPage
ramPage.range = &ram0, 0x0000, 0x8FFF
ramPage.range = &videoRam, 0x9000, 0xBFFF
ramPage.range = &upperRam, 0xC000, 0xFFBF

AddrSpace romPage
romPage.range = &rom, 0x0000, 0xBFFF
romPage.range = &upperRam, 0xC000, 0xFFBF

# 0 - ROM, 1 - RAM, 2..10 - RAM DIsks
AddrSpaceMapper lowerAddrSpace = 10
lowerAddrSpace.page = 0, &romPage
lowerAddrSpace.page = 1, &ramPage
lowerAddrSpace.page = 2, &ramDisk1
lowerAddrSpace.page = 3, &ramDisk2
lowerAddrSpace.page = 4, &ramDisk3
lowerAddrSpace.page = 5, &ramDisk4
lowerAddrSpace.page = 6, &ramDisk5
lowerAddrSpace.page = 7, &ramDisk6
lowerAddrSpace.page = 8, &ramDisk7
lowerAddrSpace.page = 9, &ramDisk8

Ppi8255 ppi1
Ppi8255 ppi2
Fdc1793 fdc

SpecMxFddControlRegisters fddRegs
fddRegs.fdc = &fdc

Pit8253 pit
pit.frequency = 2000000

SpecMxMemPageSelector pageSelector
pageSelector.mapper = &lowerAddrSpace

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
addrSpace.range = &sysRam, 0xFFC0, 0xFFDF
addrSpace.range = &dev, 0xFFE0, 0xFFFF

SpecRenderer crtRenderer
crtRenderer.screenMemory = &videoRam
crtRenderer.colorMode = mx

SpecCore core
core.window = &window
core.crtRenderer = &crtRenderer

ifdef Z80
  CpuZ80 cpu
else
  Cpu8080 cpu
endif

cpu.frequency = @FREQUENCY
cpu.startAddr = 0x0000
cpu.addrSpace = &addrSpace
cpu.core = &core

GeneralSoundSource tapeSoundSource
#tapeSoundSource.muted = yes

SpecMxPit8253SoundSource pitSoundSource
pitSoundSource.pit = &pit

SpecKbdLayout kbdLayout
SpecKeyboard keyboard
keyboard.matrix = @KBD

SpecPpi8255Circuit ppiCircuit
ppiCircuit.specKeyboard = &keyboard
ppiCircuit.tapeSoundSource = &tapeSoundSource
ppiCircuit.pitSoundSource = &pitSoundSource

ppi1.circuit = &ppiCircuit



######## File I/O ########

SpecMxFileLoader loader
loader.addrSpace = &addrSpace
loader.ramDiskAddrSpace = &ramDisk1
loader.pageMapper = &lowerAddrSpace
loader.filter = "Файлы Специалиста-MX (*.cpu)|*.cpu;*.CPU|Все файлы (*.*)|*"

FdImage diskA = 80, 2, 5, 1024
diskA.label = A
diskA.filter = "Образы дисков Спецмалиста (*.odi)|*.odi;*.ODI|Все файлы (*.*)|*"
#diskA.readOnly = yes

FdImage diskB = 80, 2, 5, 1024
diskB.label = B
diskB.filter = "Образы дисков Специалиста (*.odi)|*.odi;*.ODI|Все файлы (*.*)|*"

fdc.fdImage = 0, &diskA
fdc.fdImage = 1, &diskB

RamDisk ramDisk = 8, 0x10000
ramDisk.filter = "Файлы RAM-диска Специалиста-MX (*.ramdisk)|*.ramdisk|Все файлы (*.*)|*"
ramDisk.page = 0, &ramDisk1
ramDisk.page = 1, &ramDisk2
ramDisk.page = 2, &ramDisk3
ramDisk.page = 3, &ramDisk4
ramDisk.page = 4, &ramDisk5
ramDisk.page = 5, &ramDisk6
ramDisk.page = 6, &ramDisk7
ramDisk.page = 7, &ramDisk8


TapeRedirector rfsTapeOutFile
rfsTapeOutFile.mode = "w"
rfsTapeOutFile.filter = "Файлы RAMFOS (*.rfs)|*.rfs|Все файлы (*.*)|*"
rfsTapeOutFile.timeout = 1500

TapeRedirector rfsTapeInFile
rfsTapeInFile.mode = "r"
rfsTapeInFile.filter = "Файлы RAMFOS (*.rfs)|*.rfs;*.RFS|Все файлы (*.*)|*"
rfsTapeInFile.timeout = 3000

RfsTapeOutHook tapeOutHook = 0xC728
tapeOutHook.signature = E5C52A60C6C6
tapeOutHook.tapeRedirector = &rfsTapeOutFile
cpu.addHook = &tapeOutHook

RfsTapeOutHeaderHook tapeOutHeaderHook = 0xC70F
tapeOutHeaderHook.signature = E5C52A62C63E
tapeOutHeaderHook.tapeRedirector = &rfsTapeOutFile
cpu.addHook = &tapeOutHeaderHook

RfsTapeInHook tapeInHook = 0xC6BE
tapeInHook.signature = E5D5C52189CBE5
tapeInHook.tapeRedirector = &rfsTapeInFile
cpu.addHook = &tapeInHook

RfsTapeInHeaderHook tapeInHeaderHook = 0xC66C
tapeInHeaderHook.signature = E5D5C52189CBE5
tapeInHeaderHook.tapeRedirector = &rfsTapeInFile
cpu.addHook = &tapeInHeaderHook

CloseFileHook closeFileHook = 0xCE78
closeFileHook.signature = E5D5C5CDC4CECC
closeFileHook.addTapeRedirector = &rfsTapeInFile
closeFileHook.addTapeRedirector = &rfsTapeOutFile
cpu.addHook = &closeFileHook

EmuObjectGroup tapeGrp
tapeGrp.addItem = &tapeOutHook
tapeGrp.addItem = &tapeInHook
tapeGrp.addItem = &tapeOutHeaderHook
tapeGrp.addItem = &tapeInHeaderHook
tapeGrp.addItem = &closeFileHook



######## Include Saved Options ########

include @optFile



######## Configuration Window ########

ConfigTab configTab = @name
configTab.optFileName = @optFile

#window.frameScale
ConfigRadioSelector scaleSelector = &window, frameScale, "Масштабирование"
scaleSelector.addItem = 1x, "1x"
scaleSelector.addItem = 2x, "2x"
scaleSelector.addItem = 3x, "3x"
scaleSelector.addItem = 4x, "4x"
scaleSelector.addItem = 5x, "5x"
scaleSelector.addItem = 2x3, "2x3"
scaleSelector.addItem = 3x5, "3x5"
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
ConfigRadioSelector mixSelector = &window, fieldsMixing, "Смешивание полукадров"
mixSelector.addItem = none, "Выкл."
mixSelector.addItem = scanline, "Имитация линий растра"
configTab.addControl = 2, &mixSelector

#window.antialiasing
ConfigRadioSelector aaSelector = &window, antialiasing, "Сглаживание"
aaSelector.addItem = no, "Выкл."
aaSelector.addItem = yes, "Вкл."
configTab.addControl = 1, &aaSelector

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
configTab.addControl = 2, &cropSelector

#kayboard.matrix
ConfigRadioSelector kbdSelector = &keyboard, matrix, "Тип клавиатуры"
kbdSelector.addItem = original, "Оригинальная"
kbdSelector.addItem = ramfos, "RAMFOS"
configTab.addControl = 3, &kbdSelector

#kbdLayout.layout
ConfigRadioSelector laSelector = &kbdLayout, layout, "Раскладка клавиатуры"
laSelector.addItem = qwerty, "Qwerty"
laSelector.addItem = jcuken, "Йцукен"
laSelector.addItem = smart, "«Умная» раскладка"
configTab.addControl = 3, &laSelector

#tapeGrp.enabled
ConfigRadioSelector tapeSelector = &tapeGrp, enabled, "Перехват магнитофона"
tapeSelector.addItem = yes, "Вкл."
tapeSelector.addItem = no, "Выкл."
configTab.addControl = 3, &tapeSelector

#platform.codePage
ConfigRadioSelector codePageSelector = &platform, codePage, "Таблица символов отладчика"
codePageSelector.addItem = rk, "РК-86"
codePageSelector.addItem = koi8, "КОИ-8"
configTab.addControl = 3, &codePageSelector
