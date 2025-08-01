﻿/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2025
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QKeySequence>
#include <QStatusBar>
#include <QLabel>
#include <QActionGroup>
#include <QKeyEvent>
#include <QShortcut>
#include <QMimeData>
#include <QToolButton>
#include <QMessageBox>
#include <QLayout>
#include <QApplication>
#include <QWindow>
#include <QScreen>
#include <QSettings>
#include <QDir>

#include <string>

#include "qtPalWindow.h"
#include "qtMainWindow.h"
#include "qtPaintWidget.h"
#include "qtRenderHelper.h"
#include "qtToolBtn.h"
#include "qtSettingsDialog.h"
#include "qtPlatformConfig.h"
#include "qtAboutDialog.h"
#include "qtHelpDialog.h"

#include "qtPal.h"
#include "../EmuCalls.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    g_renderHelper->addWindow(this);

    setAcceptDrops(true);

    m_paintWidget = new PaintWidget(this);
    setCentralWidget(m_paintWidget);
    m_paintWidget->setFocusPolicy(Qt::StrongFocus);

    m_fpsTimer.setInterval(250);
    connect(&m_fpsTimer, SIGNAL(timeout()), this, SLOT(onFpsTimer()));
    m_fpsTimer.start();

    connect(qApp,SIGNAL(aboutToQuit()),this,SLOT(onQuit()));

    // determine light or dark theme used
    QPalette palette = qApp->palette();
    QColor bgColor = palette.color(QPalette::Window);
    m_darkTheme = qGray(bgColor.rgb()) < 128;

    m_1xIcon = QIcon(m_darkTheme ? ":/icons/1x-dark.png" : ":/icons/1x.png");
    m_2xIcon = QIcon(m_darkTheme ? ":/icons/2x-dark.png" : ":/icons/2x.png");
    m_3xIcon = QIcon(m_darkTheme ? ":/icons/3x-dark.png" : ":/icons/3x.png");
    m_4xIcon = QIcon(m_darkTheme ? ":/icons/4x-dark.png" : ":/icons/4x.png");
    m_5xIcon = QIcon(m_darkTheme ? ":/icons/5x-dark.png" : ":/icons/5x.png");
    m_15xIcon = QIcon(m_darkTheme ? ":/icons/15x-dark.png" : ":/icons/15x.png");
    m_25xIcon = QIcon(m_darkTheme ? ":/icons/25x-dark.png" : ":/icons/25x.png");
    m_resizableIcon = QIcon(m_darkTheme ? ":/icons/resizable-dark.png" : ":/icons/resizable.png");
    m_smoothingNearestIcon = QIcon(m_darkTheme ? ":/icons/sm_nearest-dark.png" : ":/icons/sm_nearest.png");
    m_smoothingSharpIcon = QIcon(m_darkTheme ? ":/icons/sm_sharp-dark.png" : ":/icons/sm_sharp.png");
    m_smoothingBilinearIcon = QIcon(m_darkTheme ? ":/icons/sm_bilinear-dark.png" : ":/icons/sm_bilinear.png");
    m_shaderIcon = QIcon(m_darkTheme ? ":/icons/shader-dark.png" : ":/icons/shader.png");
    m_presetIcon = QIcon(m_darkTheme ? ":/icons/preset-dark.png" : ":/icons/preset.png");

#ifdef _WIN32
    m_shiftF10shortcut = new QShortcut(this);
    m_shiftF10shortcut->setKey(Qt::SHIFT | Qt::Key_F10);
    connect(m_shiftF10shortcut, SIGNAL(activated()), this, SLOT(onShiftF10()));
#endif
}

MainWindow::~MainWindow()
{
    g_renderHelper->removeWindow(this);
    if (m_settingsDialog)
        delete m_settingsDialog;
}


void MainWindow::setPalWindow(PalWindow* palWindow)
{
    if (!palWindow) {
        if (m_windowType == EWT_EMULATION) {
            savePosition();
            m_disableResizing = true;
        }

        m_windowType = EWT_UNDEFINED;
        m_platformName = "";
        m_platformGroupName = "";
        return;
    }

    m_windowType = palWindow->getWindowType();

    if (!m_palWindow && m_windowType == EWT_EMULATION) {
        // first emulation window
        createActions();
        fillPlatformListMenu();
        fillShaderListMenu();
    }

    m_palWindow = palWindow;

    m_platformName = m_palWindow->getPlatformObjectName();
    std::string::size_type dotPos = m_platformName.find(".",0);
    m_platformGroupName = m_platformName.substr(0, dotPos);

    switch (m_windowType) {
    case EWT_EMULATION:
        if (!m_settingsDialog) {
            m_settingsDialog = new SettingsDialog(this);

            m_fpsLabel = new QLabel("", this);
            m_speedLabel = new QLabel("", this);
            m_kbdLabel = new QLabel("", this);
            m_colorLabel = new QLabel("", this);
            m_crtModeLabel = new QLabel("", this);
            m_crtModeLabel->setVisible(false);
            m_dmaTimeLabel = new QLabel("", this);
            m_dmaTimeLabel->setVisible(false);
            m_imageSizeLabel = new QLabel("", this);
            m_tapeLabel = new QLabel("", this);
            m_tapeLabel->setVisible(false);
            m_wavLabel = new QLabel("", this);
            m_wavLabel->setVisible(false);
            m_prnLabel = new QLabel("", this);
            m_prnLabel->setVisible(false);
            m_pasteLabel = new QLabel("", this);
            m_pasteLabel->setVisible(false);

            m_fpsLabel->setToolTip(tr("Emulator FPS"));
            m_speedLabel->setToolTip(tr("Emulation speed"));
            m_kbdLabel->setToolTip(tr("Keyboard layout"));
            m_colorLabel->setToolTip(tr("Color mode"));
            m_crtModeLabel->setToolTip(tr("Screen mode"));
            m_dmaTimeLabel->setToolTip(tr("Time consumed during DMA"));
            m_imageSizeLabel->setToolTip(tr("Image size in emulator"));
            m_tapeLabel->setToolTip(tr("Tape file I/O"));
            m_wavLabel->setToolTip(tr("Wav file I/O"));
            m_prnLabel->setToolTip(tr("Printer capture"));
            m_prnLabel->setToolTip(tr("Text pasting"));

            m_statusBar = statusBar();
            if (m_darkTheme)
                m_statusBar->setStyleSheet("QStatusBar::item { border: 1px inset #383838;}");
            else
                m_statusBar->setStyleSheet("QStatusBar::item { border: 1px inset #B0B0B0;}");
            m_statusBar->addWidget(m_fpsLabel);
            m_statusBar->addWidget(m_speedLabel);
            m_statusBar->addWidget(m_kbdLabel);
            m_statusBar->addWidget(m_colorLabel);
            m_statusBar->addWidget(m_tapeLabel);
            m_statusBar->addWidget(m_wavLabel);
            m_statusBar->addWidget(m_prnLabel);
            m_statusBar->addWidget(m_pasteLabel);
            m_statusBar->addWidget(m_crtModeLabel);
            m_statusBar->addWidget(m_imageSizeLabel);
            m_statusBar->addWidget(m_dmaTimeLabel);
        }

        {
            QString groupName = QString::fromUtf8(getPlatformGroupName().c_str());
            m_loaderLastFiles.setPlatformName(groupName);
            m_fddLastFiles.setPlatformName(groupName);
            m_hddLastFiles.setPlatformName(groupName);
            m_eddLastFiles.setPlatformName(groupName);
        }

        tuneMenu();

        if (m_fullscreenMode)
            setFullScreen(false);

        adjustClientSize();

        m_settingsDialog->initConfig();
        updateConfig(); // немного избыточно

        m_disableResizing = false;

        {
            bool resizable = m_clientWidth == 0 && m_clientHeight == 0;
            if (resizable) {
                QSettings settings;
                SET_INI_CODEC(settings);
                settings.beginGroup("window");
                if (settings.contains("width") && settings.contains("height")) {
                    m_clientWidth = settings.value("width").toInt();
                    m_clientHeight = settings.value("height").toInt();
                    adjustClientSize();
                    m_clientWidth = m_clientHeight = 0;
                    adjustClientSize(); //resizable
                }
            }
        }

        if (m_settingsDialog->getOptionValue("showHelp") == "yes") {
            std::string helpFile = palMakeFullFileName(emuGetPropertyValue(m_palWindow->getPlatformObjectName(), "helpFile"));
            HelpDialog* hd = HelpDialog::execute(QString::fromUtf8(helpFile.c_str()), true);
            if (hd)
                connect(hd, SIGNAL(resetShowHelp()), m_settingsDialog, SLOT(onResetShowHelp()));
        }

        break;
    case EWT_DEBUG:
        createDebugActions();
        getPaintWidget()->setHideCursor(false);
        //getPaintWidget()->setVsync(false);

        m_disableResizing = false;
        break;
    default:
        break;
    }
    m_controlsCreated = true;
}


void MainWindow::setClientSize(int width, int height)
{
    // minimum window size
    if (width != 0 && width < 120) width = 120;
    if (height != 0 && height < 75) height = 75;

    if (m_windowType == EWT_EMULATION) {
        QSettings settings;
        SET_INI_CODEC(settings);
        settings.beginGroup("system");
        bool preserveSize = settings.value("preserveSize") == "yes";

        if (width == 0 && height == 0) {
            if (preserveSize) {
                settings.endGroup();
                settings.beginGroup("window");
                if (settings.contains("width") && settings.contains("height")) {
                    m_clientWidth = settings.value("width").toInt();
                    m_clientHeight = settings.value("height").toInt();
                    adjustClientSize();
                }
            }
        } else
            if (!preserveSize)
                savePosition();
    }

    m_clientWidth = width;
    m_clientHeight = height;

    adjustClientSize();
}


void MainWindow::adjustClientSize()
{
    if (m_disableResizing)
        return;

    bool resizable = m_clientWidth == 0 && m_clientHeight == 0;

    if (resizable || m_fullscreenMode) {
        m_paintWidget->setFixedSize(m_paintWidget->width(), m_paintWidget->height());
        layout()->setSizeConstraint(QLayout::SetFixedSize);
        if (!m_fullscreenMode)
            adjustSize();
        m_paintWidget->setFixedSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
        layout()->setSizeConstraint(QLayout::SetNoConstraint);
        setFixedSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
    } else {
        m_paintWidget->setFixedSize(m_clientWidth, m_clientHeight);
        layout()->setSizeConstraint(QLayout::SetFixedSize);
        adjustSize();
    }

    if (m_statusBar)
        m_statusBar->setSizeGripEnabled(resizable);
}


void MainWindow::checkIfWindowIsVisible()
{
    if (!isVisible() && !m_disableResizing) {
        adjustClientSize();
        show();

        HelpDialog::activate();
    }
}


void MainWindow::showWindow()
{
    if (isVisible())
        return;

    if (m_showFirstTime) {
        m_showFirstTime = false;

        if (m_windowType == EWT_EMULATION) {
            // place main window, not debug one
            QSettings settings;
            SET_INI_CODEC(settings);
            settings.beginGroup("window");
            if (settings.contains("left") && settings.contains("top")) {
                int left = settings.value("left").toInt();
                int top = settings.value("top").toInt();
                if (left == 0) left = 16;
                if (top == 0) top = 32;
                bool resizable = m_clientWidth == 0 && m_clientHeight == 0;
                if (resizable && settings.contains("width") && settings.contains("height")) {
                    int width = settings.value("width").toInt();
                    int height = settings.value("height").toInt();
                    m_paintWidget->setFixedSize(width, height);
                    layout()->setSizeConstraint(QLayout::SetFixedSize);
                    adjustSize();
                    m_paintWidget->setFixedSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
                    layout()->setSizeConstraint(QLayout::SetNoConstraint);
                    setFixedSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
                }
                setGeometry(left, top, geometry().width(), geometry().height());
            } else {
                QRect screenRec = QGuiApplication::primaryScreen()->availableGeometry();
                QRect frameRec = frameGeometry();
                move(screenRec.left() + (screenRec.width() - frameRec.width()) / 3, screenRec.top() + (screenRec.height() - frameRec.height()) / 3);
            }
            settings.endGroup();

        }
        else { //if (m_windowType == EWT_DEBUG) {
            show();
        }
    } else {
        if (m_hiddenWindowGeometry != QRect(0, 0, 0, 0)) {
            if (QGuiApplication::platformName() == "xcb") {
                // workaround for X Window (KDE, LXDE and XFCE does not restore window pos after hiding)
                setGeometry(m_hiddenWindowGeometry.adjusted(1, 0, 0, 0));
                show();
                setGeometry(m_hiddenWindowGeometry);
            } else {
                setGeometry(m_hiddenWindowGeometry);
                show();
            }
        } else
            show();
    }
}


void MainWindow::MainWindow::hideWindow()
{
    if (!isVisible())
        return;

    m_hiddenWindowGeometry = geometry();
    hide();
}

void MainWindow::setFullScreen(bool fullscreen)
{
    bool visible = !(m_fullwindowAction->isChecked() || fullscreen);

    if (m_menuBar)
        m_menuBar->setVisible(visible);
    if (m_statusBar)
        m_statusBar->setVisible(visible);
    if (m_toolBar)
        m_toolBar->setVisible(visible);
    if (fullscreen) {
        m_savedGeometry = geometry();
        layout()->setSizeConstraint(QLayout::SetNoConstraint);
        setFixedSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
        showFullScreen();
    } else {
        showNormal();
        if (m_savedGeometry != geometry())
            setGeometry(m_savedGeometry);
    }
    m_fullscreenMode = fullscreen;
    adjustClientSize();
}


void MainWindow::fillPlatformListMenu()
{
    QSettings settings;
    SET_INI_CODEC(settings);
    settings.beginGroup("system");
    bool selectedPlatformsOnly = settings.value("selectedPlatformsOnly").toString() == "yes";
    QStringList selectedPlatforms = settings.value("enabledPlatforms").toStringList();
    settings.endGroup();

    QMenu* recentPlatformsMenu = new QMenu(tr("Recent"), m_platformListMenu);
    m_platformListMenu->addMenu(recentPlatformsMenu);
    m_platformListMenu->addSeparator();

    for (int i = 0; i < LAST_PLATFORMS_QTY; i++) {
        QAction* action = new QAction(m_platformListMenu);
        action->setVisible(false);
        /*QFont font = action->font();
        font.setItalic(true);
        action->setFont(font);*/
        m_lastPlatformsActions[i] = action;
        recentPlatformsMenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(onPlatformSelect()));
    }

    const std::vector<PlatformInfo>* platformList = emuGetPlatforms();

    QMap<QString, int> groups;

    for (auto it = platformList->begin(); it != platformList->end(); it++) {
        std::string platform = (*it).objName;
        std::string::size_type dotPos = platform.find(".",0);
        QString group = QString::fromUtf8(platform.substr(0, dotPos).c_str());
        if (!selectedPlatformsOnly || selectedPlatforms.contains(group))
            groups[group]++;
    }

    QMap<QString, QMenu*> groupMenus;

    for (auto it = platformList->begin(); it != platformList->end(); it++) {
        std::string sPlatform = (*it).objName;
        std::string::size_type dotPos = sPlatform.find(".",0);
        QString group = QString::fromUtf8(sPlatform.substr(0, dotPos).c_str());
        QString platform = QString::fromUtf8(sPlatform.c_str());
        QString platformName = QString::fromUtf8((*it).platformName.c_str());
        m_platformNames[platform] = platformName;
        if (selectedPlatformsOnly && !selectedPlatforms.contains(group))
            continue;
        if (groups[group] == 1) {
            // single platform
            QAction* action = new QAction(platformName, m_platformListMenu);
            action->setData(platform);
            /*QFont font = action->font();
            font.setBold(true);
            action->setFont(font);*/
            m_platformListMenu->addAction(action);
            connect(action, SIGNAL(triggered()), this, SLOT(onPlatformSelect()));
        } else { // if (groups[group] > 1) {
            QMenu* menu = groupMenus[group];
            if (!menu) {
                // first platforn in group
                menu = new QMenu(/*platformName,*/ m_platformListMenu);
                groupMenus[group] = menu;
                m_platformListMenu->addMenu(menu);
            }

            QAction* action = new QAction(platformName, m_platformListMenu);
            action->setData(platform);
            if (!platform.contains(".")) {
                menu->setTitle(platformName.left(platformName.indexOf("(")));
                QFont font = action->font();
                font.setBold(true);
                action->setFont(font);
            }
            menu->addAction(action);
            connect(action, SIGNAL(triggered()), this, SLOT(onPlatformSelect()));
        }
    }

    settings.beginGroup("Last_platforms");

    for (int i = 0; i < LAST_PLATFORMS_QTY; i++) {
        QString value = settings.value(QString::number(i)).toString();

        // workaround for changed Orion platform name
        if (value == "orion.2")
            value = "orion";

        if (!value.isEmpty()) {
            m_lastPlatformsActions[i]->setVisible(true);
            m_lastPlatformsActions[i]->setData(value);
            QString platformName = m_platformNames[value];
            if (platformName.isEmpty())
                platformName = value;
            m_lastPlatformsActions[i]->setText(platformName);
        }
    }
    settings.endGroup();
}


void MainWindow::fillShaderListMenu()
{
    QDir dir(QString::fromUtf8(palMakeFullFileName("shaders").c_str()));
    QStringList filter;
    filter << "*.glsl";
    dir.setNameFilters(filter);
    dir.setSorting(QDir::Name);
    QStringList shaderFiles = dir.entryList();

    QActionGroup* shaderListGroup = new QActionGroup(m_shaderListMenu);

    QAction* action = new QAction(tr("Off"), m_shaderListMenu);
    m_shaderListMenu->addAction(action);
    shaderListGroup->addAction(action);
    action->setData("none");
    action->setCheckable(true);
    m_shaderListMenu->addSeparator();
    connect(action, SIGNAL(triggered()), this, SLOT(onShaderSelect()));

    foreach (QString shaderName, shaderFiles) {
        action = new QAction(m_shaderListMenu);
        //action->setData("shaders/" + shaderName);
        shaderName.truncate(shaderName.lastIndexOf("."));
        action->setData(shaderName);
        action->setText(shaderName);
        action->setCheckable(true);
        m_shaderListMenu->addAction(action);
        shaderListGroup->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(onShaderSelect()));
        m_shaderList.append(shaderName);
    }
}


