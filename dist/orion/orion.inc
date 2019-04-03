# Файл настроек
@OPT_FILE = "orion/orion.opt"



######## Window ########

EmuWindow window
window.caption = @NAME

# Default window size for window.windowStyle = fixed (default = 800, 600)
window.defaultWindowSize = 800, 600

# Window style: autosize, sizable, fixed, fullscreen (default = autosise)
window.windowStyle = autosize
#window.windowStyle = sizable

# Window scaling option: 1x, 2x, 3x, fit, bestFit, fitKeepAR (default = bestFit)
window.frameScale = 2x
#window.frameScale = fitKeepAR
#window.frameScale = bestFit

# Window fields mixing: none, mix, interlace, scanline (default = none)
#window.fieldsMixing = mix

# Image antialiasing. Overrides system property (default = no)
window.antialiasing = no

# Aspect correction (default = no)
window.aspectCorrection = no
#window.aspectCorrection = yes

# Wide screen (16:9) (default = no)
window.wideScreen = no


	
######## Platform ########

platform.helpFile = @HELP_FILE

Ram ram0 = 0xF000
Ram ram1 = 0xF000
Ram ram2 = 0xF000
Ram ram3 = 0xF000
Ram sysRam = 0x400

AddrSpaceMapper ram = 4
ram.page = 0, &ram0
ram.page = 1, &ram1
ram.page = 2, &ram2
ram.page = 3, &ram3

Rom rom = @ROM, 0x800

Ppi8255 ppi1
Ppi8255 ppi2

OrionColorModeSelector colorModeSelector
OrionMemPageSelector pageSelector
pageSelector.mapper = &ram

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
addrSpace.range = &fddSpace, 0xF700, 0xF7FF
addrSpace.writeRange = &colorModeSelector, 0xF800, 0xF8FF
addrSpace.writeRange = &pageSelector, 0xF900, 0xF9FF
addrSpace.writeRange = &screenSelector, 0xFA00, 0xFAFF
addrSpace.readRange = &rom, 0xF800, 0xFFFF

OrionRenderer crtRenderer
crtRenderer.screenMemory = &ram0
crtRenderer.colorMemory = &ram1
screenSelector.crtRenderer = &crtRenderer
colorModeSelector.crtRenderer = &crtRenderer

OrionCore core
core.window = &window
core.crtRenderer = &crtRenderer

ifdef Z80
  CpuZ80 cpu
else
  Cpu8080 cpu
endif

cpu.frequency = @FREQUENCY
cpu.startAddr = 0xF800
cpu.addrSpace = &addrSpace
cpu.core = &core

RkKbdLayout kbdLayout
RkKeyboard keyboard

RkPpi8255Circuit ppiCircuit
ppiCircuit.rkKeyboard = &keyboard
ppi1.circuit = &ppiCircuit

RkRomDisk romDisk = @ROM_DISK
ppi2.circuit = &romDisk



######## File I/O ########

OrionFileLoader loader
loader.addrSpace = &ram0
loader.ramDiskAddrSpace = &ram1
loader.filter = "Файлы Ориона (*.rko, *.bru, *.ord)|*.rko;*.bru;*.ord;*.RKO;*.BRU;*.ORD|Все файлы (*.*)|*"

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
