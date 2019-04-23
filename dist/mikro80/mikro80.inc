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

# Wide screen (16:9): yes, no, custom (default = no)
window.wideScreen = no

# Screen format for window.wideScreen = custom
window.customScreenFormat = 1.667



######## Platform ########

platform.helpFile = @HELP_FILE

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

Mikro80TapeRegister tapeReg

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