void MainWindow::createActions()
{
    m_menuBar = menuBar();
    m_menuBar->setContextMenuPolicy(Qt::PreventContextMenu);
    m_toolBar = new QToolBar(this);
    m_toolBar->setFloatable(false);
    //m_toolBar->setMovable(false);
    m_toolBar->setContextMenuPolicy(Qt::PreventContextMenu);
    m_toolBar->setIconSize(QSize(16, 16));
    addToolBar(m_toolBar);
    m_toolBar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    // File menu
    QMenu* fileMenu = m_menuBar->addMenu(tr("File"));

    // Load and run
    m_loadRunMenu = new QMenu(tr("Load && Run"));
    m_loadRunMenu->setIcon(QIcon(":/icons/open_run.png"));

    m_loadRunAction = new QAction(tr("Open file..."), this);
    m_loadRunMenuAction = m_loadRunMenu->menuAction();
    m_loadRunMenuAction->setToolTip(tr("Load file and run (Alt-F3)"));
    QList<QKeySequence> loadRunKeysList;
    ADD_HOTKEY(loadRunKeysList, Qt::Key_F3);
    m_loadRunAction->setShortcuts(loadRunKeysList);
    addAction(m_loadRunAction);
    m_loadRunMenu->addAction(m_loadRunAction);
    m_toolBar->addAction(m_loadRunMenuAction);
    connect(m_loadRunAction, SIGNAL(triggered()), this, SLOT(onLoadRun()));
    connect(m_loadRunMenuAction, SIGNAL(triggered()), this, SLOT(onLoadRun()));

    m_loadRunMenu->addSeparator();
    for (int i = 0; i < LAST_FILES_QTY; i++) {
        m_loadRunLastFilesActions[i] = new QAction(this);
        m_loadRunMenu->addAction(m_loadRunLastFilesActions[i]);
        connect(m_loadRunLastFilesActions[i], SIGNAL(triggered()), this, SLOT(onLoadRunLastFiles()));
    }

    //fileMenu->addAction(m_loadRunAction);
    fileMenu->addMenu(m_loadRunMenu);


    // Load
    m_loadMenu = new QMenu(tr("Load"));
    m_loadMenu->setIcon(QIcon(":/icons/open.png"));

    m_loadAction = new QAction(tr("Open file..."), this);
    m_loadMenuAction = m_loadMenu->menuAction();
    m_loadMenuAction->setToolTip(tr("Load file (Alt-L)"));
    QList<QKeySequence> loadKeysList;
    ADD_HOTKEY(loadKeysList, Qt::Key_L);
    m_loadAction->setShortcuts(loadKeysList);
    addAction(m_loadAction);
    m_loadMenu->addAction(m_loadAction);
    m_toolBar->addAction(m_loadMenuAction);
    connect(m_loadAction, SIGNAL(triggered()), this, SLOT(onLoad()));
    connect(m_loadMenuAction, SIGNAL(triggered()), this, SLOT(onLoad()));

    m_loadMenu->addSeparator();
    for (int i = 0; i < LAST_FILES_QTY; i++) {
        m_loadLastFilesActions[i] = new QAction(this);
        m_loadMenu->addAction(m_loadLastFilesActions[i]);
        connect(m_loadLastFilesActions[i], SIGNAL(triggered()), this, SLOT(onLoadLastFiles()));
    }

    //fileMenu->addAction(m_loadAction);
    fileMenu->addMenu(m_loadMenu);


    // Load WAV
    m_loadWavAction = new QAction(QIcon(":/icons/open_wav.png"), tr("Load WAV..."), this);
    m_loadWavAction->setToolTip(tr("Load and play (or stop playing) WAV file (Alt-W)"));
    QList<QKeySequence> loadWavKeysList;
    ADD_HOTKEY(loadWavKeysList, Qt::Key_W);
    //loadWavKeysList.append(QKeySequence(Qt::ALT + Qt::Key_W));
    //loadWavKeysList.append(QKeySequence(Qt::META + Qt::Key_W));
    m_loadWavAction->setShortcuts(loadWavKeysList);
    addAction(m_loadWavAction);
    fileMenu->addAction(m_loadWavAction);
    //m_toolBar->addAction(m_loadWavAction);
    connect(m_loadWavAction, SIGNAL(triggered()), this, SLOT(onLoadWav()));

    fileMenu->addSeparator();
    m_toolBar->addSeparator();


    // Select disk A
    m_diskAMenu = new QMenu(tr("Disk A"));
    m_diskAMenu->setIcon(m_diskAOffIcon);

    m_diskAAction = new QAction(tr("Select disk A image..."), this);
    m_diskAMenuAction = m_diskAMenu->menuAction();
    m_diskAMenuAction->setToolTip(tr("Load disk A image (Alt-A)"));
    QList<QKeySequence> diskAKeysList;
    ADD_HOTKEY(diskAKeysList, Qt::Key_A);
    m_diskAAction->setShortcuts(diskAKeysList);
    addAction(m_diskAAction);
    m_diskAMenu->addAction(m_diskAAction);
    m_toolBar->addAction(m_diskAMenuAction);
    connect(m_diskAAction, SIGNAL(triggered()), this, SLOT(onDiskA()));
    connect(m_diskAMenuAction, SIGNAL(triggered()), this, SLOT(onDiskA()));

    m_diskAUnmountAction = new QAction(tr("Unmount"), this);
    m_diskAMenu->addAction(m_diskAUnmountAction);
    m_diskAMenu->addSeparator();
    m_diskAReadOnlyAction = new QAction(tr("Read only"), this);
    m_diskAReadOnlyAction->setCheckable(true);
    m_diskAMenu->addAction(m_diskAReadOnlyAction);
    m_diskAAutoMountAction = new QAction(tr("Auto mount on startup"), this);
    m_diskAAutoMountAction->setCheckable(true);
    m_diskAMenu->addAction(m_diskAAutoMountAction);
    connect(m_diskAUnmountAction, SIGNAL(triggered()), this, SLOT(onUnmountDiskA()));
    connect(m_diskAReadOnlyAction, SIGNAL(triggered()), this, SLOT(onReadOnlyDiskA()));
    connect(m_diskAAutoMountAction, SIGNAL(triggered()), this, SLOT(onAutoMountDiskA()));


    m_diskAMenu->addSeparator();
    for (int i = 0; i < LAST_FILES_QTY; i++) {
        m_fddALastFilesActions[i] = new QAction(this);
        m_diskAMenu->addAction(m_fddALastFilesActions[i]);
        connect(m_fddALastFilesActions[i], SIGNAL(triggered()), this, SLOT(onDiskALastFiles()));
    }

    // Select disk B
    m_diskBMenu = new QMenu(tr("Disk B"));
    m_diskBMenu->setIcon(m_diskBOffIcon);

    m_diskBAction = new QAction(tr("Select disk B image..."), this);
    m_diskBMenuAction = m_diskBMenu->menuAction();
    m_diskBMenuAction->setToolTip(tr("Load disk B image (Alt-B)"));
    QList<QKeySequence> diskBKeysList;
    ADD_HOTKEY(diskBKeysList, Qt::Key_B);
    m_diskBAction->setShortcuts(diskBKeysList);
    addAction(m_diskBAction);
    m_diskBMenu->addAction(m_diskBAction);
    m_toolBar->addAction(m_diskBMenuAction);
    connect(m_diskBAction, SIGNAL(triggered()), this, SLOT(onDiskB()));
    connect(m_diskBMenuAction, SIGNAL(triggered()), this, SLOT(onDiskB()));

    m_diskBUnmountAction = new QAction(tr("Unmount"), this);
    m_diskBMenu->addAction(m_diskBUnmountAction);
    m_diskBMenu->addSeparator();
    m_diskBReadOnlyAction = new QAction(tr("Read only"), this);
    m_diskBReadOnlyAction->setCheckable(true);
    m_diskBMenu->addAction(m_diskBReadOnlyAction);
    m_diskBAutoMountAction = new QAction(tr("Auto mount on startup"), this);
    m_diskBAutoMountAction->setCheckable(true);
    m_diskBMenu->addAction(m_diskBAutoMountAction);
    connect(m_diskBUnmountAction, SIGNAL(triggered()), this, SLOT(onUnmountDiskB()));
    connect(m_diskBReadOnlyAction, SIGNAL(triggered()), this, SLOT(onReadOnlyDiskB()));
    connect(m_diskBAutoMountAction, SIGNAL(triggered()), this, SLOT(onAutoMountDiskB()));

    m_diskBMenu->addSeparator();
    for (int i = 0; i < LAST_FILES_QTY; i++) {
        m_fddBLastFilesActions[i] = new QAction(this);
        m_diskBMenu->addAction(m_fddBLastFilesActions[i]);
        connect(m_fddBLastFilesActions[i], SIGNAL(triggered()), this, SLOT(onDiskBLastFiles()));
    }

    // Select disk C
    m_diskCMenu = new QMenu(tr("Disk C"));
    m_diskCMenu->setIcon(m_diskCOffIcon);

    m_diskCAction = new QAction(tr("Select disk C image..."), this);
    m_diskCMenuAction = m_diskCMenu->menuAction();
    m_diskCMenuAction->setToolTip(tr("Load disk C image (Shift-Alt-C)"));
    QList<QKeySequence> diskCKeysList;
    ADD_HOTKEY(diskCKeysList, Qt::SHIFT | Qt::Key_C);
    m_diskCAction->setShortcuts(diskCKeysList);
    addAction(m_diskCAction);
    m_diskCMenu->addAction(m_diskCAction);
    m_toolBar->addAction(m_diskCMenuAction);
    connect(m_diskCAction, SIGNAL(triggered()), this, SLOT(onDiskC()));
    connect(m_diskCMenuAction, SIGNAL(triggered()), this, SLOT(onDiskC()));

    m_diskCUnmountAction = new QAction(tr("Unmount"), this);
    m_diskCMenu->addAction(m_diskCUnmountAction);
    m_diskCMenu->addSeparator();
    m_diskCReadOnlyAction = new QAction(tr("Read only"), this);
    m_diskCReadOnlyAction->setCheckable(true);
    m_diskCMenu->addAction(m_diskCReadOnlyAction);
    m_diskCAutoMountAction = new QAction(tr("Auto mount on startup"), this);
    m_diskCAutoMountAction->setCheckable(true);
    m_diskCMenu->addAction(m_diskCAutoMountAction);
    connect(m_diskCUnmountAction, SIGNAL(triggered()), this, SLOT(onUnmountDiskC()));
    connect(m_diskCReadOnlyAction, SIGNAL(triggered()), this, SLOT(onReadOnlyDiskC()));
    connect(m_diskCAutoMountAction, SIGNAL(triggered()), this, SLOT(onAutoMountDiskC()));

    m_diskCMenu->addSeparator();
    for (int i = 0; i < LAST_FILES_QTY; i++) {
        m_fddCLastFilesActions[i] = new QAction(this);
        m_diskCMenu->addAction(m_fddCLastFilesActions[i]);
        connect(m_fddCLastFilesActions[i], SIGNAL(triggered()), this, SLOT(onDiskCLastFiles()));
    }

    // Select disk D
    m_diskDMenu = new QMenu(tr("Disk D"));
    m_diskDMenu->setIcon(m_diskDOffIcon);

    m_diskDAction = new QAction(tr("Select disk D image..."), this);
    m_diskDMenuAction = m_diskDMenu->menuAction();
    m_diskDMenuAction->setToolTip(tr("Load disk D image (Shift-Alt-D)"));
    QList<QKeySequence> diskDKeysList;
    ADD_HOTKEY(diskDKeysList, Qt::SHIFT | Qt::Key_D);
    m_diskDAction->setShortcuts(diskDKeysList);
    addAction(m_diskDAction);
    m_diskDMenu->addAction(m_diskDAction);
    m_toolBar->addAction(m_diskDMenuAction);
    connect(m_diskDAction, SIGNAL(triggered()), this, SLOT(onDiskD()));
    connect(m_diskDMenuAction, SIGNAL(triggered()), this, SLOT(onDiskD()));

    m_diskDUnmountAction = new QAction(tr("Unmount"), this);
    m_diskDMenu->addAction(m_diskDUnmountAction);
    m_diskDMenu->addSeparator();
    m_diskDReadOnlyAction = new QAction(tr("Read only"), this);
    m_diskDReadOnlyAction->setCheckable(true);
    m_diskDMenu->addAction(m_diskDReadOnlyAction);
    m_diskDAutoMountAction = new QAction(tr("Auto mount on startup"), this);
    m_diskDAutoMountAction->setCheckable(true);
    m_diskDMenu->addAction(m_diskDAutoMountAction);
    connect(m_diskDUnmountAction, SIGNAL(triggered()), this, SLOT(onUnmountDiskD()));
    connect(m_diskDReadOnlyAction, SIGNAL(triggered()), this, SLOT(onReadOnlyDiskD()));
    connect(m_diskDAutoMountAction, SIGNAL(triggered()), this, SLOT(onAutoMountDiskD()));

    m_diskDMenu->addSeparator();
    for (int i = 0; i < LAST_FILES_QTY; i++) {
        m_fddDLastFilesActions[i] = new QAction(this);
        m_diskDMenu->addAction(m_fddDLastFilesActions[i]);
        connect(m_fddDLastFilesActions[i], SIGNAL(triggered()), this, SLOT(onDiskDLastFiles()));
    }

    fileMenu->addMenu(m_diskAMenu);
    fileMenu->addMenu(m_diskBMenu);
    fileMenu->addMenu(m_diskCMenu);
    fileMenu->addMenu(m_diskDMenu);

    m_menuDiskSeparator = fileMenu->addSeparator();

    // Select HDD image
    m_hddMenu = new QMenu(tr("HDD/CF"));
    m_hddMenu->setIcon(m_hddOffIcon);

    m_hddAction = new QAction(tr("Select HDD/CF image..."), this);
    m_hddMenuAction = m_hddMenu->menuAction();
    m_hddMenuAction->setToolTip(tr("Load HDD/CF image (Shift-Alt-H)"));
    QList<QKeySequence> hddKeyList;
    ADD_HOTKEY(hddKeyList, Qt::SHIFT | Qt::Key_H);
    m_hddAction->setShortcuts(hddKeyList);
    addAction(m_hddAction);
    m_hddMenu->addAction(m_hddAction);
    m_toolBar->addAction(m_hddMenuAction);
    connect(m_hddAction, SIGNAL(triggered()), this, SLOT(onHdd()));
    connect(m_hddMenuAction, SIGNAL(triggered()), this, SLOT(onHdd()));

    m_hddUnmountAction = new QAction(tr("Unmount"), this);
    m_hddMenu->addAction(m_hddUnmountAction);
    m_hddMenu->addSeparator();
    m_hddReadOnlyAction = new QAction(tr("Read only"), this);
    m_hddReadOnlyAction->setCheckable(true);
    m_hddMenu->addAction(m_hddReadOnlyAction);
    m_hddAutoMountAction = new QAction(tr("Auto mount on startup"), this);
    m_hddAutoMountAction->setCheckable(true);
    m_hddMenu->addAction(m_hddAutoMountAction);
    connect(m_hddUnmountAction, SIGNAL(triggered()), this, SLOT(onUnmountHdd()));
    connect(m_hddReadOnlyAction, SIGNAL(triggered()), this, SLOT(onReadOnlyHdd()));
    connect(m_hddAutoMountAction, SIGNAL(triggered()), this, SLOT(onAutoMountHdd()));

    m_hddMenu->addSeparator();
    for (int i = 0; i < LAST_FILES_QTY; i++) {
        m_hddLastFilesActions[i] = new QAction(this);
        m_hddMenu->addAction(m_hddLastFilesActions[i]);
        connect(m_hddLastFilesActions[i], SIGNAL(triggered()), this, SLOT(onHddLastFiles()));
    }

    fileMenu->addMenu(m_hddMenu);

    m_menuHddSeparator = fileMenu->addSeparator();

    // EDD menu
    m_eddMenu = new QMenu(tr("RAM Disk (EDD)"));
    m_eddMenu->setIcon(m_eddOffIcon);

    m_eddAction = new QAction(tr("Load and assign RAM Disk image..."), this);
    m_eddMenuAction = m_eddMenu->menuAction();
    m_eddMenuAction->setToolTip(tr("Load and assign RAM Disk image (Alt-E)"));
    QList<QKeySequence> eddKeyList;
    ADD_HOTKEY(eddKeyList, Qt::Key_E);
    m_eddAction->setShortcuts(eddKeyList);
    addAction(m_eddAction);
    m_eddMenu->addAction(m_eddAction);
    m_toolBar->addAction(m_eddMenuAction);
    connect(m_eddAction, SIGNAL(triggered()), this, SLOT(onEdd()));
    connect(m_eddMenuAction, SIGNAL(triggered()), this, SLOT(onEdd()));

    m_eddSaveAsAction = new QAction(tr("Save as..."), this);
    m_eddMenu->addAction(m_eddSaveAsAction);
    connect(m_eddSaveAsAction, SIGNAL(triggered()), this, SLOT(onEddSaveAs()));
    m_eddMenu->addSeparator();

    m_eddSaveAction = new QAction(tr("Save"), this);
    QList<QKeySequence> eddSaveKeyList;
    ADD_HOTKEY(eddSaveKeyList, Qt::Key_O);
    m_eddSaveAction->setShortcuts(eddSaveKeyList);
    m_eddMenu->addAction(m_eddSaveAction);
    connect(m_eddSaveAction, SIGNAL(triggered()), this, SLOT(onEddSave()));
    m_eddMenu->addSeparator();

    m_eddAutoLoadAction = new QAction(tr("Auto load on startup"), this);
    m_eddAutoLoadAction->setCheckable(true);
    m_eddMenu->addAction(m_eddAutoLoadAction);

    m_eddAutoSaveAction = new QAction(tr("Auto save on exit"), this);
    m_eddAutoSaveAction->setCheckable(true);
    m_eddMenu->addAction(m_eddAutoSaveAction);
    m_eddMenu->addSeparator();

    m_eddUnassignAction = new QAction(tr("Unassign"), this);
    m_eddMenu->addAction(m_eddUnassignAction);
    m_eddMenu->addSeparator();

    connect(m_eddUnassignAction, SIGNAL(triggered()), this, SLOT(onEddUnassign()));
    connect(m_eddAutoLoadAction, SIGNAL(triggered()), this, SLOT(onEddAutoLoad()));
    connect(m_eddAutoSaveAction, SIGNAL(triggered()), this, SLOT(onEddAutoSave()));

    for (int i = 0; i < LAST_FILES_QTY; i++) {
        m_eddLastFilesActions[i] = new QAction(this);
        m_eddMenu->addAction(m_eddLastFilesActions[i]);
        connect(m_eddLastFilesActions[i], SIGNAL(triggered()), this, SLOT(onEddLastFiles()));
    }

    fileMenu->addMenu(m_eddMenu);


    // EDD2 menu
    m_edd2Menu = new QMenu(tr("RAM Disk 2 (EDD2)"));
    m_edd2Menu->setIcon(m_edd2OffIcon);

    m_edd2Action = new QAction(tr("Load and assign RAM Disk 2 image..."), this);
    m_edd2MenuAction = m_edd2Menu->menuAction();
    m_edd2MenuAction->setToolTip(tr("Load and assign RAM Disk 2 image (Shift-Alt-E)"));
    QList<QKeySequence> edd2KeyList;
    ADD_HOTKEY(edd2KeyList, Qt::SHIFT | Qt::Key_E);
    m_edd2Action->setShortcuts(edd2KeyList);
    addAction(m_edd2Action);
    m_edd2Menu->addAction(m_edd2Action);
    m_toolBar->addAction(m_edd2MenuAction);
    connect(m_edd2Action, SIGNAL(triggered()), this, SLOT(onEdd2()));
    connect(m_edd2MenuAction, SIGNAL(triggered()), this, SLOT(onEdd2()));

    m_edd2SaveAsAction = new QAction(tr("Save as..."), this);
    m_edd2Menu->addAction(m_edd2SaveAsAction);
    connect(m_edd2SaveAsAction, SIGNAL(triggered()), this, SLOT(onEdd2SaveAs()));
    m_edd2Menu->addSeparator();

    m_edd2SaveAction = new QAction(tr("Save"), this);
    QList<QKeySequence> edd2SaveKeyList;
    ADD_HOTKEY(edd2SaveKeyList, Qt::SHIFT | Qt::Key_O);
    m_edd2SaveAction->setShortcuts(edd2SaveKeyList);
    m_edd2Menu->addAction(m_edd2SaveAction);
    connect(m_edd2SaveAction, SIGNAL(triggered()), this, SLOT(onEdd2Save()));
    m_edd2Menu->addSeparator();

    m_edd2AutoLoadAction = new QAction(tr("Auto load on startup"), this);
    m_edd2AutoLoadAction->setCheckable(true);
    m_edd2Menu->addAction(m_edd2AutoLoadAction);

    m_edd2AutoSaveAction = new QAction(tr("Auto save on exit"), this);
    m_edd2AutoSaveAction->setCheckable(true);
    m_edd2Menu->addAction(m_edd2AutoSaveAction);
    m_edd2Menu->addSeparator();

    m_edd2UnassignAction = new QAction(tr("Unassign"), this);
    m_edd2Menu->addAction(m_edd2UnassignAction);
    m_edd2Menu->addSeparator();

    connect(m_edd2UnassignAction, SIGNAL(triggered()), this, SLOT(onEdd2Unassign()));
    connect(m_edd2AutoLoadAction, SIGNAL(triggered()), this, SLOT(onEdd2AutoLoad()));
    connect(m_edd2AutoSaveAction, SIGNAL(triggered()), this, SLOT(onEdd2AutoSave()));

    for (int i = 0; i < LAST_FILES_QTY; i++) {
        m_edd2LastFilesActions[i] = new QAction(this);
        m_edd2Menu->addAction(m_edd2LastFilesActions[i]);
        connect(m_edd2LastFilesActions[i], SIGNAL(triggered()), this, SLOT(onEdd2LastFiles()));
    }

    fileMenu->addMenu(m_edd2Menu);


    m_menuEddSeparator = fileMenu->addSeparator();
    m_toolbarDiskSeparator = m_toolBar->addSeparator();


    // Exit
    m_exitAction = new QAction(tr("Exit"), this);
    m_exitAction->setToolTip(tr("Exit (Alt-X)"));
    QList<QKeySequence> exitKeysList;
    ADD_HOTKEY(exitKeysList, Qt::Key_X);
    //exitKeysList.append(QKeySequence(Qt::ALT + Qt::Key_X));
    //exitKeysList.append(QKeySequence(Qt::META + Qt::Key_X));
    m_exitAction->setShortcuts(exitKeysList);
    addAction(m_exitAction);
    fileMenu->addAction(m_exitAction);
    connect(m_exitAction, SIGNAL(triggered()), this, SLOT(onExit()));

    // Platform menu
    QMenu* platformMenu = m_menuBar->addMenu(tr("Platform"));

    m_platformListMenu = new QMenu(tr("Platform"), this);
    m_platformListMenu->setIcon(QIcon(":/icons/computer.png"));

    m_platformSelectAction = m_platformListMenu->menuAction();
    m_platformSelectAction->setToolTip(tr("Select platform (Alt-F9)"));
    QList<QKeySequence> platformKeyList;
    ADD_HOTKEY(platformKeyList, Qt::Key_F9);
    //platformKeyList.append(QKeySequence(Qt::ALT + Qt::Key_F9));
    //platformKeyList.append(QKeySequence(Qt::META + Qt::Key_F9));
    m_platformSelectAction->setShortcuts(platformKeyList);
    addAction(m_platformSelectAction);
    connect(m_platformSelectAction, SIGNAL(triggered()), this, SLOT(onPlatform()));
    // Select platform
    m_toolBar->addAction(m_platformSelectAction);

    platformMenu->addMenu(m_platformListMenu);
    // platformMenu->addAction(m_platformSelectAction);

    // Platform configuration
    m_platformConfigAction = new QAction(QIcon(":/icons/config.png"), tr("Platform configuration..."), this);
    m_platformConfigAction->setToolTip(tr("Configure current platform (Alt-F8)"));
    QList<QKeySequence> platformConfigKeysList;
    ADD_HOTKEY(platformConfigKeysList, Qt::Key_F8);
    //platformConfigKeysList.append(QKeySequence(Qt::ALT + Qt::Key_F8));
    //platformConfigKeysList.append(QKeySequence(Qt::META + Qt::Key_F8));
    m_platformConfigAction->setShortcuts(platformConfigKeysList);
    addAction(m_platformConfigAction);
    m_toolBar->addAction(m_platformConfigAction);
    platformMenu->addAction(m_platformConfigAction);
    connect(m_platformConfigAction, SIGNAL(triggered()), this, SLOT(onPlatformConfig()));
    platformMenu->addSeparator();

    // Reset
    m_resetAction = new QAction(QIcon(":/icons/reset.png"), tr("Reset"), this);
    m_resetAction->setToolTip(tr("Reset (Alt-F11)"));
    QList<QKeySequence> resetKeysList;
    ADD_HOTKEY(resetKeysList, Qt::Key_F11);
    //resetKeysList.append(QKeySequence(Qt::ALT + Qt::Key_F11));
    //resetKeysList.append(QKeySequence(Qt::META + Qt::Key_F11));
    m_resetAction->setShortcuts(resetKeysList);
    addAction(m_resetAction);
    platformMenu->addAction(m_resetAction);
    m_toolBar->addAction(m_resetAction);
    connect(m_resetAction, SIGNAL(triggered()), this, SLOT(onReset()));

    // Pause
    m_pauseAction = new QAction(QIcon(":/icons/pause.png"), tr("Pause"), this);
    m_pauseAction->setCheckable(true);
    m_pauseAction->setToolTip(tr("Pause (Pause, Alt-P)"));
    QList<QKeySequence> pauseKeysList;
    pauseKeysList.append(QKeySequence(Qt::Key_Pause));
    ADD_HOTKEY(pauseKeysList, Qt::Key_P);
    //pauseKeysList.append(QKeySequence(Qt::ALT + Qt::Key_P));
    //pauseKeysList.append(QKeySequence(Qt::META + Qt::Key_P));
    m_pauseAction->setShortcuts(pauseKeysList);
    addAction(m_pauseAction);
    platformMenu->addAction(m_pauseAction);
    m_toolBar->addAction(m_pauseAction);
    connect(m_pauseAction, SIGNAL(triggered()), this, SLOT(onPause()));

    // Fast forward
    QToolButton* forwardButton = new QToolButton(this);
    forwardButton->setFocusPolicy(Qt::NoFocus);
    forwardButton->setIcon(QIcon(":/icons/forward.png"));
    forwardButton->setToolTip(tr("Fast forward (End)"));
    m_toolBar->addWidget(forwardButton);
    connect(forwardButton, SIGNAL(pressed()), this, SLOT(onForwardOn()));
    connect(forwardButton, SIGNAL(released()), this, SLOT(onForwardOff()));

    // Full throttle
    QToolButton* fullThrottleButton = new QToolButton(this);
    fullThrottleButton->setFocusPolicy(Qt::NoFocus);
    fullThrottleButton->setIcon(QIcon(":/icons/full_throttle.png"));
    fullThrottleButton->setToolTip(tr("Full Throttle (Alt-End)"));
    m_toolBar->addWidget(fullThrottleButton);
    connect(fullThrottleButton, SIGNAL(pressed()), this, SLOT(onFullThrottleOn()));
    connect(fullThrottleButton, SIGNAL(released()), this, SLOT(onForwardOff()));

    platformMenu->addSeparator();

    // Speed up
    m_speedUpAction = new QAction(tr("Speed up"), this);
    m_speedUpAction->setToolTip(tr("Speed up (Alt-PgUp)"));
    QList<QKeySequence> speedUpKeysList;
    ADD_HOTKEY(speedUpKeysList, Qt::Key_PageUp);
    m_speedUpAction->setShortcuts(speedUpKeysList);
    addAction(m_speedUpAction);
    platformMenu->addAction(m_speedUpAction);
    connect(m_speedUpAction, SIGNAL(triggered()), this, SLOT(onSpeedUp()));

    // Speed down
    m_speedDownAction = new QAction(tr("Speed down"), this);
    m_speedDownAction->setToolTip(tr("Speed down (Alt-PgDn)"));
    QList<QKeySequence> speedDownKeysList;
    ADD_HOTKEY(speedDownKeysList, Qt::Key_PageDown);
    m_speedDownAction->setShortcuts(speedDownKeysList);
    addAction(m_speedDownAction);
    platformMenu->addAction(m_speedDownAction);
    connect(m_speedDownAction, SIGNAL(triggered()), this, SLOT(onSpeedDown()));

    // Speed up (fine)
    m_speedUpFineAction = new QAction(tr("Speed up (fine)"), this);
    m_speedUpFineAction->setToolTip(tr("Fine speed up (Alt-Up)"));
    QList<QKeySequence> speedUpFineKeysList;
    ADD_HOTKEY(speedUpFineKeysList, Qt::Key_Up);
    m_speedUpFineAction->setShortcuts(speedUpFineKeysList);
    addAction(m_speedUpFineAction);
    platformMenu->addAction(m_speedUpFineAction);
    connect(m_speedUpFineAction, SIGNAL(triggered()), this, SLOT(onSpeedUpFine()));

    // Speed down (fine)
    m_speedDownFineAction = new QAction(tr("Speed down (fine)"), this);
    m_speedDownFineAction->setToolTip(tr("Fine speed down (Alt-Down)"));
    QList<QKeySequence> speedDownFineKeysList;
    ADD_HOTKEY(speedDownFineKeysList, Qt::Key_Down);
    m_speedDownFineAction->setShortcuts(speedDownFineKeysList);
    addAction(m_speedDownFineAction);
    platformMenu->addAction(m_speedDownFineAction);
    connect(m_speedDownFineAction, SIGNAL(triggered()), this, SLOT(onSpeedDownFine()));

    // Speed normal
    m_speedNormalAction = new QAction(tr("Normal speed"), this);
    m_speedNormalAction->setToolTip(tr("Normal speed (Alt-Home)"));
    QList<QKeySequence> speedNormalKeysList;
    ADD_HOTKEY(speedNormalKeysList, Qt::Key_Home);
    m_speedNormalAction->setShortcuts(speedNormalKeysList);
    addAction(m_speedNormalAction);
    platformMenu->addAction(m_speedNormalAction);
    connect(m_speedNormalAction, SIGNAL(triggered()), this, SLOT(onSpeedNormal()));

    platformMenu->addSeparator();

    // Debug
    m_debugAction = new QAction(QIcon(m_darkTheme ? ":/icons/debug-dark.png" : ":/icons/debug.png"), tr("Debug..."), this);
    m_debugAction->setToolTip(tr("Debug (Alt-D)"));
    QList<QKeySequence> debugKeysList;
    ADD_HOTKEY(debugKeysList, Qt::Key_D);
    //debugKeysList.append(QKeySequence(Qt::ALT + Qt::Key_D));
    //debugKeysList.append(QKeySequence(Qt::META + Qt::Key_D));
    m_debugAction->setShortcuts(debugKeysList);
    addAction(m_debugAction);
    platformMenu->addAction(m_debugAction);
    m_toolBar->addAction(m_debugAction);
    connect(m_debugAction, SIGNAL(triggered()), this, SLOT(onDebug()));

    m_toolBar->addSeparator();

    // Settings menu
    QMenu* settingsMenu = m_menuBar->addMenu(tr("Settings"));

    // Settings
    m_settingsAction = new QAction(QIcon(":/icons/settings.png"), tr("Emulator settings..."), this);
    m_settingsAction->setToolTip(tr("Emulator settings (Alt-F12)"));
    QList<QKeySequence> settingsKeysList;
    ADD_HOTKEY(settingsKeysList, Qt::Key_F12);
    //settingsKeysList.append(QKeySequence(Qt::ALT + Qt::Key_F12));
    //settingsKeysList.append(QKeySequence(Qt::META + Qt::Key_F12));
    m_settingsAction->setShortcuts(settingsKeysList);
    addAction(m_settingsAction);
    settingsMenu->addAction(m_settingsAction);
    m_toolBar->addAction(m_settingsAction);
    connect(m_settingsAction, SIGNAL(triggered()), this, SLOT(onSettings()));

    // Screenshot
    m_screenshotAction = new QAction(QIcon(":/icons/screenshot.png"), tr("Take screenshot..."), this);
    m_screenshotAction->setToolTip(tr("Take screenshot (Alt-H)"));
    QList<QKeySequence> screenshotKeysList;
    ADD_HOTKEY(screenshotKeysList, Qt::Key_H);
    //screenshotKeysList.append(QKeySequence(Qt::ALT + Qt::Key_H));
    //screenshotKeysList.append(QKeySequence(Qt::META + Qt::Key_H));
    m_screenshotAction->setShortcuts(screenshotKeysList);
    addAction(m_screenshotAction);
    m_toolBar->addAction(m_screenshotAction);
    connect(m_screenshotAction, SIGNAL(triggered()), this, SLOT(onScreenshot()));

    // Copy screen to clipboard
    m_copyImageAction = new QAction(tr("Copy screenshot"), this);
    m_copyImageAction->setToolTip(tr("Copy screenshot to clipboard (Alt-Ins)"));
    QList<QKeySequence> copyImageKeysList;
    ADD_HOTKEY(copyImageKeysList, Qt::Key_Insert);
    //copyImageKeysList.append(QKeySequence(Qt::ALT + Qt::Key_Insert));
    //copyImageKeysList.append(QKeySequence(Qt::META + Qt::Key_Insert));
    m_copyImageAction->setShortcuts(copyImageKeysList);
    addAction(m_copyImageAction);
    connect(m_copyImageAction, SIGNAL(triggered()), this, SLOT(onCopyImage()));

    // Copy text screen to clipboard
    m_copyTextAction = new QAction(tr("Copy text screen"), this);
    //m_copyTextAction->setToolTip(tr("Copy text screen to clipboard (Alt-Shift-Ins)"));
    QList<QKeySequence> copyTextKeysList;
    ADD_HOTKEY(copyTextKeysList, Qt::SHIFT | Qt::Key_Insert);
    //copyTextKeysList.append(QKeySequence(Qt::ALT + Qt::SHIFT | Qt::Key_Insert));
    //copyTextKeysList.append(QKeySequence(Qt::META + Qt::SHIFT | Qt::Key_Insert));
    m_copyTextAction->setShortcuts(copyTextKeysList);
    addAction(m_copyTextAction);
    connect(m_copyTextAction, SIGNAL(triggered()), this, SLOT(onCopyText()));

    // Paste text from clipboard
    m_pasteAction = new QAction(tr("Paste text"), this);
    //m_pasteAction->setToolTip(tr("Paste text from clipboard (Alt-Shift-V)"));
    QList<QKeySequence> pasteKeysList;
    ADD_HOTKEY(pasteKeysList, Qt::SHIFT | Qt::Key_V);
    m_pasteAction->setShortcuts(pasteKeysList);
    addAction(m_pasteAction);
    connect(m_pasteAction, SIGNAL(triggered()), this, SLOT(onPaste()));

    m_toolBar->addSeparator();

    settingsMenu->addSeparator();

    // Keyboard layout
    QMenu* layoutMenu = new QMenu(tr("Keyboard layout"), this);
    QActionGroup* layoutGroup = new QActionGroup(layoutMenu);

    m_qwertyAction = new QAction("Qwerty", this);
    m_qwertyAction->setCheckable(true);
    m_qwertyAction->setToolTip("Qwerty (Alt-Q)");
    QList<QKeySequence> qwertyKeyList;
    ADD_HOTKEY(qwertyKeyList, Qt::Key_Q);
    //qwertyKeyList.append(QKeySequence(Qt::ALT + Qt::Key_Q));
    //qwertyKeyList.append(QKeySequence(Qt::META + Qt::Key_Q));
    m_qwertyAction->setShortcuts(qwertyKeyList);
    addAction(m_qwertyAction);
    layoutMenu->addAction(m_qwertyAction);
    layoutGroup->addAction(m_qwertyAction);

    m_jcukenAction = new QAction(tr("Jcuken"), this);
    m_jcukenAction->setCheckable(true);
    m_jcukenAction->setToolTip(tr("Jcuken (Alt-J)"));
    QList<QKeySequence> jcukenKeyList;
    ADD_HOTKEY(jcukenKeyList, Qt::Key_J);
    //jcukenKeyList.append(QKeySequence(Qt::ALT + Qt::Key_J));
    //jcukenKeyList.append(QKeySequence(Qt::META + Qt::Key_J));
    m_jcukenAction->setShortcuts(jcukenKeyList);
    addAction(m_jcukenAction);
    layoutMenu->addAction(m_jcukenAction);
    layoutGroup->addAction(m_jcukenAction);

    m_smartAction = new QAction(tr("Smart"), this);
    m_smartAction->setCheckable(true);
    m_smartAction->setToolTip(tr("Smart (Alt-K)"));
    QList<QKeySequence> smartKeyList;
    ADD_HOTKEY(smartKeyList, Qt::Key_K);
    //smartKeyList.append(QKeySequence(Qt::ALT + Qt::Key_K));
    //smartKeyList.append(QKeySequence(Qt::META + Qt::Key_K));
    m_smartAction->setShortcuts(smartKeyList);
    addAction(m_smartAction);
    layoutMenu->addAction(m_smartAction);
    layoutGroup->addAction(m_smartAction);

    connect(m_qwertyAction, SIGNAL(triggered()), this, SLOT(onQwerty()));
    connect(m_jcukenAction, SIGNAL(triggered()), this, SLOT(onJcuken()));
    connect(m_smartAction, SIGNAL(triggered()), this, SLOT(onSmart()));
    settingsMenu->addMenu(layoutMenu);

    m_layoutButton = new QToolButtonWA(this);
    m_layoutButton->setFocusPolicy(Qt::NoFocus);
    //m_layoutButton->setText("Qwerty ");
    m_layoutButton->setIcon(QIcon(m_darkTheme ? ":/icons/keyboard-dark.png" : ":/icons/keyboard.png"));
    m_layoutButton->setToolTip(tr("Keyboard layout"));
    m_layoutButton->setMenu(layoutMenu);
    m_layoutButton->setPopupMode(QToolButton::InstantPopup);
    m_toolBar->addWidget(m_layoutButton);

    // Printer capture on/off
    m_printerCaptureAction = new QAction(QIcon(":/icons/printer.png"), tr("Printer capture"), this);
    m_printerCaptureAction->setCheckable(true);
    m_printerCaptureAction->setToolTip(tr("Capture printer output (Shift-Alt-P)"));
    QList<QKeySequence> printerCaptureKeyList;
    ADD_HOTKEY(printerCaptureKeyList, Qt::SHIFT | Qt::Key_P);
    m_printerCaptureAction->setShortcuts(printerCaptureKeyList);
    addAction(m_printerCaptureAction);
    settingsMenu->addAction(m_printerCaptureAction);
    //m_toolBar->addAction(m_printerCaptureAction);
    connect(m_printerCaptureAction, SIGNAL(triggered()), this, SLOT(onPrinterCapture()));

    // Tape hook on/off
    m_tapeHookAction = new QAction(QIcon(":/icons/tape.png"), tr("Tape hook"), this);
    m_tapeHookAction->setCheckable(true);
    m_tapeHookAction->setToolTip(tr("Tape hook (Alt-T)"));
    QList<QKeySequence> tapeHookKeyList;
    ADD_HOTKEY(tapeHookKeyList, Qt::Key_T);
    m_tapeHookAction->setShortcuts(tapeHookKeyList);
    addAction(m_tapeHookAction);
    settingsMenu->addAction(m_tapeHookAction);
    m_toolBar->addAction(m_tapeHookAction);
    connect(m_tapeHookAction, SIGNAL(triggered()), this, SLOT(onTapeHook()));

    // Tape mute on/off
    m_muteTapeAction = new QAction(QIcon(":/icons/mute_tape.png"), tr("Mute tape output"), this);
    m_muteTapeAction->setCheckable(true);
    m_muteTapeAction->setToolTip(tr("Mute tape output"));
    settingsMenu->addAction(m_muteTapeAction);
    connect(m_muteTapeAction, SIGNAL(triggered()), this, SLOT(onMuteTape()));

    // Color mode
    m_colorModeMenu = new QMenu(tr("Color mode"), this);
    QActionGroup* colorModeGroup = new QActionGroup(m_colorModeMenu);

    m_colorMonoOrigAction = new QAction(QIcon(":/icons/bw.png"), tr("Black && white original"), this);
    m_colorMonoOrigAction->setCheckable(true);
    m_colorMonoOrigAction->setData("original");
    m_colorModeMenu->addAction(m_colorMonoOrigAction);
    colorModeGroup->addAction(m_colorMonoOrigAction);
    connect(m_colorMonoOrigAction, SIGNAL(triggered()), this, SLOT(onColorSelect()));

    m_colorMonoAction = new QAction(QIcon(":/icons/bw.png"), tr("Black && white"), this);
    m_colorMonoAction->setCheckable(true);
    m_colorMonoAction->setData("mono");
    m_colorModeMenu->addAction(m_colorMonoAction);
    colorModeGroup->addAction(m_colorMonoAction);
    connect(m_colorMonoAction, SIGNAL(triggered()), this, SLOT(onColorSelect()));

    m_colorColor1Action = new QAction(QIcon(":/icons/color.png"), "Color 1", this);
    m_colorColor1Action->setCheckable(true);
    //m_colorColor1Action->setData("color1");
    m_colorModeMenu->addAction(m_colorColor1Action);
    colorModeGroup->addAction(m_colorColor1Action);
    connect(m_colorColor1Action, SIGNAL(triggered()), this, SLOT(onColorSelect()));

    m_colorColor2Action = new QAction(QIcon(":/icons/color.png"),"Color 2", this);
    m_colorColor2Action->setCheckable(true);
    //m_colorColor2Action->setData("color2");
    m_colorModeMenu->addAction(m_colorColor2Action);
    colorModeGroup->addAction(m_colorColor2Action);
    connect(m_colorColor2Action, SIGNAL(triggered()), this, SLOT(onColorSelect()));

    m_colorColor3Action = new QAction(QIcon(":/icons/color.png"),"Color 3", this);
    m_colorColor3Action->setCheckable(true);
    //m_colorColor3Action->setData("color2");
    m_colorModeMenu->addAction(m_colorColor3Action);
    colorModeGroup->addAction(m_colorColor3Action);
    connect(m_colorColor3Action, SIGNAL(triggered()), this, SLOT(onColorSelect()));

    m_colorGrayscaleAction = new QAction(QIcon(":/icons/gray.png"), tr("Grayscale"), this);
    m_colorGrayscaleAction->setCheckable(true);
    m_colorGrayscaleAction->setData("grayscale");
    m_colorModeMenu->addAction(m_colorGrayscaleAction);
    colorModeGroup->addAction(m_colorGrayscaleAction);
    connect(m_colorGrayscaleAction, SIGNAL(triggered()), this, SLOT(onColorSelect()));

    m_colorModeMenu->setIcon(QIcon(":/icons/colormode.png"));
    m_colorMenuAction = m_colorModeMenu->menuAction();
    m_colorMenuAction->setToolTip(tr("Color mode (Alt-C)"));
    addAction(m_colorMenuAction);
    m_toolBar->addAction(m_colorMenuAction);
    settingsMenu->addAction(m_colorMenuAction);

    QList<QKeySequence> colorKeysList;
    ADD_HOTKEY(colorKeysList, Qt::Key_C);
    //colorKeysList.append(QKeySequence(Qt::ALT + Qt::Key_C));
    //colorKeysList.append(QKeySequence(Qt::META + Qt::Key_C));
    m_colorMenuAction->setShortcuts(colorKeysList);

    settingsMenu->addAction(m_colorMenuAction);
    connect(m_colorMenuAction, SIGNAL(triggered()), this, SLOT(onColorMode()));

    // Crop
    m_cropAction = new QAction(QIcon(m_darkTheme ? ":/icons/crop-dark.png" : ":/icons/crop.png"), tr("Visible area"), this);
    m_cropAction->setCheckable(true);
    m_cropAction->setToolTip(tr("Show only visible area (Alt-V)"));
    QList<QKeySequence> cropKeysList;
    ADD_HOTKEY(cropKeysList, Qt::Key_V);
    //cropKeysList.append(QKeySequence(Qt::ALT + Qt::Key_V));
    //cropKeysList.append(QKeySequence(Qt::META + Qt::Key_V));
    m_cropAction->setShortcuts(cropKeysList);
    addAction(m_cropAction);
    m_toolBar->addAction(m_cropAction);
    connect(m_cropAction, SIGNAL(triggered()), this, SLOT(onCrop()));
    settingsMenu->addAction(m_cropAction);

    // Aspect
    m_aspectAction = new QAction(QIcon(m_darkTheme ? ":/icons/aspect-dark.png" : ":/icons/aspect.png"), tr("Aspect"), this);
    m_aspectAction->setCheckable(true);
    m_aspectAction->setToolTip(tr("Original aspect ratio (Alt-R)"));
    QList<QKeySequence> aspectKeysList;
    ADD_HOTKEY(aspectKeysList, Qt::Key_R);
    //aspectKeysList.append(QKeySequence(Qt::ALT + Qt::Key_R));
    //aspectKeysList.append(QKeySequence(Qt::META + Qt::Key_R));
    m_aspectAction->setShortcuts(aspectKeysList);
    addAction(m_aspectAction);
    m_toolBar->addAction(m_aspectAction);
    connect(m_aspectAction, SIGNAL(triggered()), this, SLOT(onAspect()));
    settingsMenu->addAction(m_aspectAction);

    // Wide screen
    m_wideScreenAction = new QAction(QIcon(m_darkTheme ? ":/icons/wide-dark.png" : ":/icons/wide.png"), tr("Wide screen (16:9)"), this);
    m_wideScreenAction->setCheckable(true);
    m_wideScreenAction->setToolTip(tr("Wide screen (16:9) (Alt-N)"));
    QList<QKeySequence> wideScreenKeysList;
    ADD_HOTKEY(wideScreenKeysList, Qt::Key_N);
    //wideScreenKeysList.append(QKeySequence(Qt::ALT + Qt::Key_N));
    //wideScreenKeysList.append(QKeySequence(Qt::META + Qt::Key_N));
    m_wideScreenAction->setShortcuts(wideScreenKeysList);
    addAction(m_wideScreenAction);
    //m_toolBar->addAction(m_wideScreenAction);
    connect(m_wideScreenAction, SIGNAL(triggered()), this, SLOT(onWideScreen()));
    settingsMenu->addAction(m_wideScreenAction);

    // Smoothing
    m_smoothingMenu = new QMenu(tr("Smoothing / postprocessing"), this);
    QActionGroup* smoothingGroup = new QActionGroup(m_smoothingMenu);

    m_smoothingNearestAction = new QAction(m_smoothingNearestIcon, tr("Nearest"), this);
    m_smoothingNearestAction->setCheckable(true);
    m_smoothingNearestAction->setData("nearest");
    m_smoothingMenu->addAction(m_smoothingNearestAction);
    smoothingGroup->addAction(m_smoothingNearestAction);
    connect(m_smoothingNearestAction, SIGNAL(triggered()), this, SLOT(onSmoothingSelect()));

    m_smoothingSharpAction = new QAction(m_smoothingSharpIcon, tr("Sharp (pixel edges)"), this);
    m_smoothingSharpAction->setCheckable(true);
    m_smoothingSharpAction->setData("sharp");
    m_smoothingMenu->addAction(m_smoothingSharpAction);
    smoothingGroup->addAction(m_smoothingSharpAction);
    connect(m_smoothingSharpAction, SIGNAL(triggered()), this, SLOT(onSmoothingSelect()));

    m_smoothingBilinearAction = new QAction(m_smoothingBilinearIcon, tr("Bilinear"), this);
    m_smoothingBilinearAction->setCheckable(true);
    m_smoothingBilinearAction->setData("bilinear");
    m_smoothingMenu->addAction(m_smoothingBilinearAction);
    smoothingGroup->addAction(m_smoothingBilinearAction);
    connect(m_smoothingBilinearAction, SIGNAL(triggered()), this, SLOT(onSmoothingSelect()));

    m_smoothingShaderAction = new QAction(m_shaderIcon, tr("Custom shader"), this);
    m_smoothingShaderAction->setCheckable(true);
    m_smoothingShaderAction->setData("custom");
    m_smoothingMenu->addAction(m_smoothingShaderAction);
    smoothingGroup->addAction(m_smoothingShaderAction);
    connect(m_smoothingShaderAction, SIGNAL(triggered()), this, SLOT(onSmoothingSelect()));

    m_smoothingMenu->addSeparator();

    m_desaturateAction = new QAction(/*m_desaturateIcon,*/ tr("Desaturate"), this);
    m_desaturateAction->setCheckable(true);
    m_smoothingMenu->addAction(m_desaturateAction);
    connect(m_desaturateAction, SIGNAL(triggered()), this, SLOT(onDesaturate()));

    m_smoothingMenu->setIcon(QIcon(":/icons/smooth.png"));
    m_smoothingAction = m_smoothingMenu->menuAction();
    m_smoothingAction->setToolTip(tr("Smoothing mode (Alt-S)"));
    addAction(m_smoothingAction);
    m_toolBar->addAction(m_smoothingAction);
    settingsMenu->addAction(m_smoothingAction);

    m_shaderListMenu = new QMenu(tr("Custom shaders"), m_smoothingMenu);
    m_shaderListMenu->menuAction()->setCheckable(true);
    m_shaderListMenu->setIcon(m_shaderIcon);
    m_smoothingMenu->addSeparator();
    m_smoothingMenu->addMenu(m_shaderListMenu);
    smoothingGroup->addAction(m_shaderListMenu->menuAction());

    QList<QKeySequence> smoothingKeyList;
    ADD_HOTKEY(smoothingKeyList, Qt::Key_S);
    m_smoothingAction->setShortcuts(smoothingKeyList);

    //settingsMenu->addAction(m_smoothingAction);
    connect(m_smoothingAction, SIGNAL(triggered()), this, SLOT(onSmoothing()));

    // Font
    m_fontAction = new QAction(QIcon(m_darkTheme ? ":/icons/font-dark.png" : ":/icons/font.png"), tr("Font"), this);
    m_fontAction->setCheckable(true);
    m_fontAction->setToolTip(tr("Advanced font (Alt-F)"));
    QList<QKeySequence> fontKeysList;
    ADD_HOTKEY(fontKeysList, Qt::Key_F);
    //fontKeysList.append(QKeySequence(Qt::ALT + Qt::Key_F));
    //fontKeysList.append(QKeySequence(Qt::META + Qt::Key_F));
    m_fontAction->setShortcuts(fontKeysList);
    //platformMenu->addAction(m_fontAction);
    addAction(m_fontAction);
    m_toolBar->addAction(m_fontAction);
    settingsMenu->addAction(m_fontAction);
    connect(m_fontAction, SIGNAL(triggered()), this, SLOT(onFont()));

    // Mute tape output toolbar button
    //m_toolBar->addAction(m_muteTapeAction);

    // Fast Reset
    m_fastResetAction = new QAction(QIcon(":/icons/fast_reset.png"), tr("Fast Reset"), this);
    m_fastResetAction->setCheckable(true);
    m_fastResetAction->setToolTip(tr("Fast Reset (Alt-U)"));
    QList<QKeySequence> fastResetKeyList;
    ADD_HOTKEY(fastResetKeyList, Qt::Key_U);
    m_fastResetAction->setShortcuts(fastResetKeyList);
    addAction(m_fastResetAction);
    settingsMenu->addAction(m_fastResetAction);
    connect(m_fastResetAction, SIGNAL(triggered()), this, SLOT(onFastReset()));

    // Mute
    m_muteAction = new QAction(QIcon(m_darkTheme ? ":/icons/mute-dark.png" : ":/icons/mute.png"), tr("Mute"), this);
    m_muteAction->setCheckable(true);
    m_muteAction->setToolTip(tr("Mute (Alt-M)"));
    QList<QKeySequence> muteKeysList;
    ADD_HOTKEY(muteKeysList, Qt::Key_M);
    //muteKeysList.append(QKeySequence(Qt::ALT + Qt::Key_M));
    //muteKeysList.append(QKeySequence(Qt::META + Qt::Key_M));
    m_muteAction->setShortcuts(muteKeysList);
    addAction(m_muteAction);
    settingsMenu->addAction(m_muteAction);
    connect(m_muteAction, SIGNAL(triggered()), this, SLOT(onMute()));

    QMenu* viewMenu = m_menuBar->addMenu(tr("View"));

    m_presetMenu = new QMenu (tr("Window size"), this);
    QActionGroup* presetGroup = new QActionGroup(m_presetMenu);
    m_presetMenu->setIcon(m_presetIcon);

    // 1x preset
    m_preset1xAction = new QAction(m_1xIcon, tr("Fixed: 1x"), this);
    //m_preset1xAction->setToolTip(tr("Preset: 1x (Alt-1)"));
    m_preset1xAction->setCheckable(true);
    QList<QKeySequence> preset1xKeysList;
    ADD_HOTKEY(preset1xKeysList, Qt::Key_1);
    //preset1xKeysList.append(QKeySequence(Qt::ALT + Qt::Key_1));
    //preset1xKeysList.append(QKeySequence(Qt::META + Qt::Key_1));
    presetGroup->addAction(m_preset1xAction);
    m_preset1xAction->setShortcuts(preset1xKeysList);
    addAction(m_preset1xAction);
    m_presetMenu->addAction(m_preset1xAction);
    connect(m_preset1xAction, SIGNAL(triggered()), this, SLOT(on1x()));

    // 2x preset
    m_preset2xAction = new QAction(m_2xIcon, tr("Fixed: 2x"), this);
    //m_preset2xAction->setToolTip(tr("Preset: 2x (Alt-2)"));
    m_preset2xAction->setCheckable(true);
    QList<QKeySequence> preset2xKeysList;
    ADD_HOTKEY(preset2xKeysList, Qt::Key_2);
    //preset2xKeysList.append(QKeySequence(Qt::ALT + Qt::Key_2));
    //preset2xKeysList.append(QKeySequence(Qt::META + Qt::Key_2));
    presetGroup->addAction(m_preset2xAction);
    m_preset2xAction->setShortcuts(preset2xKeysList);
    addAction(m_preset2xAction);
    m_presetMenu->addAction(m_preset2xAction);
    connect(m_preset2xAction, SIGNAL(triggered()), this, SLOT(on2x()));

    // 3x preset
    m_preset3xAction = new QAction(m_3xIcon, tr("Fixed: 3x"), this);
    //m_preset3xAction->setToolTip(tr("Preset: 3x (Alt-3)"));
    QList<QKeySequence> preset3xKeysList;
    ADD_HOTKEY(preset3xKeysList, Qt::Key_3);
    m_preset3xAction->setCheckable(true);
    //preset3xKeysList.append(QKeySequence(Qt::ALT + Qt::Key_3));
    //preset3xKeysList.append(QKeySequence(Qt::META + Qt::Key_3));
    presetGroup->addAction(m_preset3xAction);
    m_preset3xAction->setShortcuts(preset3xKeysList);
    addAction(m_preset3xAction);
    m_presetMenu->addAction(m_preset3xAction);
    connect(m_preset3xAction, SIGNAL(triggered()), this, SLOT(on3x()));

    // 4x preset
    m_preset4xAction = new QAction(m_4xIcon, tr("Fixed: 4x"), this);
    //m_preset4xAction->setToolTip(tr("Preset: 4x (Alt-4)"));
    m_preset4xAction->setCheckable(true);
    QList<QKeySequence> preset4xKeysList;
    ADD_HOTKEY(preset4xKeysList, Qt::Key_4);
    //preset4xKeysList.append(QKeySequence(Qt::ALT + Qt::Key_4));
    //preset4xKeysList.append(QKeySequence(Qt::META + Qt::Key_4));
    presetGroup->addAction(m_preset4xAction);
    m_preset4xAction->setShortcuts(preset4xKeysList);
    addAction(m_preset4xAction);
    m_presetMenu->addAction(m_preset4xAction);
    connect(m_preset4xAction, SIGNAL(triggered()), this, SLOT(on4x()));

    // 5x preset
    m_preset5xAction = new QAction(m_5xIcon, tr("Fixed: 5x"), this);
    //m_preset5xAction->setToolTip(tr("Preset: 5x (Alt-5)"));
    m_preset5xAction->setCheckable(true);
    QList<QKeySequence> preset5xKeysList;
    ADD_HOTKEY(preset5xKeysList, Qt::Key_5);
    //preset5xKeysList.append(QKeySequence(Qt::ALT + Qt::Key_5));
    //preset5xKeysList.append(QKeySequence(Qt::META + Qt::Key_5));
    presetGroup->addAction(m_preset5xAction);
    m_preset5xAction->setShortcuts(preset5xKeysList);
    addAction(m_preset5xAction);
    m_presetMenu->addAction(m_preset5xAction);
    connect(m_preset5xAction, SIGNAL(triggered()), this, SLOT(on5x()));

    m_presetMenu->addSeparator();

    // 1.5x preset
    m_preset15xAction = new QAction(m_15xIcon, tr("Fixed: 1.5x"), this);
    m_preset15xAction->setCheckable(true);
    QList<QKeySequence> preset15xKeysList;
    ADD_HOTKEY(preset15xKeysList, Qt::Key_8);
    presetGroup->addAction(m_preset15xAction);
    m_preset15xAction->setShortcuts(preset15xKeysList);
    addAction(m_preset15xAction);
    m_presetMenu->addAction(m_preset15xAction);
    connect(m_preset15xAction, SIGNAL(triggered()), this, SLOT(on15x()));

    // 2.5x preset
    m_preset25xAction = new QAction(m_25xIcon, tr("Fixed: 2.5x"), this);
    m_preset25xAction->setCheckable(true);
    QList<QKeySequence> preset25xKeysList;
    ADD_HOTKEY(preset25xKeysList, Qt::Key_9);
    presetGroup->addAction(m_preset25xAction);
    m_preset25xAction->setShortcuts(preset25xKeysList);
    addAction(m_preset25xAction);
    m_presetMenu->addAction(m_preset25xAction);
    connect(m_preset25xAction, SIGNAL(triggered()), this, SLOT(on25x()));
    m_presetMenu->addSeparator();

    // Fit preset
    m_presetFitAction = new QAction(m_resizableIcon, tr("Resizable"), this);
    //m_presetFitAction->setToolTip(tr("Preset: Fit (Alt-0)"));
    m_presetFitAction->setCheckable(true);
    QList<QKeySequence> presetFitKeysList;
    ADD_HOTKEY(presetFitKeysList, Qt::Key_0);
    //presetFitKeysList.append(QKeySequence(Qt::ALT + Qt::Key_0));
    //presetFitKeysList.append(QKeySequence(Qt::META + Qt::Key_0));
    presetGroup->addAction(m_presetFitAction);
    m_presetFitAction->setShortcuts(presetFitKeysList);
    addAction(m_presetFitAction);
    m_presetMenu->addAction(m_presetFitAction);
    connect(m_presetFitAction, SIGNAL(triggered()), this, SLOT(onFit()));

    m_presetAction = m_presetMenu->menuAction();
    m_presetAction->setToolTip(tr("Window size"));
    addAction(m_presetAction);
    m_presetAction->setMenu(m_presetMenu);
    m_toolBar->addAction(m_presetAction);
    settingsMenu->addAction(m_presetAction);
    /*QList<QKeySequence> presetKeyList;
    ADD_HOTKEY(presetKeyList, Qt::Key_);
    m_presetAction->setShortcuts(presetKeyList);*/
    connect(m_presetAction, SIGNAL(triggered()), this, SLOT(onPreset()));

    viewMenu->addSeparator();
    viewMenu->addAction(m_screenshotAction);
    viewMenu->addAction(m_copyImageAction);
    viewMenu->addAction(m_copyTextAction);
    viewMenu->addAction(m_pasteAction);
    viewMenu->addSeparator();

    m_fullscreenAction = new QAction(tr("Fullscreen mode"), this);
    m_fullscreenAction->setCheckable(true);
    //setToolTip(tr("Fullscreen mode (Alt-Enter)"));
    QList<QKeySequence> fullscreenKeysList;
    ADD_HOTKEY(fullscreenKeysList, Qt::Key_Return);
    //fullscreenKeysList.append(QKeySequence(Qt::ALT + Qt::Key_Return));
    //fullscreenKeysList.append(QKeySequence(Qt::META + Qt::Key_Return));
    m_fullscreenAction->setShortcuts(fullscreenKeysList);
    connect(m_fullscreenAction, SIGNAL(triggered()), this, SLOT(onFullscreen()));
    addAction(m_fullscreenAction);
    viewMenu->addAction(m_fullscreenAction);

    m_fullwindowAction = new QAction(tr("Full-windowed mode"), this);
    m_fullwindowAction->setCheckable(true);
    //setToolTip(tr("Hide menu and buttons (Alt-\\)"));
    QList<QKeySequence> fullwindowKeysList;
    ADD_HOTKEY(fullwindowKeysList, Qt::Key_Backslash);
    //fullwindowKeysList.append(QKeySequence(Qt::ALT + Qt::Key_Backslash));
    //fullwindowKeysList.append(QKeySequence(Qt::META + Qt::Key_Backslash));
    m_fullwindowAction->setShortcuts(fullwindowKeysList);
    connect(m_fullwindowAction, SIGNAL(triggered()), this, SLOT(onFullwindow()));
    addAction(m_fullwindowAction);
    viewMenu->addAction(m_fullwindowAction);

    m_toolBar->addAction(m_fastResetAction);
    m_toolBar->addAction(m_muteAction);

    QMenu* helpMenu = m_menuBar->addMenu(tr("Help"));

    m_platformHelpAction = new QAction(tr("Platform help..."), this);
    QList<QKeySequence> platformHelpKeysList;
    ADD_HOTKEY(platformHelpKeysList, Qt::Key_F1);
    //platformHelpKeysList.append(QKeySequence(Qt::ALT + Qt::Key_F1));
    //platformHelpKeysList.append(QKeySequence(Qt::META + Qt::Key_F1));
    m_platformHelpAction->setShortcuts(platformHelpKeysList);
    m_platformHelpAction->setToolTip(tr("Show help on current platform"));
    helpMenu->addAction(m_platformHelpAction);
    connect(m_platformHelpAction, SIGNAL(triggered()), this, SLOT(onPlatformHelp()));

    helpMenu->addSeparator();

    m_aboutAction = new QAction(tr("About..."), this);
    m_aboutAction->setToolTip(tr("About"));
    helpMenu->addAction(m_aboutAction);
    connect(m_aboutAction, SIGNAL(triggered()), this, SLOT(onAbout()));

    settingsMenu->addSeparator();

    // Autosave settings
    m_autosaveAction = new QAction(tr("Auto save settings"), this);
    m_autosaveAction->setCheckable(true);
    m_autosaveAction->setChecked(true);
    m_autosaveAction->setEnabled(false);
    settingsMenu->addAction(m_autosaveAction);

    settingsMenu->addSeparator();

    // Reset settings
    QMenu* resetMenu = new QMenu(tr("Reset settings"), this);
    QAction* resetPlatformAction = new QAction(tr("Current platform and common settings..."), this);
    QAction* resetAllAction = new QAction(tr("All settings..."), this);
    resetMenu->addAction(resetPlatformAction);
    resetMenu->addAction(resetAllAction);
    connect(resetPlatformAction, SIGNAL(triggered()), this, SLOT(onResetPlatform()));
    connect(resetAllAction, SIGNAL(triggered()), this, SLOT(onResetAll()));
    settingsMenu->addSeparator();
    settingsMenu->addMenu(resetMenu);
}


