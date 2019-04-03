######## Window ########

EmuWindow window
window.caption = @NAME

# Default window size for window.windowStyle = fixed (default = 800, 600)
#window.defaultWindowSize = 768, 512
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

# Image antialiasing (default = no)
window.antialiasing = no

# Aspect correction (default = no)
window.aspectCorrection = no
#window.aspectCorrection = yes

# Wide screen (16:9) (default = no)
window.wideScreen = no



######## Platform ########

platform.helpFile = @HELP_FILE

Ram ram = 0x9000
SpecVideoRam videoRam = 0x3000

Rom rom = @ROM, @ROM_LEN

Ppi8255 ppi1
Ppi8255 ppi2

AddrSpace addrSpace
addrSpace.range = &ram,      0x0000, 0x8FFF
addrSpace.range = &videoRam, 0x9000, 0xBFFF
addrSpace.range = &rom,      0xC000, @ROM_END
addrSpace.range = &ppi1,     0xF000, 0xF7FF
addrSpace.range = &ppi2,     0xF800, 0xFFFF

SpecRenderer crtRenderer
crtRenderer.screenMemory = &videoRam
crtRenderer.colorMode = 8color

SpecCore core
core.window = &window
core.crtRenderer = &crtRenderer

Cpu8080 cpu
cpu.frequency = 2000000
cpu.startAddr = 0xC000
cpu.addrSpace = &addrSpace
cpu.core = &core

RkKbdLayout kbdLayout
SpecKeyboard keyboard
#keyboard.matrix = "original"

SpecPpi8255Circuit ppiCircuit
ppiCircuit.specKeyboard = &keyboard
ppiCircuit.videoRam = &videoRam
ppi2.circuit = &ppiCircuit



######## File I/O ########

SpecFileLoader loader
loader.addrSpace = &addrSpace
loader.filter = "Файлы Специалиста (*.rks)|*.rks;*.RKS|Все файлы (*.*)|*"

TapeRedirector tapeInFile
tapeInFile.mode = "r"
tapeInFile.filter = "Файлы Специалиста (*.rks)|*.rks;*.RKS|Файлы RK-совместимых ПК (*.rk?)|*.rk;*.rk?;*.RK;*.RK?|Все файлы (*.*)|*"

TapeRedirector tapeOutFile
tapeOutFile.mode = "w"
tapeOutFile.filter = "Файлы Специалиста (*.rks)|*.rks|Файлы RK-совместимых ПК (*.rk)|*.rk|Все файлы (*.*)|*"

RkTapeOutHook tapeOutHook = @C80C_ADDR
tapeOutHook.outReg = A
tapeOutHook.tapeRedirector = &tapeOutFile
cpu.addHook = &tapeOutHook

RkTapeInHook tapeInHook = @C806_ADDR
tapeInHook.tapeRedirector = &tapeInFile
#tapeInHook.enabled = no
tapeInHook.suspendAfterResetForMs = 200
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
configTab.addControl = 2, &wsSelector

#crtRenderer.visibleArea
ConfigRadioSelector cropSelector = &crtRenderer, visibleArea, "Отображать реальный бордюр"
cropSelector.addItem = no, "Нет"
cropSelector.addItem = yes, "Да"
configTab.addControl = 3, &cropSelector

#crtRenderer.colorMode
ConfigRadioSelector colorSelector = &crtRenderer, colorMode, "Цветовой режим"
colorSelector.addItem = mono, "Монохромный"
colorSelector.addItem = 4color, "4-цветный"
colorSelector.addItem = 8color, "8-цветный"
configTab.addControl = 2, &colorSelector

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

ConfigRadioSelector tapeSuspendSelector = &tapeInHook, suspendAfterResetForMs, "Подавлять открытие файла после сброса"
tapeSuspendSelector.addItem = 200, "Вкл."
tapeSuspendSelector.addItem = 0, "Выкл."
configTab.addControl = 3, &tapeSuspendSelector
