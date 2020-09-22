#-------------------------------------------------
#
# Project created by QtCreator 2017-06-12T11:16:24
#
#-------------------------------------------------

QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Emu80qt
TEMPLATE = app

#CONFIG += console
CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Profiler
#QMAKE_CXXFLAGS_DEBUG += -pg
#QMAKE_LFLAGS_DEBUG += -pg


DEFINES += PAL_QT

SOURCES += \
    Covox.cpp \
    Main.cpp \
    AddrSpace.cpp \
    Apogey.cpp \
    AtaDrive.cpp \
    CloseFileHook.cpp \
    ConfigReader.cpp \
    Cpu.cpp \
    Cpu8080.cpp \
    Cpu8080dasm.cpp \
    CpuHook.cpp \
    CpuZ80.cpp \
    CpuZ80dasm.cpp \
    Crt8275.cpp \
    Crt8275Renderer.cpp \
    CrtRenderer.cpp \
    Debugger.cpp \
    Dma8257.cpp \
    EmuCalls.cpp \
    EmuConfig.cpp \
    Emulation.cpp \
    EmuObjects.cpp \
    EmuWindow.cpp \
    Eureka.cpp \
    Fdc1793.cpp \
    FdImage.cpp \
    FileLoader.cpp \
    GenericModules.cpp \
    KbdLayout.cpp \
    Memory.cpp \
    Mikro80.cpp \
    Mikrosha.cpp \
    MsxTapeHooks.cpp \
    ObjectFactory.cpp \
    Orion.cpp \
    Pal.cpp \
    Parameters.cpp \
    Partner.cpp \
    Pit8253.cpp \
    Pit8253Sound.cpp \
    Pk8000.cpp \
    Platform.cpp \
    PlatformCore.cpp \
    Ppi8255.cpp \
    PpiAtaAdapter.cpp \
    Psg3910.cpp \
    RamDisk.cpp \
    Rk86.cpp \
    RkFdd.cpp \
    RkKeyboard.cpp \
    RkPpi8255Circuit.cpp \
    RkRomDisk.cpp \
    RkSdController.cpp \
    RkTapeHooks.cpp \
    SdAdapters.cpp \
    SdCard.cpp \
    Shortcuts.cpp \
    SoundMixer.cpp \
    Specialist.cpp \
    TapeRedirector.cpp \
    Ut88.cpp \
    Vector.cpp \
    WavReader.cpp \
    WavWriter.cpp \
    qt/qtAboutDialog.cpp \
    qt/qtAudioDevice.cpp \
    qt/qtChoosePlatformDialog.cpp \
    qt/qtConfigWidget.cpp \
    qt/qtMainWindow.cpp \
    qt/qtPaintWidget.cpp \
    qt/qtPal.cpp \
    qt/qtPalFile.cpp \
    qt/qtPalWindow.cpp \
    qt/qtPlatformConfig.cpp \
    qt/qtRenderHelper.cpp \
    qt/qtSettingsDialog.cpp \
    qt/qtToolBtn.cpp \
    qt/qtHelpDialog.cpp

HEADERS  += \
    AddrSpace.h \
    Apogey.h \
    AtaDrive.h \
    CloseFileHook.h \
    ConfigReader.h \
    Covox.h \
    Cpu.h \
    Cpu8080.h \
    Cpu8080dasm.h \
    CpuHook.h \
    CpuWaits.h \
    CpuZ80.h \
    CpuZ80dasm.h \
    Crt8275.h \
    Crt8275Renderer.h \
    CrtRenderer.h \
    Debugger.h \
    Dma8257.h \
    EmuCalls.h \
    EmuConfig.h \
    Emulation.h \
    EmuObjects.h \
    EmuTypes.h \
    EmuWindow.h \
    Eureka.h \
    Fdc1793.h \
    FdImage.h \
    FileLoader.h \
    GenericModules.h \
    Globals.h \
    KbdLayout.h \
    Keyboard.h \
    Memory.h \
    Mikro80.h \
    Mikrosha.h \
    MsxTapeHooks.h \
    ObjectFactory.h \
    Orion.h \
    Pal.h \
    PalFile.h \
    PalKeys.h \
    PalWindow.h \
    Parameters.h \
    Partner.h \
    Pit8253.h \
    Pit8253Sound.h \
    Pk8000.h \
    Platform.h \
    PlatformCore.h \
    Ppi8255.h \
    Ppi8255Circuit.h \
    PpiAtaAdapter.h \
    Psg3910.h \
    RamDisk.h \
    Rk86.h \
    RkFdd.h \
    RkKeyboard.h \
    RkPpi8255Circuit.h \
    RkRomDisk.h \
    RkSdController.h \
    RkTapeHooks.h \
    SdAdapters.h \
    SdCard.h \
    Shortcuts.h \
    SoundMixer.h \
    Specialist.h \
    TapeRedirector.h \
    Ut88.h \
    Vector.h \
    Version.h \
    WavReader.h \
    WavWriter.h \
    qt/qtAboutDialog.h \
    qt/qtAudioDevice.h \
    qt/qtChoosePlatformDialog.h \
    qt/qtConfigWidget.h \
    qt/qtMainWindow.h \
    qt/qtPaintWidget.h \
    qt/qtPal.h \
    qt/qtPalFile.h \
    qt/qtPalWindow.h \
    qt/qtPlatformConfig.h \
    qt/qtRenderHelper.h \
    qt/qtSettingsDialog.h \
    qt/qtToolBtn.h \
    qt/qtHelpDialog.h

RESOURCES += \
    qt/icons.qrc \
    qt/translations.qrc

TRANSLATIONS += qt/emu80_ru.ts

CODECFORSRC = UTF-8

FORMS += \
    qt/qtPlatformConfig.ui \
    qt/qtSettingsDialog.ui \
    qt/qtChoosePlatformDialog.ui \
    qt/qtAboutDialog.ui \
    qt/qtHelpDialog.ui \
    qt/qtApogeyConfig.ui

win32:RC_FILE = qt/emu80.rc

BUILDDIR = build
OBJECTS_DIR = $${BUILDDIR}/obj
MOC_DIR = $${BUILDDIR}/moc
RCC_DIR = $${BUILDDIR}/qrc
UI_DIR = $${BUILDDIR}/ui

INSTALLDIR = ~/emu80
QMAKE_EXTRA_TARGETS += install
install.commands = mkdir -p $$INSTALLDIR && mkdir -p $$INSTALLDIR/_settings && cp Emu80qt $$INSTALLDIR && cp -r dist/* $$INSTALLDIR && cp COPYING.txt $$INSTALLDIR && cp whatsnew.txt $$INSTALLDIR && cp doc/* $$INSTALLDIR
