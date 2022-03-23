# Файл справки
@HELP_FILE = mikro80/mikro80.html


######## Window ########

EmuWindow window
window.caption = @NAME

# Default window size for window.windowStyle = fixed (default = 800, 600)
window.defaultWindowSize = 768, 640

# Window style: autosize, sizable, fixed, fullscreen (default = autosise)
window.windowStyle = autosize
#window.windowStyle = sizable

# Window scaling option: 1x, 2x, 3x, 4x, 5x, 2x3, 3x5, 4x6, fit, bestFit, fitKeepAR (default = bestFit)
window.frameScale = 2x
#window.frameScale = fitKeepAR
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



######## Platform ########

platform.helpFile = @HELP_FILE

# Code page: rk, koi8 (default = rk)
platform.codePage = rk

Ram ram1 = 0xe000
Ram videoRam = 0x1000
Ram ram2 = 0x800

Rom rom = @ROM, 0x800

AddrSpace addrSpace
addrSpace.range =      &ram1,     0x0000, 0xDFFF
addrSpace.range =      &videoRam, 0xE000, 0xEFFF
addrSpace.range =      &ram2,     0xF000, 0xF7FF
addrSpace.range =      &rom,      0xF800, 0xFFFF

#addrSpace.poke = 0xF75C, 0x54
#addrSpace.poke = 0xF75D, 0x38
addrSpace.poke = 0xF75C, 0x4B
addrSpace.poke = 0xF75D, 0x32

Ppi8255 ppi
AddrSpaceInverter invertedPpi = &ppi

GeneralSoundSource tapeSoundSource
#tapeSoundSource.muted = yes

Mikro80TapeRegister tapeReg
tapeReg.tapeSoundSource = &tapeSoundSource

AddrSpace ioAddrSpace
ioAddrSpace.range = &tapeReg, 0x01, 0x01
ioAddrSpace.range = &invertedPpi, 0x04, 0x07
ioAddrSpace.range = &invertedPpi, 0xA0, 0xA3

Mikro80Renderer crtRenderer
crtRenderer.screenMemory = &videoRam
crtRenderer.font = @SYM_GEN
crtRenderer.altFont = @FONT

Mikro80Core core
core.window = &window
core.crtRenderer = &crtRenderer

Cpu8080 cpu
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
