# Файл справки
@HELP_FILE = mikro80/mikro80.html

# Файл ROM-диска
ifndef CFG_ROMDISK_FILE
  @CFG_ROMDISK_FILE = "mikro80/romdisk.bin"
endif


######## Window ########

EmuWindow window
window.caption = @NAME

# Default window size for window.windowStyle = fixed (default = 800, 600)
window.defaultWindowSize = 768, 640

# Window style: autosize, resizable, fixed, fullscreen (default = autosise)
window.windowStyle = autosize
#window.windowStyle = resizable

# Window scaling option: 1x, 2x, 3x, 4x, 5x, fit, bestFit (default = bestFit)
window.frameScale = 2x
#window.frameScale = bestFit

# Window fields mixing: none, mix, average, interlace, scanline (default = none)
#window.fieldsMixing = mix

# Image smoothing / post-processing: nearest, bilinear, sharp, custom (default = sharp)
window.smoothing = sharp

# Custom image shader: "shader name" or none (default = none)
#window.shader = crt-hyllian

# Convert image to grayscale
#window.desaturate = yes

# Aspect correction (default = no)
window.aspectCorrection = no
#window.aspectCorrection = yes

# Wide screen (16:9): yes, no, custom (default = no)
window.wideScreen = no

# Gray window background (default: no)
#window.grayBackground = yes

# Screen format for window.wideScreen = custom
window.customScreenFormat = 1.111



######## Platform ########

platform.helpFile = @HELP_FILE

# Code page: rk, koi8 (default = rk)
platform.codePage = rk

Ram ram1 = 0xe000
Ram videoRam = 0x1000
Ram ram2 = 0x800

Rom rom = @ROM, 0x800


AddrSpace mainPage = 0x10000

Ram ramDiskPage0 = 0x10000
Ram ramDiskPage1 = 0x10000
Ram ramDiskPage2 = 0x10000
Ram ramDiskPage3 = 0x10000
Ram ramDiskPage4 = 0x10000
Ram ramDiskPage5 = 0x10000
Ram ramDiskPage6 = 0x10000
Ram ramDiskPage7 = 0x10000

Cpu8080 cpu

Ut88AddrSpaceMapper addrSpace = 9
addrSpace.cpu = &cpu
addrSpace.page = 0, &mainPage
addrSpace.page = 1, &ramDiskPage0
addrSpace.page = 2, &ramDiskPage1
addrSpace.page = 3, &ramDiskPage2
addrSpace.page = 4, &ramDiskPage3
addrSpace.page = 5, &ramDiskPage4
addrSpace.page = 6, &ramDiskPage5
addrSpace.page = 7, &ramDiskPage6
addrSpace.page = 8, &ramDiskPage7

Ut88MemPageSelector pageSelector
pageSelector.pages = 9
pageSelector.mapper = &addrSpace

mainPage.range =      &ram1,     0x0000, 0xDFFF
mainPage.range =      &videoRam, 0xE000, 0xEFFF
mainPage.range =      &ram2,     0xF000, 0xF7FF
mainPage.range =      &rom,      0xF800, 0xFFFF

#mainPage.poke = 0xF75C, 0x54
#mainPage.poke = 0xF75D, 0x38
mainPage.poke = 0xF75C, 0x4B
mainPage.poke = 0xF75D, 0x32

Ppi8255 ppi
AddrSpaceInverter invertedPpi = &ppi

Ppi8255 ppi2

RkRomDisk romDisk = @CFG_ROMDISK_FILE
ppi2.circuit = &romDisk

GeneralSoundSource tapeSoundSource
#tapeSoundSource.muted = yes

Mikro80TapeRegister tapeReg
tapeReg.tapeSoundSource = &tapeSoundSource

AddrSpace ioAddrSpace
ioAddrSpace.range = &tapeReg, 0x01, 0x01
ioAddrSpace.range = &invertedPpi, 0x04, 0x07
ioAddrSpace.range = &pageSelector, 0x40, 0x4F
ioAddrSpace.range = &ppi2, 0xA0, 0xA3

Mikro80Renderer crtRenderer
crtRenderer.screenMemory = &videoRam
crtRenderer.font = @SYM_GEN
crtRenderer.altFont = @FONT