void MainWindow::tuneMenu()
{
    // Color mode
    bool hasColor = false;
    std::string platformGroup = getPlatformGroupName();

    m_speedLabel->setVisible(false);

    m_colorColor3Action->setVisible(false);
    m_colorColor3Action->setEnabled(false);

    if (platformGroup == "rk86") {
        hasColor = true;
        m_colorLabel->setVisible(false);

        m_colorMonoOrigAction->setVisible(true);
        m_colorMonoOrigAction->setEnabled(true);

        m_colorMonoAction->setVisible(true);
        m_colorMonoAction->setEnabled(true);

        m_colorColor1Action->setVisible(true);
        m_colorColor1Action->setEnabled(true);
        m_colorColor1Action->setText(tr("Color (Tolkalin)"));
        m_colorColor1Action->setData("color1");

        m_colorGrayscaleAction->setVisible(false);
        m_colorGrayscaleAction->setEnabled(false);

        m_colorColor2Action->setVisible(true);
        m_colorColor2Action->setEnabled(true);
        m_colorColor2Action->setText(tr("Color (Akimenko)"));
        m_colorColor2Action->setData("color2");

        m_colorColor3Action->setVisible(true);
        m_colorColor3Action->setEnabled(true);
        m_colorColor3Action->setText(tr("Color (Apogey)"));
        m_colorColor3Action->setData("color3");
    } else if (platformGroup == "apogey") {
        hasColor = true;

        m_colorMonoOrigAction->setVisible(false);
        m_colorMonoOrigAction->setEnabled(false);

        m_colorMonoAction->setVisible(true);
        m_colorMonoAction->setEnabled(true);

        m_colorColor1Action->setVisible(true);
        m_colorColor1Action->setEnabled(true);
        m_colorColor1Action->setText(tr("Color"));
        m_colorColor1Action->setData("color");

        m_colorGrayscaleAction->setVisible(true);
        m_colorGrayscaleAction->setEnabled(true);

        m_colorColor2Action->setVisible(false);
        m_colorColor2Action->setEnabled(false);
    } else if (platformGroup == "bashkiria" || platformGroup == "orion" || platformGroup == "lvov" ||
               platformGroup == "vector" || platformGroup == "pk8000" || platformGroup == "korvet") {
        hasColor = true;

        m_colorMonoOrigAction->setVisible(false);
        m_colorMonoOrigAction->setEnabled(false);

        m_colorMonoAction->setVisible(true);
        m_colorMonoAction->setEnabled(true);

        m_colorColor1Action->setVisible(true);
        m_colorColor1Action->setEnabled(true);
        m_colorColor1Action->setText(tr("Color"));
        m_colorColor1Action->setData("color");

        m_colorGrayscaleAction->setVisible(false);
        m_colorGrayscaleAction->setEnabled(false);

        m_colorColor2Action->setVisible(false);
        m_colorColor2Action->setEnabled(false);
    } else if (platformGroup == "kr04") {
        hasColor = true;
        m_colorLabel->setVisible(false);

        m_colorMonoOrigAction->setVisible(false);
        m_colorMonoOrigAction->setEnabled(false);

        m_colorMonoAction->setVisible(true);
        m_colorMonoAction->setEnabled(true);

        m_colorColor1Action->setVisible(true);
        m_colorColor1Action->setEnabled(true);
        m_colorColor1Action->setText(tr("Color"));
        m_colorColor1Action->setData("color");

        m_colorGrayscaleAction->setVisible(false);
        m_colorGrayscaleAction->setEnabled(false);

        m_colorColor2Action->setVisible(true);
        m_colorColor2Action->setEnabled(true);
        m_colorColor2Action->setText(tr("Color Module"));
        m_colorColor2Action->setData("colorModule");
    } else if (platformGroup == "spec" || platformGroup == "sp580") {
        hasColor = true;

        m_colorMonoOrigAction->setVisible(false);
        m_colorMonoOrigAction->setEnabled(false);

        m_colorMonoAction->setVisible(true);
        m_colorMonoAction->setEnabled(true);

        m_colorColor1Action->setVisible(true);
        m_colorColor1Action->setEnabled(true);
        m_colorColor1Action->setText(tr("4-color"));
        m_colorColor1Action->setData("4color");

        m_colorGrayscaleAction->setVisible(false);
        m_colorGrayscaleAction->setEnabled(false);

        m_colorColor2Action->setVisible(true);
        m_colorColor2Action->setEnabled(true);
        m_colorColor2Action->setText(tr("8-color"));
        m_colorColor2Action->setData("8color");
    }

    m_colorLabel->setVisible(hasColor);
    m_colorMenuAction->setVisible(hasColor);
    m_colorMenuAction->setEnabled(hasColor);

    m_copyTextAction->setVisible(platformGroup == "apogey" || platformGroup == "rk86" ||
                                 platformGroup == "partner" || platformGroup == "mikrosha" ||
                                 platformGroup == "mikro80" || platformGroup == "ut88" ||
                                 platformGroup == "korvet" || platformGroup == "palmira" ||
                                 platformGroup == "kr04");

    m_printerCaptureAction->setVisible(platformGroup == "korvet" || platformGroup == "vector" || platformGroup == "pk8000" || platformGroup == "lvov");

    m_loadMenuAction->setVisible(platformGroup != "korvet" && platformGroup != "bashkiria" && platformGroup != "zx");
    m_loadRunMenuAction->setVisible(platformGroup != "korvet" && platformGroup != "bashkiria");

    m_pasteAction->setVisible(!emuGetPropertyValue(m_palWindow->getPlatformObjectName() + ".kbdTapper", "pasting").empty());

    m_platformConfigAction->setVisible(PlatformConfigDialog::hasConfig(QString::fromUtf8(getPlatformObjectName().c_str())));

    updateLastFiles();

    QString platformName = QString::fromUtf8(getPlatformObjectName().c_str());
    updateLastPlatforms(platformName);
}


