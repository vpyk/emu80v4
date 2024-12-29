# Файл настроек
@OPT_FILE = "pk8000/pk8000.opt"

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
mixSelector.addItem = mix, "Вкл."
mixSelector.addItem = interlace, "Чередование"
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

#kbdLayout.layout
ConfigRadioSelector laSelector = &kbdLayout, layout, "Раскладка клавиатуры"
laSelector.addItem = qwerty, "Qwerty"
laSelector.addItem = jcuken, "Йцукен"
laSelector.addItem = smart, "«Умная» раскладка"
configTab.addControl = 2, &laSelector

#crtRenderer.colorMode
ConfigRadioSelector colorRadioSelector = &crtRenderer, colorMode, "Режим цвета"
colorRadioSelector.addItem = color, "Цветной"
colorRadioSelector.addItem = mono, "Ч/б"
configTab.addControl = 2, &colorRadioSelector

#kbdLayout.numpadJoystick
ConfigRadioSelector njSelector = &kbdLayout, numpadJoystick, "Клавиши джойстика"
njSelector.addItem = no, "Совмещены с клавиатурой (стрелки, пробел, Enter)"
njSelector.addItem = yes, "Доп. цифр. клавиатура (стрелки, 5, Ins)"
configTab.addControl = 3, &njSelector

ConfigRadioSelector tapeSelector = &tapeGrp, enabled, "Перехват магнитофона"
tapeSelector.addItem = yes, "Вкл."
tapeSelector.addItem = no, "Выкл."
configTab.addControl = 3, &tapeSelector

ConfigRadioSelector multiblockSelector = &loader, allowMultiblock, "Мультиблочная загрузка"
multiblockSelector.addItem = yes, "Вкл."
multiblockSelector.addItem = no, "Выкл."
configTab.addControl = 3, &multiblockSelector

#platform.codePage
ConfigRadioSelector codePageSelector = &platform, codePage, "Таблица символов отладчика"
codePageSelector.addItem = rk, "РК-86"
codePageSelector.addItem = koi8, "КОИ-8"
configTab.addControl = 3, &codePageSelector

#psgSoundSource.mixing
ConfigRadioSelector ayStereoSelector = &psgSoundSource, mixing, "Режим AY"
ayStereoSelector.addItem = stereo, "Стерео"
ayStereoSelector.addItem = mono, "Моно"
configTab.addControl = 3, &ayStereoSelector