Mikro80Core core
core.window = &window
core.crtRenderer = &crtRenderer

cpu.frequency = @CPU_FREQUENCY
cpu.startAddr = 0xF800
cpu.addrSpace = &addrSpace
cpu.ioAddrSpace = &ioAddrSpace
cpu.core = &core

RkKbdLayout kbdLayout
RkKeyboard keyboard
keyboard.matrix = "mikro80"

RkPpi8255Circuit ppiCircuit
ppiCircuit.rkKeyboard = &keyboard
ppi.circuit = &ppiCircuit

RamDisk ramDisk = 8, 0x10000
ramDisk.filter = "Файлы RAM-диска Микро-80 (*.ramdisk)|*.ramdisk|Все файлы (*.*)|*"
ramDisk.page = 0, &ramDiskPage0
ramDisk.page = 1, &ramDiskPage1
ramDisk.page = 2, &ramDiskPage2
ramDisk.page = 3, &ramDiskPage3
ramDisk.page = 4, &ramDiskPage4
ramDisk.page = 5, &ramDiskPage5
ramDisk.page = 6, &ramDiskPage6
ramDisk.page = 7, &ramDiskPage7


######## File I/O ########

RkFileLoader loader
loader.addrSpace = &addrSpace
loader.filter = "Файлы Микро-80 (*.rk8)|*.rk8;*.RK8|Файлы RK-совместимых ПК (*.rk?)|*.rk;*.rk?;*.RK;*.RK?|Все файлы (*.*)|*"
loader.skipTicks = 7000000

TapeRedirector tapeInFile
tapeInFile.mode = "r"
tapeInFile.filter = "Файлы Микро-80 (*.rk8)|*.rk8;*.RK8|Файлы RK-совместимых ПК (*.rk?)|*.rk;*.rk?;*.RK;*.RK?|Все файлы (*.*)|*"

TapeRedirector tapeOutFile
tapeOutFile.mode = "w"
tapeOutFile.filter = "Файлы Микро-80 (*.rk8)|*.rk8|Файлы RK-совместимых ПК (*.rk)|*.rk|Все файлы (*.*)|*"

RkTapeOutHook tapeOutHook = @C80C_ADDR
tapeOutHook.outReg = C
tapeOutHook.tapeRedirector = &tapeOutFile
cpu.addHook = &tapeOutHook

RkTapeInHook tapeInHook = @C806_ADDR
tapeInHook.tapeRedirector = &tapeInFile
#tapeInHook.enabled = no
cpu.addHook = &tapeInHook

CloseFileHook closeFileHook = @C803_ADDR
closeFileHook.addTapeRedirector = &tapeInFile
closeFileHook.addTapeRedirector = &tapeOutFile
cpu.addHook = &closeFileHook

EmuObjectGroup tapeGrp
tapeGrp.addItem = &tapeOutHook
tapeGrp.addItem = &tapeInHook
tapeGrp.addItem = &closeFileHook
tapeGrp.enabled = yes



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

#crtRenderer.altRenderer
ConfigRadioSelector rendSelector = &crtRenderer, altRenderer, "Шрифт"
rendSelector.addItem = no, "Оригинальный шрифт"
rendSelector.addItem = yes, "8×16"
configTab.addControl = 2, &rendSelector

#kbdLayout.layout
ConfigRadioSelector laSelector = &kbdLayout, layout, "Раскладка клавиатуры"
laSelector.addItem = qwerty, "Qwerty"
laSelector.addItem = jcuken, "Йцукен"
laSelector.addItem = smart, "«Умная» раскладка"
configTab.addControl = 3, &laSelector

ConfigRadioSelector tapeSelector = &tapeGrp, enabled, "Перехват магнитофона"
tapeSelector.addItem = yes, "Вкл."
tapeSelector.addItem = no, "Выкл."
configTab.addControl = 3, &tapeSelector

#platform.codePage
ConfigRadioSelector codePageSelector = &platform, codePage, "Таблица символов отладчика"
codePageSelector.addItem = rk, "РК-86"
codePageSelector.addItem = koi8, "КОИ-8"
configTab.addControl = 3, &codePageSelector
