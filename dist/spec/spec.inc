######## Window ########

EmuWindow window
window.caption = @NAME

# Default window size for window.windowStyle = fixed (default = 800, 600)
#window.defaultWindowSize = 768, 512
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



######## Platform ########

platform.helpFile = @HELP_FILE

# Code page: rk, koi8 (default = rk)
platform.codePage = rk

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

ifndef Z80
  Cpu8080 cpu
else
  CpuZ80 cpu
endif

cpu.frequency = 2000000
cpu.startAddr = 0xC000
cpu.addrSpace = &addrSpace
cpu.core = &core

GeneralSoundSource tapeSoundSource
#tapeSoundSource.muted = yes
GeneralSoundSource beepSoundSource

SpecKbdLayout kbdLayout
SpecKeyboard keyboard
keyboard.matrix = @KBD

SpecPpi8255Circuit ppiCircuit
ppiCircuit.specKeyboard = &keyboard
ppiCircuit.videoRam = &videoRam
ppiCircuit.tapeSoundSource = &tapeSoundSource
ppiCircuit.beepSoundSource = &beepSoundSource

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
configTab.addControl = 1, &mixSelector

#window.smoothing
ConfigRadioSelector smSelector = &window, smoothing, "Сглаживание"
smSelector.addItem = nearest, "Выкл."
smSelector.addItem = bilinear, "Билинейное"
smSelector.addItem = sharp, "Границы пикселей (рекомендуется)"
configTab.addControl = 2, &smSelector

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

#kayboard.matrix
ConfigRadioSelector kbdSelector = &keyboard, matrix, "Тип клавиатуры"
kbdSelector.addItem = original, "Оригинальная"
kbdSelector.addItem = ramfos, "RAMFOS"
kbdSelector.addItem = eureka, "Eureka"
kbdSelector.addItem = lik, "Lik"
configTab.addControl = 3, &kbdSelector

ConfigRadioSelector tapeSelector = &tapeGrp, enabled, "Перехват магнитофона"
tapeSelector.addItem = yes, "Вкл."
tapeSelector.addItem = no, "Выкл."
configTab.addControl = 3, &tapeSelector

ConfigRadioSelector tapeSuspendSelector = &tapeInHook, suspendAfterResetForMs, "Подавлять открытие файла после сброса"
tapeSuspendSelector.addItem = 200, "Вкл."
tapeSuspendSelector.addItem = 0, "Выкл."
configTab.addControl = 3, &tapeSuspendSelector

#platform.codePage
ConfigRadioSelector codePageSelector = &platform, codePage, "Таблица символов отладчика"
codePageSelector.addItem = rk, "РК-86"
codePageSelector.addItem = koi8, "КОИ-8"
configTab.addControl = 3, &codePageSelector