void MainWindow::incFrameCount()
{
    if (m_frameCount == 0) {
        m_firstFpsCoutnerFrameTime = palGetCounter();
        m_lastFpsCoutnerFrameTime = m_firstFpsCoutnerFrameTime;
    } else
        m_lastFpsCoutnerFrameTime = palGetCounter();
    ++m_frameCount;
    //static int n = 0; setWindowTitle(QString::number(n++)); // debug - show frame No in the window title
};


void MainWindow::mouseClick(int x, int y, PalMouseKey key)
{
    if (m_windowType != EWT_DEBUG)
        return;

    m_palWindow->mouseClick(x, y, key);
}


void MainWindow::mouseDrag(int x, int y)
{
    if (m_windowType != EWT_EMULATION)
        return;

    m_palWindow->mouseDrag(x, y);
}


void MainWindow::onFpsTimer()
{
    if (m_windowType != EWT_EMULATION)
        return;

    ++m_fpsTimerCnt %= 4;

    if (!m_fpsTimerCnt) {
        // every 4th call (1 s)
        uint64_t delta = m_lastFpsCoutnerFrameTime - m_firstFpsCoutnerFrameTime;

        QString s;
        if (delta != 0) {
            unsigned fps = ((uint64_t)(m_frameCount - 1) * 1000000000 + delta / 2) / delta;
            if (fps < 1000) {
                s.setNum(fps);
                s += " fps";
            }
        }

        m_fpsLabel->setText(s);
        m_frameCount = 0;
    }

    double speed = emuGetEmulationSpeedFactor();
    m_speedLabel->setVisible(speed != 1.);
    if (speed == 0.)
        m_speedLabel->setText(tr("Paused"));
    else if (speed < 0.)
        m_speedLabel->setText(tr("Max"));
    else
        m_speedLabel->setText(QString::number(speed, 'f', 2) + "x");


    std::string platform = m_palWindow->getPlatformObjectName() + ".";

    std::string crtMode = "";
    if (m_palWindow)
        crtMode= emuGetPropertyValue(platform + "crtRenderer", "crtMode");
    m_crtModeLabel->setText(QString::fromUtf8(crtMode.c_str()));
    m_crtModeLabel->setVisible(crtMode != "");

    std::string dmaTime = "";
    if (m_palWindow)
        dmaTime = emuGetPropertyValue(platform + "dma", "percentage");
    m_dmaTimeLabel->setText(/*tr("DMA: ") + */QString::fromUtf8(dmaTime.c_str()) + " %");
    m_dmaTimeLabel->setVisible(dmaTime != "");

    m_imageSizeLabel->setText(QString::number(m_paintWidget->getImageWidth()) + QString::fromUtf8(u8"\u00D7") + QString::number(m_paintWidget->getImageHeight()));

    std::string fileName;
    QString labelText;
    fileName = emuGetPropertyValue(platform + "tapeInFile", "currentFile");
    if (fileName != "")
        labelText = tr("Reading RK file:");
    else {
        fileName = emuGetPropertyValue(platform + "tapeOutFile", "currentFile");
        if (fileName != "")
            labelText = tr("Writing RK file:");
        else {
            fileName = emuGetPropertyValue(platform + "msxTapeInFile", "currentFile");
            if (fileName != "")
                labelText = tr("Reading MSX file:");
            else {
                fileName = emuGetPropertyValue(platform + "msxTapeOutFile", "currentFile");
                if (fileName != "")
                    labelText = tr("Writing MSX file:");
                else {
                    fileName = emuGetPropertyValue(platform + "rfsTapeOutFile", "currentFile");
                    if (fileName != "")
                        labelText = tr("Writing RFS file:");
                    else {
                        fileName = emuGetPropertyValue(platform + "rfsTapeInFile", "currentFile");
                        if (fileName != "")
                            labelText = tr("Reading RFS file:");
                    }
                }
            }
        }
    }
    if (fileName != "") {
        QString qFileName = QString::fromUtf8(fileName.c_str());
        qFileName = qFileName.mid(qFileName.lastIndexOf('/') + 1);
        m_tapeLabel->setText(labelText + " " + qFileName);
    }
    m_tapeLabel->setVisible(fileName != "");

    fileName = emuGetPropertyValue("prnWriter", "fileName");
    if (!fileName.empty()) {
        QString qFileName = QString::fromUtf8(fileName.c_str());
        qFileName = qFileName.mid(qFileName.lastIndexOf('/') + 1);
        m_prnLabel->setText(tr("Printing to:") + " " + qFileName);
        m_prnLabel->setVisible(true);
    } else
        m_prnLabel->setVisible(false);

    std::string position;
    fileName = emuGetPropertyValue("wavWriter", "currentFile");
    if (fileName != "")
        labelText = tr("Writing wave file:");
    else {
        fileName = emuGetPropertyValue("wavReader", "currentFile");
        if (fileName != "") {
            labelText = tr("Playing wave file:");
            position = emuGetPropertyValue("wavReader", "position");
        }
    }
    if (fileName != "") {
        QString qFileName = QString::fromUtf8(fileName.c_str());
        qFileName = qFileName.mid(qFileName.lastIndexOf('/') + 1);
        QString qPosition;
        if (position != "") {
            qPosition = " [" + QString::fromUtf8(position.c_str()) + "]";
        }
        m_wavLabel->setText(labelText + " " + qFileName + qPosition);
    }
    m_wavLabel->setVisible(fileName != "");

    std::string val = emuGetPropertyValue(platform + "kbdTapper", "pasting");
    if (val == "yes") {
        m_pasteLabel->setText(tr("Pasting"));
        m_pasteLabel->setVisible(true);
    } else {
        m_pasteLabel->setText("");
        m_pasteLabel->setVisible(false);
    }
}


