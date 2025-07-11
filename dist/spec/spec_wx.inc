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
