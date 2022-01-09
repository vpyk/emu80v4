# ���� ��������
@OPT_FILE = "korvet/korvet.opt"

######## Include Saved Options ########

include @OPT_FILE


######## Configuration Window ########

ConfigTab configTab = @NAME
configTab.optFileName = @OPT_FILE

#window.frameScale
ConfigRadioSelector scaleSelector = &window, frameScale, "���������������"
scaleSelector.addItem = 1x, "1x"
scaleSelector.addItem = 2x, "2x"
scaleSelector.addItem = 3x, "3x"
scaleSelector.addItem = 4x, "4x"
scaleSelector.addItem = 5x, "5x"
scaleSelector.addItem = 2x3, "2x3"
scaleSelector.addItem = 3x5, "3x5"
scaleSelector.addItem = fit, "��������� �� ��� ����"
scaleSelector.addItem = fitKeepAR, "��������� � ����������� ���������"
scaleSelector.addItem = bestFit, "��������� � ����� �������������"
configTab.addControl = 1, &scaleSelector

#window.windowStyle
ConfigRadioSelector styleSelector = &window, windowStyle, "������ ����"
styleSelector.addItem = autosize, "������������� ������������"
styleSelector.addItem = sizable, "���������� �������������"
styleSelector.addItem = fixed, "�������������"
configTab.addControl = 1, &styleSelector

#window.fieldsMixing
ConfigRadioSelector mixSelector = &window, fieldsMixing, "���������� ����������"
mixSelector.addItem = none, "����."
mixSelector.addItem = mix, "���."
mixSelector.addItem = interlace, "�����������"
mixSelector.addItem = scanline, "�������� ����� ������"
configTab.addControl = 1, &mixSelector

#window.antialiasing
ConfigRadioSelector aaSelector = &window, antialiasing, "�����������"
aaSelector.addItem = no, "����."
aaSelector.addItem = yes, "���."
configTab.addControl = 2, &aaSelector

#window.aspectCorrection
ConfigRadioSelector arSelector = &window, aspectCorrection, "��������� Aspect Ratio"
arSelector.addItem = no, "��� (1:1)"
arSelector.addItem = yes, "������������ AR"
configTab.addControl = 2, &arSelector

#window.wideScreen
ConfigRadioSelector wsSelector = &window, wideScreen, "������ ������"
wsSelector.addItem = no, "4:3"
wsSelector.addItem = yes, "16:9"
wsSelector.addItem = custom, "����������������"
configTab.addControl = 2, &wsSelector

#crtRenderer.visibleArea
ConfigRadioSelector cropSelector = &crtRenderer, visibleArea, "���������� ������ ������� �������"
cropSelector.addItem = no, "���"
cropSelector.addItem = yes, "��"
configTab.addControl = 2, &cropSelector

#kbdLayout.layout
ConfigRadioSelector laSelector = &kbdLayout, layout, "��������� ����������"
laSelector.addItem = qwerty, "Qwerty"
laSelector.addItem = jcuken, "������"
laSelector.addItem = smart, "������� ���������"
configTab.addControl = 3, &laSelector

#tapeGrp.enabled
ConfigRadioSelector tapeSelector = &tapeGrp, enabled, "�������� ��������� � �����������"
tapeSelector.addItem = yes, "���."
tapeSelector.addItem = no, "����."
configTab.addControl = 3, &tapeSelector

#platform.codePage
ConfigRadioSelector codePageSelector = &platform, codePage, "������� �������� ���������"
codePageSelector.addItem = rk, "��-86"
codePageSelector.addItem = koi8, "���-8"
configTab.addControl = 3, &codePageSelector