PalKeyCode MainWindow::translateKey(QKeyEvent* evt)
{
    int key = evt->key();
    bool keypad = evt->modifiers() & Qt::KeypadModifier;

    switch (key) {
    case Qt::Key_A:
        return PK_A;
    case Qt::Key_B:
        return PK_B;
    case Qt::Key_C:
        return PK_C;
    case Qt::Key_D:
        return PK_D;
    case Qt::Key_E:
        return PK_E;
    case Qt::Key_F:
        return PK_F;
    case Qt::Key_G:
        return PK_G;
    case Qt::Key_H:
        return PK_H;
    case Qt::Key_I:
        return PK_I;
    case Qt::Key_J:
        return PK_J;
    case Qt::Key_K:
        return PK_K;
    case Qt::Key_L:
        return PK_L;
    case Qt::Key_M:
        return PK_M;
    case Qt::Key_N:
        return PK_N;
    case Qt::Key_O:
        return PK_O;
    case Qt::Key_P:
        return PK_P;
    case Qt::Key_Q:
        return PK_Q;
    case Qt::Key_R:
        return PK_R;
    case Qt::Key_S:
        return PK_S;
    case Qt::Key_T:
        return PK_T;
    case Qt::Key_U:
        return PK_U;
    case Qt::Key_V:
        return PK_V;
    case Qt::Key_W:
        return PK_W;
    case Qt::Key_X:
        return PK_X;
    case Qt::Key_Y:
        return PK_Y;
    case Qt::Key_Z:
        return PK_Z;

    case Qt::Key_1:
        return PK_1;
    case Qt::Key_Exclam:
        return PK_1;
    case Qt::Key_2:
        return PK_2;
    case Qt::Key_At:
        return PK_2;
    case Qt::Key_3:
        return PK_3;
    case Qt::Key_NumberSign:
        return PK_3;
    case Qt::Key_4:
        return PK_4;
    case Qt::Key_Dollar:
        return PK_4;
    case Qt::Key_5:
        return PK_5;
    case Qt::Key_Percent:
        return PK_5;
    case Qt::Key_6:
        return PK_6;
    case Qt::Key_AsciiCircum:
        return PK_6;
    case Qt::Key_7:
        return PK_7;
    case Qt::Key_Ampersand:
        return PK_7;
    case Qt::Key_8:
        return PK_8;
    case Qt::Key_Asterisk:
        return keypad ? PK_KP_MUL : PK_8;
    case Qt::Key_9:
        return PK_9;
    case Qt::Key_ParenLeft:
        return PK_9;
    case Qt::Key_0:
        return PK_0;
    case Qt::Key_ParenRight:
        return PK_0;

    case Qt::Key_Return:
        return PK_ENTER;
    case Qt::Key_Escape:
        return PK_ESC;
    case Qt::Key_Backspace:
        return PK_BSP;
    case Qt::Key_Tab:
        return PK_TAB;
    case Qt::Key_Space:
        return PK_SPACE;

    case Qt::Key_Minus:
        return keypad ? PK_KP_MINUS : PK_MINUS;
    case Qt::Key_Underscore:
        return PK_MINUS;
    case Qt::Key_Equal:
        return PK_EQU;
    case Qt::Key_Plus:
        return keypad ? PK_KP_PLUS : PK_EQU;
    case Qt::Key_BracketLeft:
    case Qt::Key_BraceLeft:
        return PK_LBRACKET;
    case Qt::Key_BracketRight:
    case Qt::Key_BraceRight:
        return PK_RBRACKET;
    case Qt::Key_Backslash:
    case Qt::Key_Bar:
        return PK_BSLASH;
    case Qt::Key_Semicolon:
    case Qt::Key_Colon:
        return PK_SEMICOLON;
    case Qt::Key_Apostrophe:
    case Qt::Key_QuoteDbl:
        return PK_APOSTROPHE;
    case Qt::Key_QuoteLeft:
    case Qt::Key_AsciiTilde:
        return PK_TILDE;
    case Qt::Key_Comma:
        return PK_COMMA;
    case Qt::Key_Less:
        return PK_COMMA;
    case Qt::Key_Period:
        return PK_PERIOD;
    case Qt::Key_Greater:
        return PK_PERIOD;
    case Qt::Key_Slash:
    case Qt::Key_Question:
        return keypad ? PK_KP_DIV : PK_SLASH;

    case Qt::Key_CapsLock:
        return PK_CAPSLOCK;

    case Qt::Key_F1:
        return PK_F1;
    case Qt::Key_F2:
        return PK_F2;
    case Qt::Key_F3:
        return PK_F3;
    case Qt::Key_F4:
        return PK_F4;
    case Qt::Key_F5:
        return PK_F5;
    case Qt::Key_F6:
        return PK_F6;
    case Qt::Key_F7:
        return PK_F7;
    case Qt::Key_F8:
        return PK_F8;
    case Qt::Key_F9:
        return PK_F9;
    case Qt::Key_F10:
        return PK_F10;
    case Qt::Key_F11:
        return PK_F11;
    case Qt::Key_F12:
        return PK_F12;

    case Qt::Key_ScrollLock:
        return PK_SCRLOCK;
    case Qt::Key_Pause:
        return PK_PAUSEBRK;

    case Qt::Key_Insert:
        return keypad ? PK_KP_0 :PK_INS;
    case Qt::Key_Home:
        return keypad ? PK_KP_7 :PK_HOME;
    case Qt::Key_PageUp:
        return keypad ? PK_KP_9 :PK_PGUP;
    case Qt::Key_Delete:
        return keypad ? PK_KP_PERIOD : PK_DEL;
    case Qt::Key_End:
        return keypad ? PK_KP_1 :PK_END;
    case Qt::Key_PageDown:
        return keypad ? PK_KP_3 :PK_PGDN;
    case Qt::Key_Right:
        return keypad ? PK_KP_6 :PK_RIGHT;
    case Qt::Key_Left:
        return keypad ? PK_KP_4 :PK_LEFT;
    case Qt::Key_Down:
        return keypad ? PK_KP_2 :PK_DOWN;
    case Qt::Key_Up:
        return keypad ? PK_KP_8 :PK_UP;
    case Qt::Key_Clear:
        return PK_KP_5;

    case Qt::Key_NumLock:
        return PK_NUMLOCK;
    case Qt::Key_Enter:
        return PK_KP_ENTER;

    case Qt::Key_Control:
        return PK_LCTRL;
    case Qt::Key_Shift:
        return PK_LSHIFT;
    case Qt::Key_Alt:
        return PK_LALT;
    case Qt::Key_Meta:
#ifndef __APPLE__
        return PK_LWIN;
#else
        return PK_LCTRL;
#endif
    case Qt::Key_Menu:
        return PK_MENU;

        /*case SDL_SCANCODE_RCTRL:
        return PK_RCTRL;
    case SDL_SCANCODE_RSHIFT:
        return PK_RSHIFT;
    case SDL_SCANCODE_RALT:
        return PK_RALT;
    case SDL_SCANCODE_RGUI:
        return PK_RWIN;
    case SDL_SCANCODE_APPLICATION:
        return PK_MENU;*/

    default:
        return PK_NONE;
    }

}


void MainWindow::keyPressEvent(QKeyEvent* evt)
{
    if (evt->key() == Qt::Key_End && !(evt->modifiers() & Qt::KeypadModifier)) {
        emuSysReq(m_palWindow, evt->modifiers() & Qt::AltModifier ? SR_FULLTHROTTLE : SR_SPEEDUP);
        return;
    }
    unsigned unicodeKey = evt->text().isEmpty() ? 0 : evt->text()[0].unicode(); // "at()" does not operate with empty strings
    emuKeyboard(m_palWindow, translateKey(evt), true, unicodeKey);
}


void MainWindow::keyReleaseEvent(QKeyEvent* evt)
{
    if (evt->key() == Qt::Key_End && !(evt->modifiers() & Qt::KeypadModifier)) {
        emuSysReq(m_palWindow, SR_SPEEDNORMAL);
        return;
    }
    unsigned unicodeKey = evt->text().isEmpty() ? 0 : evt->text()[0].unicode(); // "at()" does not operate with empty strings
    emuKeyboard(m_palWindow, translateKey(evt), false, unicodeKey);
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    savePosition();

    event->ignore();
    emuSysReq(m_palWindow, SR_CLOSE);
}


void MainWindow::onQuit()
{
    savePosition();
}


void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() && (event->mimeData()->urls().size() == 1))
        event->acceptProposedAction();
}


void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}


void MainWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        QString qFileName = event->mimeData()->urls().begin()->toLocalFile();
        emuDropFile(m_palWindow, qFileName.toUtf8().constData());
    }
}


void MainWindow::onShiftF10()
{
    emuKeyboard(m_palWindow, PK_F10, true, 0);
}


void MainWindow::onReset()
{
    emuSysReq(m_palWindow, SR_RESET);
}


void MainWindow::onPause()
{
    bool paused = ((QAction*)sender())->isChecked();
    emuSysReq(m_palWindow, paused ? SR_PAUSEON : SR_PAUSEOFF);
}


void MainWindow::onSpeedUp()
{
    emuSysReq(m_palWindow, SR_SPEEDSTEPUP);
}


void MainWindow::onSpeedDown()
{
    emuSysReq(m_palWindow, SR_SPEEDSTEPDOWN);
}


void MainWindow::onSpeedUpFine()
{
    emuSysReq(m_palWindow, SR_SPEEDSTEPUPFINE);
}


void MainWindow::onSpeedDownFine()
{
    emuSysReq(m_palWindow, SR_SPEEDSTEPDOWNFINE);
}


void MainWindow::onSpeedNormal()
{
    emuSysReq(m_palWindow, SR_SPEEDSTEPNORMAL);
}


void MainWindow::onMute()
{
    emuSysReq(m_palWindow, SR_MUTE);
}


void MainWindow::onForwardOn()
{
    emuSysReq(m_palWindow, SR_SPEEDUP);
}


void MainWindow::onForwardOff()
{
    emuSysReq(m_palWindow, SR_SPEEDNORMAL);
}


void MainWindow::onFullThrottleOn()
{
    emuSysReq(m_palWindow, SR_FULLTHROTTLE);
}


void MainWindow::onDebug()
{
    emuSysReq(m_palWindow, SR_DEBUG);
}


void MainWindow::onQwerty()
{
    emuSysReq(m_palWindow, SR_QUERTY);
    saveConfig();
}


void MainWindow::onJcuken()
{
    emuSysReq(m_palWindow, SR_JCUKEN);
    //saveConfig(); // don't save jcuken layout
}


void MainWindow::onSmart()
{
    emuSysReq(m_palWindow, SR_SMART);
    saveConfig();
}


void MainWindow::onColorMode()
{
    emuSysReq(m_palWindow, SR_COLOR);
    saveConfig();
}


void MainWindow::onColorSelect()
{
    if (!m_colorModeMenu)
        return;

    QAction* action = (QAction*)sender();
    std::string colorMode(action->data().toString().toUtf8().constData());

    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".crtRenderer", "colorMode", colorMode);
    updateConfig();
    saveConfig();
}


void MainWindow::onPreset()
{
    std::string wnd = m_palWindow->getPlatformObjectName() + ".window";
    std::string windowStyle = emuGetPropertyValue(wnd, "windowStyle");
    if (windowStyle == "resizable") {
        emuSetPropertyValue(wnd, "windowStyle", "autosize");
        emuSetPropertyValue(wnd, "frameScale", "fixed");
    } else if (windowStyle == "autosize") {
        emuSetPropertyValue(wnd, "windowStyle", "resizable");
        emuSetPropertyValue(wnd, "frameScale", "fit");
    }
    updateConfig();
    saveConfig();
}


void MainWindow::on1x()
{
    emuSysReq(m_palWindow, SR_1X);
    saveConfig();
}


void MainWindow::on2x()
{
    emuSysReq(m_palWindow, SR_2X);
    saveConfig();
}


void MainWindow::on3x()
{
    emuSysReq(m_palWindow, SR_3X);
    saveConfig();
}


void MainWindow::on4x()
{
    emuSysReq(m_palWindow, SR_4X);
    saveConfig();
}


void MainWindow::on5x()
{
    emuSysReq(m_palWindow, SR_5X);
    saveConfig();
}


void MainWindow::on15x()
{
    emuSysReq(m_palWindow, SR_1_5X);
    saveConfig();
}


void MainWindow::on25x()
{
    emuSysReq(m_palWindow, SR_2_5X);
    saveConfig();
}


/*void MainWindow::on2x3()
{
    emuSysReq(m_palWindow, SR_2X3);
    saveConfig();
}


void MainWindow::on3x5()
{
    emuSysReq(m_palWindow, SR_3X5);
    saveConfig();
}


void MainWindow::on4x6()
{
    emuSysReq(m_palWindow, SR_4X6);
    saveConfig();
}


void MainWindow::onStretch()
{
    emuSysReq(m_palWindow, SR_STRETCH);
    saveConfig();
}*/


void MainWindow::onFit()
{
    emuSysReq(m_palWindow, SR_FIT);
    saveConfig();
}


void MainWindow::onFullscreen()
{
    emuSysReq(m_palWindow, SR_FULLSCREEN);
}


void MainWindow::onFullwindow()
{
    bool visible = !(m_fullwindowAction->isChecked() || m_fullscreenMode);
    if (m_menuBar)
        m_menuBar->setVisible(visible);
    if (m_statusBar)
        m_statusBar->setVisible(visible);
    if (m_toolBar)
        m_toolBar->setVisible(visible);
    if (m_clientWidth != 0 && m_clientHeight != 0) {
        adjustClientSize();
    }
}


void MainWindow::onLoad()
{
    emuSysReq(m_palWindow, SR_LOAD);
    QString lastFileName = QString::fromUtf8(emuGetPropertyValue(m_palWindow->getPlatformObjectName() + ".loader", "lastFile").c_str());
    if (!lastFileName.isEmpty()) {
        m_loaderLastFiles.addToLastFiles(lastFileName);
        updateLastFiles();
    }
    updateActions();
}


void MainWindow::onLoadLastFiles()
{
    QAction* action = (QAction*)sender();
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".loader", "loadFile", action->text().toStdString());
    m_loaderLastFiles.addToLastFiles(action->text());
    updateLastFiles();
    updateActions();
}


void MainWindow::onLoadRun()
{
    emuSysReq(m_palWindow, SR_LOADRUN);
    QString lastFileName = QString::fromUtf8(emuGetPropertyValue(m_palWindow->getPlatformObjectName() + ".loader", "lastFile").c_str());
    if (!lastFileName.isEmpty()) {
        m_loaderLastFiles.addToLastFiles(lastFileName);
        updateLastFiles();
    }
    updateActions();
}


void MainWindow::onLoadRunLastFiles()
{
    QAction* action = (QAction*)sender();
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".loader", "loadRunFile", action->text().toStdString());
    m_loaderLastFiles.addToLastFiles(action->text());
    updateLastFiles();
    updateActions();
}


void MainWindow::onExit()
{
    emuSysReq(m_palWindow, SR_EXIT);
}


void MainWindow::onLoadWav()
{
    emuSysReq(m_palWindow, SR_LOADWAV);
}


void MainWindow::onDiskA()
{
    emuSysReq(m_palWindow, SR_DISKA);
    QString lastFileName = QString::fromUtf8(emuGetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskA", "fileName").c_str());
    m_fddLastFiles.addToLastFiles(lastFileName);
    updateActions();
    updateLastFiles();
}


void MainWindow::onUnmountDiskA()
{
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskA", "fileName", "");
    updateConfig();
    saveConfig();
}


void MainWindow::onReadOnlyDiskA()
{
    bool readOnly = m_diskAReadOnlyAction->isChecked();
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskA", "readOnly", readOnly ? "yes" : "no");
    updateConfig();
    saveConfig();
}


void MainWindow::onDiskALastFiles()
{
    QAction* action = (QAction*)sender();
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskA", "fileName", action->text().toStdString());
    m_fddLastFiles.addToLastFiles(action->text());
    updateLastFiles();
    updateConfig();
    saveConfig();
}


void MainWindow::onAutoMountDiskA()
{
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskA", "autoMount", m_diskAAutoMountAction->isChecked() ? "yes" : "no");
    updateConfig();
    saveConfig();
}


void MainWindow::onDiskB()
{
    emuSysReq(m_palWindow, SR_DISKB);
    QString lastFileName = QString::fromUtf8(emuGetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskB", "fileName").c_str());
    m_fddLastFiles.addToLastFiles(lastFileName);
    updateActions();
    updateLastFiles();
}


void MainWindow::onUnmountDiskB()
{
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskB", "fileName", "");
    updateConfig();
    saveConfig();
}


void MainWindow::onReadOnlyDiskB()
{
    bool readOnly = m_diskBReadOnlyAction->isChecked();
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskB", "readOnly", readOnly ? "yes" : "no");
    updateConfig();
    saveConfig();
}


void MainWindow::onDiskBLastFiles()
{
    QAction* action = (QAction*)sender();
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskB", "fileName", action->text().toStdString());
    m_fddLastFiles.addToLastFiles(action->text());
    updateLastFiles();
    updateConfig();
    saveConfig();
}


void MainWindow::onAutoMountDiskB()
{
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskB", "autoMount", m_diskBAutoMountAction->isChecked() ? "yes" : "no");
    updateConfig();
    saveConfig();
}


void MainWindow::onDiskC()
{
    emuSysReq(m_palWindow, SR_DISKC);
    QString lastFileName = QString::fromUtf8(emuGetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskC", "fileName").c_str());
    m_fddLastFiles.addToLastFiles(lastFileName);
    updateActions();
    updateLastFiles();
}


void MainWindow::onUnmountDiskC()
{
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskC", "fileName", "");
    updateConfig();
    saveConfig();
}


void MainWindow::onReadOnlyDiskC()
{
    bool readOnly = m_diskCReadOnlyAction->isChecked();
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskC", "readOnly", readOnly ? "yes" : "no");
    updateConfig();
    saveConfig();
}


void MainWindow::onDiskCLastFiles()
{
    QAction* action = (QAction*)sender();
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskC", "fileName", action->text().toStdString());
    m_fddLastFiles.addToLastFiles(action->text());
    updateLastFiles();
    updateConfig();
    saveConfig();
}


void MainWindow::onAutoMountDiskC()
{
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskC", "autoMount", m_diskCAutoMountAction->isChecked() ? "yes" : "no");
    updateConfig();
    saveConfig();
}


void MainWindow::onDiskD()
{
    emuSysReq(m_palWindow, SR_DISKD);
    QString lastFileName = QString::fromUtf8(emuGetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskD", "fileName").c_str());
    m_fddLastFiles.addToLastFiles(lastFileName);
    updateActions();
    updateLastFiles();
}


void MainWindow::onUnmountDiskD()
{
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskD", "fileName", "");
    updateConfig();
    saveConfig();
}


void MainWindow::onReadOnlyDiskD()
{
    bool readOnly = m_diskDReadOnlyAction->isChecked();
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskD", "readOnly", readOnly ? "yes" : "no");
    updateConfig();
    saveConfig();
}


void MainWindow::onDiskDLastFiles()
{
    QAction* action = (QAction*)sender();
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskD", "fileName", action->text().toStdString());
    m_fddLastFiles.addToLastFiles(action->text());
    updateLastFiles();
    updateConfig();
    saveConfig();
}


void MainWindow::onAutoMountDiskD()
{
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".diskD", "autoMount", m_diskDAutoMountAction->isChecked() ? "yes" : "no");
    updateConfig();
    saveConfig();
}


void MainWindow::onHdd()
{
    emuSysReq(m_palWindow, SR_HDD);
    QString lastFileName = QString::fromUtf8(emuGetPropertyValue(m_palWindow->getPlatformObjectName() + ".hdd", "fileName").c_str());
    m_hddLastFiles.addToLastFiles(lastFileName);
    updateActions();
    updateLastFiles();
}


void MainWindow::onUnmountHdd()
{
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".hdd", "fileName", "");
    updateConfig();
    saveConfig();
}


void MainWindow::onReadOnlyHdd()
{
    bool readOnly = m_hddReadOnlyAction->isChecked();
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".hdd", "readOnly", readOnly ? "yes" : "no");
    updateConfig();
    saveConfig();
}


void MainWindow::onHddLastFiles()
{
    QAction* action = (QAction*)sender();
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".hdd", "fileName", action->text().toStdString());
    m_hddLastFiles.addToLastFiles(action->text());
    updateLastFiles();
    updateConfig();
    saveConfig();
}


void MainWindow::onAutoMountHdd()
{
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".hdd", "autoMount", m_hddAutoMountAction->isChecked() ? "yes" : "no");
    updateConfig();
    saveConfig();
}


void MainWindow::onCrop()
{
    emuSysReq(m_palWindow, SR_CROPTOVISIBLE);
    saveConfig();
}


void MainWindow::onAspect()
{
    emuSysReq(m_palWindow, SR_ASPECTCORRECTION);
    saveConfig();
}


void MainWindow::onWideScreen()
{
    emuSysReq(m_palWindow, SR_WIDESCREEN);
    saveConfig();
}


void MainWindow::onFont()
{
    emuSysReq(m_palWindow, SR_FONT);
    saveConfig();
}


void MainWindow::onSmoothing()
{
    emuSysReq(m_palWindow, SR_SMOOTHING);
    saveConfig();
}


void MainWindow::onSmoothingSelect()
{
    if (!m_smoothingMenu)
        return;

    QAction* action = (QAction*)sender();
    std::string smoothingMode(action->data().toString().toUtf8().constData());

    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".window", "smoothing", smoothingMode);
    updateConfig();
    saveConfig();
}


void MainWindow::setSmoothingAndShader(SmoothingType smoothing, const std::string& shaderName)
{
    QString shaderFileName;
    if (!shaderName.empty() && shaderName != "none") {
        QString shaderWithDir = "shaders/" + QString::fromUtf8(shaderName.c_str()) + ".glsl";
        shaderFileName = QString::fromUtf8(palMakeFullFileName(shaderWithDir.toUtf8().constData()).c_str());
    }

    m_paintWidget->setSmoothingAndShaderFile(smoothing, shaderFileName);
}


void MainWindow::onShaderSelect()
{
    QAction* action = (QAction*)sender();
    QString shaderName = action->data().toString();

    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".window", "shader", shaderName.toUtf8().constData());
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".window", "smoothing", shaderName != "none" ? "custom" : "sharp");
    updateConfig();
    saveConfig();
}


void MainWindow::onDesaturate()
{
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".window", "desaturate", m_desaturateAction->isChecked() ? "yes" : "no");
    updateConfig();
    saveConfig();
}


void MainWindow::onPlatformSelect()
{
    QAction* action = (QAction*)sender();
    std::string platform(action->data().toString().toUtf8().constData());

    {
        // Set as default
        QSettings settings;
        SET_INI_CODEC(settings);
        settings.beginGroup("system");
        settings.setValue("platform", action->data().toString().toUtf8().constData());
    } // ensure QSettings object is destroyed now before calling emuSelectPlatform and creating another QSettings

    emuSelectPlatform(platform);
}


void MainWindow::onPlatform()
{
    emuSysReq(m_palWindow, SR_CHPLATFORM);
}


void MainWindow::onPlatformConfig()
{
    emuSysReq(m_palWindow, SR_CHCONFIG);
}


void MainWindow::onTapeHook()
{
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".tapeGrp", "enabled", m_tapeHookAction->isChecked() ? "yes" : "no");
    m_settingsDialog->updateConfig();
    saveConfig();
}


void MainWindow::onMuteTape()
{
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".tapeSoundSource", "muted", m_muteTapeAction->isChecked() ? "yes" : "no");
    m_settingsDialog->updateConfig();
    saveConfig();
}


void MainWindow::onFastReset()
{
    emuSysReq(m_palWindow, SR_FASTRESET);
    //emuSetPropertyValue(m_palWindow->getPlatformObjectName(), "fastReset", m_fastResetAction->isChecked() ? "yes" : "no"); // equivalent to the one above

    m_settingsDialog->updateConfig();
    saveConfig();
}


void MainWindow::onSettings()
{
    if (m_settingsDialog)
        m_settingsDialog->execute();
}


void MainWindow::onScreenshot()
{
    emuSysReq(m_palWindow, SR_SCREENSHOT);
}


void MainWindow::onCopyImage()
{
    m_paintWidget->screenshot("");
}


void MainWindow::onCopyText()
{
    emuSysReq(m_palWindow, SR_COPYTXT);
}


void MainWindow::onPaste()
{
    emuSysReq(m_palWindow, SR_PASTE);
}


void MainWindow::onPlatformHelp()
{
    std::string helpFile = palMakeFullFileName(emuGetPropertyValue(m_palWindow->getPlatformObjectName(), "helpFile"));
    HelpDialog::execute(QString::fromUtf8(helpFile.c_str()), false);
    HelpDialog::activate();
}


void MainWindow::onAbout()
{
    AboutDialog* dialog = new AboutDialog(this);
    m_paintWidget->pauseCursorTimer(true);
    dialog->execute();
    m_paintWidget->pauseCursorTimer(false);
    delete dialog;
}


void MainWindow::onResetPlatform()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Emu80: warning"));
    msgBox.setText(tr("Reset current platform and common settings?"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.addButton(QMessageBox::Yes);
    msgBox.addButton(QMessageBox::No);
    if (msgBox.exec() == QMessageBox::Yes) {
        m_settingsDialog->resetPlatformOptions();
    }
}


void MainWindow::onResetAll()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Emu80 Warning"));
    msgBox.setText(tr("Reset all settings?"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.addButton(QMessageBox::Yes);
    msgBox.addButton(QMessageBox::No);
    if (msgBox.exec() == QMessageBox::Yes) {
        m_settingsDialog->resetAllOptions();
    }
}


void MainWindow::onEdd()
{
    std::string ramDisk = m_palWindow->getPlatformObjectName() + ".ramDisk";

    emuSysReq(m_palWindow, SR_OPENRAMDISK);

    QString lastFileName = QString::fromUtf8(emuGetPropertyValue(m_palWindow->getPlatformObjectName() + ".ramDisk", "fileName").c_str());
    if (!lastFileName.isEmpty())
        m_eddLastFiles.addToLastFiles(lastFileName);

    updateActions();
    updateLastFiles();
}


void MainWindow::onEddSaveAs()
{
    std::string ramDisk = m_palWindow->getPlatformObjectName() + ".ramDisk";

    emuSysReq(m_palWindow, SR_SAVERAMDISKAS);

    QString lastFileName = QString::fromUtf8(emuGetPropertyValue(ramDisk, "fileName").c_str());
    if (!lastFileName.isEmpty())
        m_eddLastFiles.addToLastFiles(lastFileName);

    updateActions();
    updateLastFiles();
}


void MainWindow::onEddSave()
{
    std::string ramDisk = m_palWindow->getPlatformObjectName() + ".ramDisk";
    emuSysReq(m_palWindow, SR_SAVERAMDISK);
}


void MainWindow::onEddUnassign()
{
    std::string ramDisk = m_palWindow->getPlatformObjectName() + ".ramDisk";
    emuSetPropertyValue(ramDisk, "fileName", "");

    updateConfig();
    saveConfig();
}


void MainWindow::onEddAutoLoad()
{
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".ramDisk", "autoLoad", m_eddAutoLoadAction->isChecked() ? "yes" : "no");

    updateConfig();
    saveConfig();
}


void MainWindow::onEddAutoSave()
{
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".ramDisk", "autoSave", m_eddAutoSaveAction->isChecked() ? "yes" : "no");

    updateConfig();
    saveConfig();
}


void MainWindow::onEddLastFiles()
{
    QAction* action = (QAction*)sender();
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".ramDisk", "fileName", action->text().toStdString());
    m_eddLastFiles.addToLastFiles(action->text());
    updateLastFiles();
    updateConfig();
    saveConfig();
}


void MainWindow::onEdd2()
{
    std::string ramDisk = m_palWindow->getPlatformObjectName() + ".ramDisk2";

    emuSysReq(m_palWindow, SR_OPENRAMDISK2);

    QString lastFileName = QString::fromUtf8(emuGetPropertyValue(ramDisk, "fileName").c_str());
    if (!lastFileName.isEmpty())
        m_eddLastFiles.addToLastFiles(lastFileName);

    updateActions();
    updateLastFiles();
}


void MainWindow::onEdd2SaveAs()
{
    std::string ramDisk = m_palWindow->getPlatformObjectName() + ".ramDisk2";

    emuSysReq(m_palWindow, SR_SAVERAMDISK2AS);

    QString lastFileName = QString::fromUtf8(emuGetPropertyValue(ramDisk, "fileName").c_str());
    if (!lastFileName.isEmpty())
        m_eddLastFiles.addToLastFiles(lastFileName);

    updateActions();
    updateLastFiles();
}


void MainWindow::onEdd2Save()
{
    std::string ramDisk = m_palWindow->getPlatformObjectName() + ".ramDisk2";
    emuSysReq(m_palWindow, SR_SAVERAMDISK2);
}


void MainWindow::onEdd2Unassign()
{
    std::string ramDisk = m_palWindow->getPlatformObjectName() + ".ramDisk2";
    emuSetPropertyValue(ramDisk, "fileName", "");

    updateConfig();
    saveConfig();
}


void MainWindow::onEdd2AutoLoad()
{
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".ramDisk2", "autoLoad", m_edd2AutoLoadAction->isChecked() ? "yes" : "no");

    updateConfig();
    saveConfig();
}


void MainWindow::onEdd2AutoSave()
{
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".ramDisk2", "autoSave", m_edd2AutoSaveAction->isChecked() ? "yes" : "no");

    updateConfig();
    saveConfig();
}


void MainWindow::onEdd2LastFiles()
{
    QAction* action = (QAction*)sender();
    emuSetPropertyValue(m_palWindow->getPlatformObjectName() + ".ramDisk2", "fileName", action->text().toStdString());
    m_eddLastFiles.addToLastFiles(action->text());
    updateLastFiles();
    updateConfig();
    saveConfig();
}


void MainWindow::onPrinterCapture()
{
    QAction* action = (QAction*)sender();
    std::string colorMode(action->data().toString().toUtf8().constData());

    emuSysReq(m_palWindow, ((QAction*)(sender()))->isChecked() ? SR_PRNCAPTURE_ON : SR_PRNCAPTURE_OFF);
    updateConfig(); //updateActions();
}


void MainWindow::updateConfig()
{
    if (m_palWindow->getWindowType() != EWT_EMULATION)
        return;

    updateActions();

    if (m_settingsDialog) {
        m_settingsDialog->updateConfig();
    }
}


void MainWindow::saveConfig()
{
    if (m_settingsDialog)
        m_settingsDialog->saveConfig();
}


void MainWindow::updateMountToolTip(QAction* action, const QString& fileName)
{
    QString toolTip = action->toolTip();
    toolTip = toolTip.left(toolTip.indexOf('\n'));
    if (!fileName.isEmpty())
        toolTip = toolTip + "\n" + tr("Mounted:") + " " + fileName;
    action->setToolTip(toolTip);
}


void MainWindow::updateActions()
{
    std::string platform = "";
    if (m_palWindow)
        platform = m_palWindow->getPlatformObjectName();
    if (platform == "")
        return;

    platform += ".";
    std::string val;


    // Disks menu

    val = emuGetPropertyValue(platform + "diskA", "label");
    m_diskAMenuAction->setVisible(!val.empty());
    m_diskAAction->setVisible(!val.empty()); // turn off shortcut
    if (!val.empty()) {
        QString qFileName = QString::fromUtf8(emuGetPropertyValue(platform + "diskA", "fileName").c_str());
        updateMountToolTip(m_diskAMenuAction, qFileName);
        if (qFileName.isEmpty()) {
            m_diskAUnmountAction->setEnabled(false);
            m_diskAUnmountAction->setText(tr("Unmount"));
            m_diskAMenuAction->setIcon(m_diskAOffIcon);
        } else {
            m_diskAUnmountAction->setEnabled(true);
            qFileName = qFileName.mid(qFileName.lastIndexOf('/') + 1);
            m_diskAUnmountAction->setText(tr("Unmount ") + " " + qFileName);
            m_diskAMenuAction->setIcon(m_diskAOnIcon);
        }
    }
    val = emuGetPropertyValue(platform + "diskA", "readOnly");
    m_diskAReadOnlyAction->setChecked(val == "yes");
    val = emuGetPropertyValue(platform + "diskA", "autoMount");
    m_diskAAutoMountAction->setChecked(val == "yes");

    val = emuGetPropertyValue(platform + "diskB", "label");
    m_diskBMenuAction->setVisible(!val.empty());
    m_diskBAction->setVisible(!val.empty()); // turn off shortcut
    if (!val.empty()) {
        QString qFileName = QString::fromUtf8(emuGetPropertyValue(platform + "diskB", "fileName").c_str());
        updateMountToolTip(m_diskBMenuAction, qFileName);
        if (qFileName.isEmpty()) {
            m_diskBUnmountAction->setEnabled(false);
            m_diskBUnmountAction->setText(tr("Unmount"));
            m_diskBMenuAction->setIcon(m_diskBOffIcon);
        } else {
            m_diskBUnmountAction->setEnabled(true);
            qFileName = qFileName.mid(qFileName.lastIndexOf('/') + 1);
            m_diskBUnmountAction->setText(tr("Unmount ") + " " + qFileName);
            m_diskBMenuAction->setIcon(m_diskBOnIcon);
        }
    }
    val = emuGetPropertyValue(platform + "diskB", "readOnly");
    m_diskBReadOnlyAction->setChecked(val == "yes");
    val = emuGetPropertyValue(platform + "diskB", "autoMount");
    m_diskBAutoMountAction->setChecked(val == "yes");

    val = emuGetPropertyValue(platform + "diskC", "label");
    m_diskCMenuAction->setVisible(!val.empty());
    m_diskCAction->setVisible(!val.empty()); // turn off shortcut
    if (!val.empty()) {
        QString qFileName = QString::fromUtf8(emuGetPropertyValue(platform + "diskC", "fileName").c_str());
        updateMountToolTip(m_diskCMenuAction, qFileName);
        if (qFileName.isEmpty()) {
            m_diskCUnmountAction->setEnabled(false);
            m_diskCUnmountAction->setText(tr("Unmount"));
            m_diskCMenuAction->setIcon(m_diskCOffIcon);
        } else {
            m_diskCUnmountAction->setEnabled(true);
            qFileName = qFileName.mid(qFileName.lastIndexOf('/') + 1);
            m_diskCUnmountAction->setText(tr("Unmount ") + " " + qFileName);
            m_diskCMenuAction->setIcon(m_diskCOnIcon);
        }
    }
    val = emuGetPropertyValue(platform + "diskC", "readOnly");
    m_diskCReadOnlyAction->setChecked(val == "yes");
    val = emuGetPropertyValue(platform + "diskC", "autoMount");
    m_diskCAutoMountAction->setChecked(val == "yes");

    val = emuGetPropertyValue(platform + "diskD", "label");
    m_diskDMenuAction->setVisible(!val.empty());
    m_diskDAction->setVisible(!val.empty()); // turn off shortcut
    if (!val.empty()) {
        QString qFileName = QString::fromUtf8(emuGetPropertyValue(platform + "diskD", "fileName").c_str());
        updateMountToolTip(m_diskDMenuAction, qFileName);
        if (qFileName.isEmpty()) {
            m_diskDUnmountAction->setEnabled(false);
            m_diskDUnmountAction->setText(tr("Unmount"));
            m_diskDMenuAction->setIcon(m_diskDOffIcon);
        } else {
            m_diskDUnmountAction->setEnabled(true);
            qFileName = qFileName.mid(qFileName.lastIndexOf('/') + 1);
            m_diskDUnmountAction->setText(tr("Unmount ") + " " + qFileName);
            m_diskDMenuAction->setIcon(m_diskDOnIcon);
        }
    }
    val = emuGetPropertyValue(platform + "diskD", "readOnly");
    m_diskDReadOnlyAction->setChecked(val == "yes");
    val = emuGetPropertyValue(platform + "diskD", "autoMount");
    m_diskDAutoMountAction->setChecked(val == "yes");

    val = emuGetPropertyValue(platform + "hdd", "label");
    m_hddMenuAction->setVisible(!val.empty());
    m_hddAction->setVisible(!val.empty()); // turn off shortcut
    m_menuHddSeparator->setVisible(!val.empty());
    if (!val.empty()) {
        QString qFileName = QString::fromUtf8(emuGetPropertyValue(platform + "hdd", "fileName").c_str());
        updateMountToolTip(m_hddMenuAction, qFileName);
        if (qFileName.isEmpty()) {
            m_hddUnmountAction->setEnabled(false);
            m_hddUnmountAction->setText(tr("Unmount"));
            m_hddMenuAction->setIcon(m_hddOffIcon);
        } else {
            m_hddUnmountAction->setEnabled(true);

            if (m_hddLastFiles.getSize() == 0) {
                // add files from cfg if any
                m_hddLastFiles.addToLastFiles(qFileName);
                m_hddLastFiles.tuneActions(m_hddLastFilesActions);
            }

            qFileName = qFileName.mid(qFileName.lastIndexOf('/') + 1);
            m_hddUnmountAction->setText(tr("Unmount ") + " " + qFileName);
            m_hddMenuAction->setIcon(m_hddOnIcon);
        }
    }
    val = emuGetPropertyValue(platform + "hdd", "readOnly");
    m_hddReadOnlyAction->setChecked(val == "yes");
    val = emuGetPropertyValue(platform + "hdd", "autoMount");
    m_hddAutoMountAction->setChecked(val == "yes");


    val = emuGetPropertyValue(platform + "ramDisk", "name");
    m_eddMenuAction->setVisible(!val.empty());
    m_eddAction->setVisible(!val.empty()); // turn off shortcut
    //m_menuEddSeparator->setVisible(!val.empty());
    if (!val.empty()) {
        QString qFileName = QString::fromUtf8(emuGetPropertyValue(platform + "ramDisk", "fileName").c_str());
        updateMountToolTip(m_eddMenuAction, qFileName);
        if (qFileName.isEmpty()) {
            m_eddUnassignAction->setEnabled(false);
            m_eddSaveAction->setEnabled(false);
            m_eddSaveAction->setText(tr("Save"));
            m_eddAutoLoadAction->setEnabled(false);
            m_eddAutoSaveAction->setEnabled(false);
            m_eddMenuAction->setIcon(m_eddOffIcon);

            /*QFont font = m_eddSaveAction->font();
            font.setBold(false);
            m_eddSaveAction->setFont(font);*/
        } else {
            m_eddUnassignAction->setEnabled(true);
            m_eddSaveAction->setEnabled(true);

            if (m_eddLastFiles.getSize() == 0) {
                // add files from cfg if any
                m_eddLastFiles.addToLastFiles(qFileName);
                m_eddLastFiles.tuneActions(m_eddLastFilesActions);
            }

            qFileName = qFileName.mid(qFileName.lastIndexOf('/') + 1);
            m_eddSaveAction->setText(tr("Save ") + " " + qFileName);
            m_eddAutoLoadAction->setEnabled(true);
            m_eddAutoSaveAction->setEnabled(true);
            m_eddMenuAction->setIcon(m_eddOnIcon);

            /*QFont font = m_eddSaveAction->font();
            font.setBold(true);
            m_eddSaveAction->setFont(font);*/
        }
    }
    val = emuGetPropertyValue(platform + "ramDisk", "autoLoad");
    m_eddAutoLoadAction->setChecked(val == "yes");
    val = emuGetPropertyValue(platform + "ramDisk", "autoSave");
    m_eddAutoSaveAction->setChecked(val == "yes");


    val = emuGetPropertyValue(platform + "ramDisk2", "name");
    m_edd2MenuAction->setVisible(!val.empty());
    m_edd2Action->setVisible(!val.empty()); // turn off shortcut
    //m_menuEddSeparator->setVisible(!val.empty());
    if (!val.empty()) {
        QString qFileName = QString::fromUtf8(emuGetPropertyValue(platform + "ramDisk2", "fileName").c_str());
        updateMountToolTip(m_edd2MenuAction, qFileName);
        if (qFileName.isEmpty()) {
            m_edd2UnassignAction->setEnabled(false);
            m_edd2SaveAction->setEnabled(false);
            m_edd2SaveAction->setText(tr("Save"));
            m_edd2AutoLoadAction->setEnabled(false);
            m_edd2AutoSaveAction->setEnabled(false);
            m_edd2MenuAction->setIcon(m_edd2OffIcon);

            /*QFont font = m_edd2SaveAction->font();
            font.setBold(false);
            m_edd2SaveAction->setFont(font);*/
        } else {
            m_edd2UnassignAction->setEnabled(true);
            m_edd2SaveAction->setEnabled(true);

            if (m_eddLastFiles.getSize() == 0) {
                // add files from cfg if any
                m_eddLastFiles.addToLastFiles(qFileName);
                m_eddLastFiles.tuneActions(m_edd2LastFilesActions);
            }

            qFileName = qFileName.mid(qFileName.lastIndexOf('/') + 1);
            m_edd2SaveAction->setText(tr("Save ") + " " + qFileName);
            m_edd2AutoLoadAction->setEnabled(true);
            m_edd2AutoSaveAction->setEnabled(true);
            m_edd2MenuAction->setIcon(m_edd2OnIcon);

            /*QFont font = m_edd2SaveAction->font();
            font.setBold(true);
            m_edd2SaveAction->setFont(font);*/
        }
    }
    val = emuGetPropertyValue(platform + "ramDisk2", "autoLoad");
    m_edd2AutoLoadAction->setChecked(val == "yes");
    val = emuGetPropertyValue(platform + "ramDisk2", "autoSave");
    m_edd2AutoSaveAction->setChecked(val == "yes");


    // Window size menu

    std::string windowStyle = emuGetPropertyValue(platform + "window", "windowStyle");
    std::string frameScale = emuGetPropertyValue(platform + "window", "frameScale");
    bool noPreset = false;
    if (windowStyle == "resizable") {
        if (frameScale == "fit") {
            m_presetFitAction->setChecked(true);
            m_presetAction->setIcon(m_resizableIcon);
        } else
            noPreset = true;
    } else if (windowStyle == "autosize") {
        if (frameScale == "1x") {
            m_preset1xAction->setChecked(true);
            m_presetAction->setIcon(m_1xIcon);
        } else if (frameScale == "2x") {
            m_preset2xAction->setChecked(true);
            m_presetAction->setIcon(m_2xIcon);
        } else if (frameScale == "3x") {
            m_preset3xAction->setChecked(true);
            m_presetAction->setIcon(m_3xIcon);
        } else if (frameScale == "4x") {
            m_preset4xAction->setChecked(true);
            m_presetAction->setIcon(m_4xIcon);
        } else if (frameScale == "5x") {
            m_preset5xAction->setChecked(true);
            m_presetAction->setIcon(m_5xIcon);
        } else if (frameScale == "1.5x") {
            m_preset15xAction->setChecked(true);
            m_presetAction->setIcon(m_15xIcon);
        } else if (frameScale == "2.5x") {
            m_preset25xAction->setChecked(true);
            m_presetAction->setIcon(m_25xIcon);
        } else
            noPreset = true;
    } else
        noPreset = true;

    if (noPreset) {
        m_presetFitAction->setChecked(false);
        m_preset1xAction->setChecked(false);
        m_preset2xAction->setChecked(false);
        m_preset3xAction->setChecked(false);
        m_preset4xAction->setChecked(false);
        m_preset5xAction->setChecked(false);
        m_presetAction->setIcon(m_presetIcon);
    }


    bool disksVisible = m_diskAMenuAction->isVisible() || m_diskBMenuAction->isVisible() || m_diskCMenuAction->isVisible() || m_diskDMenuAction->isVisible();
    m_menuDiskSeparator->setVisible(disksVisible);
    m_toolbarDiskSeparator->setVisible(disksVisible);

    bool eddPresent = !emuGetPropertyValue(platform + "ramDisk", "name").empty() || !emuGetPropertyValue(platform + "ramDisk2", "name").empty();
    m_menuEddSeparator->setVisible(eddPresent);

    val = emuGetPropertyValue(platform + "crtRenderer", "altRenderer");
    if (val == "")
        m_fontAction->setVisible(false);
    else {
        m_fontAction->setVisible(true);
        m_fontAction->setChecked(val == "yes");
    }

    val = emuGetPropertyValue(platform + "crtRenderer", "visibleArea");
    if (val == "")
        m_cropAction->setVisible(false);
    else {
        m_cropAction->setVisible(true);
        m_cropAction->setChecked(val == "yes");
    }

    val = emuGetPropertyValue(platform + "window", "aspectCorrection");
    if (val == "")
        m_aspectAction->setVisible(false);
    else {
        m_aspectAction->setVisible(true);
        m_aspectAction->setChecked(val == "yes");
    }

    val = emuGetPropertyValue(platform + "window", "wideScreen");
    if (val == "")
        m_wideScreenAction->setVisible(false);
    else {
        m_wideScreenAction->setVisible(true);
        m_wideScreenAction->setChecked(val == "yes");
    }

    val = emuGetPropertyValue(platform + "window", "smoothing");
    m_smoothingAction->setVisible(true);
    if (val == "nearest") {
        m_smoothingNearestAction->setChecked(true);
        m_smoothingMenu->setIcon(m_smoothingNearestIcon);
        m_shaderListMenu->menuAction()->setChecked(false);
        m_desaturateAction->setEnabled(true);
    } else if (val == "bilinear") {
        m_smoothingBilinearAction->setChecked(true);
        m_smoothingMenu->setIcon(m_smoothingBilinearIcon);
        m_shaderListMenu->menuAction()->setChecked(false);
        m_desaturateAction->setEnabled(true);
    } else if (val == "sharp") {
        m_smoothingSharpAction->setChecked(true);
        m_smoothingMenu->setIcon(m_smoothingSharpIcon);
        m_shaderListMenu->menuAction()->setChecked(false);
        m_desaturateAction->setEnabled(true);
    } else if (val == "custom") {
        m_smoothingShaderAction->setChecked(true);
        m_smoothingMenu->setIcon(m_shaderIcon);
        m_desaturateAction->setEnabled(false);
    } else
        m_smoothingMenu->setVisible(false);

    QString qVal = QString::fromUtf8(emuGetPropertyValue(platform + "window", "shader").c_str());
    auto actions = m_shaderListMenu->actions();
    foreach(auto action, actions)
        action->setChecked(action->data().toString() == qVal);

    if (qVal != "none") {
        m_smoothingShaderAction->setText(tr("Shader") + ": " + qVal);
        m_smoothingShaderAction->setEnabled(true);
    } else {
        m_smoothingShaderAction->setText(tr("Shader"));
        m_smoothingShaderAction->setEnabled(false);
    }

    val = emuGetPropertyValue(platform + "window", "desaturate");
    m_desaturateAction->setChecked(val == "yes");

    val = emuGetPropertyValue(platform + "tapeGrp", "enabled");
    if (val == "")
        m_tapeHookAction->setVisible(false);
    else {
        m_tapeHookAction->setVisible(true);
        m_tapeHookAction->setChecked(val == "yes");
    }

    val = emuGetPropertyValue(platform + "tapeSoundSource", "muted");
    if (val == "")
        m_muteTapeAction->setVisible(false);
    else {
        m_muteTapeAction->setVisible(true);
        m_muteTapeAction->setChecked(val == "yes");
    }

    val = emuGetPropertyValue(m_palWindow->getPlatformObjectName(), "fastReset");
    if (val == "")
        m_fastResetAction->setVisible(false);
    else {
        m_fastResetAction->setVisible(true);
        m_fastResetAction->setChecked(val == "yes");
    }

    val = emuGetPropertyValue(platform + "kbdLayout", "layout");
    if (val == "jcuken") {
        m_jcukenAction->setChecked(true);
        m_kbdLabel->setText(tr("JCUKEN"));
    } else if (val == "smart") {
        m_smartAction->setChecked(true);
        m_kbdLabel->setText(tr("\"Smart\""));
    } else { // if (val == "qwerty")
        m_qwertyAction->setChecked(true);
        m_kbdLabel->setText("QWERTY");
    }

    val = emuGetPropertyValue(platform + "crtRenderer", "colorMode");
    if (val != "" && m_colorModeMenu) {
        QList<QAction*> list = m_colorModeMenu->actions();
        for (auto it = list.begin(); it != list.end(); it++) {
            if ((*it)->data().toString().toUtf8().constData() == val) {
                (*it)->setChecked(true);
                m_colorLabel->setText((*it)->text().replace("&&", "&"));
                break;
            }
        }
    }

    m_wideScreenAction->setEnabled(emuGetPropertyValue(m_palWindow->getPlatformObjectName() + ".window", "wideScreen") != "custom");

    m_printerCaptureAction->setChecked(!emuGetPropertyValue("prnWriter", "fileName").empty());
}


void MainWindow::savePosition()
{
    if (!m_palWindow || m_windowType != EWT_EMULATION || !isVisible() || m_fullscreenMode)
        return;

    QSettings settings;
    SET_INI_CODEC(settings);
    settings.beginGroup("window");
    if (geometry().topLeft() != QPoint(0, 0)) { // don't save incorrect position
        settings.setValue("left", geometry().x());
        settings.setValue("top", geometry().y());
    }

    bool resizable = m_clientWidth == 0 && m_clientHeight == 0;
    if (resizable || m_settingsDialog->getOptionValue("preserveSize") != "yes") {
        settings.setValue("width", m_paintWidget->width());
        settings.setValue("height", m_paintWidget->height());
    }

    settings.endGroup();
}


void MainWindow::updateLastFiles()
{
    m_fddLastFiles.tuneActions(m_fddALastFilesActions);
    m_fddLastFiles.tuneActions(m_fddBLastFilesActions);
    m_fddLastFiles.tuneActions(m_fddCLastFilesActions);
    m_fddLastFiles.tuneActions(m_fddDLastFilesActions);
    m_hddLastFiles.tuneActions(m_hddLastFilesActions);
    m_eddLastFiles.tuneActions(m_eddLastFilesActions);
    m_eddLastFiles.tuneActions(m_edd2LastFilesActions);
    m_loaderLastFiles.tuneActions(m_loadLastFilesActions);
    m_loaderLastFiles.tuneActions(m_loadRunLastFilesActions);
}

void MainWindow::updateLastPlatforms(QString platform)
{
    if (m_lastPlatformsActions[0]->isVisible() && m_lastPlatformsActions[0]->data().toString() == platform)
        return;

    for (int i = 0; i < LAST_PLATFORMS_QTY; i++)
        if (platform == m_lastPlatformsActions[i]->data()) {
            for (int j = i; j < LAST_PLATFORMS_QTY - 1; j++) {
                m_lastPlatformsActions[j]->setData(m_lastPlatformsActions[j + 1]->data().toString());
                m_lastPlatformsActions[j]->setText(m_lastPlatformsActions[j + 1]->text());
                m_lastPlatformsActions[j]->setVisible(m_lastPlatformsActions[j + 1]->isVisible());
            }
            m_lastPlatformsActions[LAST_PLATFORMS_QTY - 1]->setData("");
            m_lastPlatformsActions[LAST_PLATFORMS_QTY - 1]->setText("");
            m_lastPlatformsActions[LAST_PLATFORMS_QTY - 1]->setVisible(false);
            break;
        }

    for (int i = LAST_PLATFORMS_QTY - 1; i > 0; i--) {
        m_lastPlatformsActions[i]->setData(m_lastPlatformsActions[i - 1]->data().toString());
        m_lastPlatformsActions[i]->setText(m_lastPlatformsActions[i - 1]->text());
        m_lastPlatformsActions[i]->setVisible(m_lastPlatformsActions[i - 1]->isVisible());
    }

    m_lastPlatformsActions[0]->setData(platform);
    m_lastPlatformsActions[0]->setText(m_platformNames[platform]);
    m_lastPlatformsActions[0]->setVisible(true);

    QSettings settings;
    SET_INI_CODEC(settings);
    settings.beginGroup("Last_platforms");

    for (int i = 0; i < LAST_PLATFORMS_QTY; i++)
        settings.setValue(QString::number(i), m_lastPlatformsActions[i]->data().toString());

    settings.endGroup();
}



// LastFileList implementation

void LastFileList::setPlatformName(const QString& name)
{
    m_platform = name;
    m_loaded = false;
}


QString LastFileList::getKeyPrefix()
{
    return m_platform + "/" + m_type + "/";
}


void LastFileList::loadLastFiles()
{
    QString keyPrefix = getKeyPrefix();

    QSettings settings;
    SET_INI_CODEC(settings);
    settings.beginGroup("Last_files");

    m_list.clear();
    for (int i = 1; i <= LAST_FILES_QTY; i++) {
        QString value = settings.value(keyPrefix + QString::number(i)).toString();
        if (!value.isEmpty())
            m_list.append(value);
    }

    settings.endGroup();
}


void LastFileList::addToLastFiles(const QString& fileName)
{
    if (fileName.isEmpty()) // just in case
        return;

    int i = 0;
    for (auto it = m_list.begin(); it != m_list.end(); it++, i++)
        if (fileName == *it) {
            // if item is already in list, move it to the top
            m_list.move(i, 0);
            saveLastFiles();
            return;
        }

    m_list.prepend(fileName);

    if (m_list.size() > LAST_FILES_QTY)
        m_list.removeLast();

    saveLastFiles();
}


void LastFileList::saveLastFiles()
{
    QString keyPrefix = getKeyPrefix();

    QSettings settings;
    SET_INI_CODEC(settings);
    settings.beginGroup("Last_files");

    int i = 1;
    for (auto it = m_list.begin(); it != m_list.end(); it++, i++)
        settings.setValue(keyPrefix + QString::number(i), *it);

    settings.endGroup();

    m_loaded = true;
}


QString LastFileList::getLastFile()
{
    if (!m_list.isEmpty()) {
        return *m_list.begin();
    } else {
        return "";
    }
}


void LastFileList::tuneActions(QAction** actions)
{
    if (!m_loaded)
        loadLastFiles();

    int i = 0;
    for (auto it = m_list.begin(); it != m_list.end(); it++, i++) {
        actions[i]->setText(*it);
        actions[i]->setVisible(true);
    }
    for (; i < LAST_FILES_QTY; i++) {
        actions[i]->setVisible(false);
    }
}
