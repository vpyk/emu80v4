/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2019
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
#include <QDesktopWidget>
#include <QScreen>

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

    //setWindowFlags(windowFlags() |= Qt::WindowMaximizeButtonHint);
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
        m_windowType = EWT_UNDEFINED;
        return;
    }

    m_windowType = palWindow->getWindowType();

    if (!m_palWindow && m_windowType == EWT_EMULATION) {
        // first emulation window
        createActions();
        fillPlatformListMenu();
    }
    m_palWindow = palWindow;
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
            m_imageSizeLabel = new QLabel("", this);
            m_tapeLabel = new QLabel("", this);
            m_tapeLabel->setVisible(false);
            m_wavLabel = new QLabel("", this);
            m_wavLabel->setVisible(false);

            m_statusBar = statusBar();
            m_statusBar->addWidget(m_fpsLabel);
            m_statusBar->addWidget(m_speedLabel);
            m_statusBar->addWidget(m_kbdLabel);
            m_statusBar->addWidget(m_colorLabel);
            m_statusBar->addWidget(m_tapeLabel);
            m_statusBar->addWidget(m_wavLabel);
            m_statusBar->addWidget(m_crtModeLabel);
            m_statusBar->addWidget(m_imageSizeLabel);
        }

        tuneMenu();

        if (m_fullscreenMode)
            setFullScreen(false);

        adjustClientSize();

        m_settingsDialog->initConfig();
        updateConfig(); // немного избыточно

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
        break;
    default:
        break;
    }
    m_controlsCreated = true;
}


std::string MainWindow::getPlatformObjectName()
{
    if (m_palWindow)
        return m_palWindow->getPlatformObjectName();
    else
        return "";
}


std::string MainWindow::getPlatformGroupName()
{
    std::string platform = getPlatformObjectName();
    std::string::size_type dotPos = platform.find(".",0);
    return platform.substr(0, dotPos);
}


void MainWindow::setClientSize(int width, int height)
{
    m_clientWidth = width;
    m_clientHeight = height;

    adjustClientSize();
}


void MainWindow::adjustClientSize()
{
    bool sizeable = m_clientWidth == 0 && m_clientHeight == 0;

    if (sizeable || m_fullscreenMode) {
        m_paintWidget->setFixedSize(m_paintWidget->width(), m_paintWidget->height());
        if (!m_fullscreenMode)
            adjustSize();
        m_paintWidget->setFixedSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
        layout()->setSizeConstraint(QLayout::SetNoConstraint);
        setFixedSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
    } else {
        m_paintWidget->setFixedSize(m_clientWidth, m_clientHeight);
        layout()->setSizeConstraint(QLayout::SetFixedSize);
    }

    if (m_statusBar)
        m_statusBar->setSizeGripEnabled(sizeable);
}


void MainWindow::showWindow()
{
    // workaround for miximize button
    if (!isVisible()) {
        setFixedSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
        setWindowFlags(windowFlags() |= Qt::WindowMaximizeButtonHint);
        adjustClientSize();
        show();

        if (m_showFirstTime) {
            m_showFirstTime = false;
            if (m_windowType == EWT_EMULATION) {
                // center main window, not debug one
                QRect rec = QGuiApplication::primaryScreen()->availableGeometry();
                move((rec.width() - frameGeometry().width()) / 3, (rec.height() - frameGeometry().height()) / 3);

                HelpDialog::activate();
            }
            else { //if (m_windowType == EWT_DEBUG) {
                // place debug window within current screen rect
                int sn = QApplication::desktop()->screenNumber(this);
                QRect rec = QGuiApplication::screens()[sn]->availableGeometry();

                int top = frameGeometry().top();
                int left = frameGeometry().left();
                if (frameGeometry().bottom() > rec.bottom())
                    top = top + rec.bottom() - frameGeometry().bottom();
                if (frameGeometry().right() > rec.right())
                    left = left + rec.right() - frameGeometry().right();

                move(left, top);
            }
        }

    }
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
        layout()->setSizeConstraint(QLayout::SetNoConstraint);
        showFullScreen();
    } else
        showNormal();
    m_fullscreenMode = fullscreen;
    adjustClientSize();
}


void MainWindow::fillPlatformListMenu()
{
    const std::vector<PlatformInfo>* platforms = emuGetPlatforms();
    for (auto it = platforms->begin(); it != platforms->end(); it++) {
        QAction* action = new QAction(QString::fromUtf8((*it).platformName.c_str()), m_platformListMenu);
        action->setData(QString::fromUtf8((*it).objName.c_str()));
        m_platformListMenu->addAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(onPlatformSelect()));
    }
}


#ifndef __APPLE__
    #define ADD_HOTKEY(keyList, key) \
    keyList.append(QKeySequence(Qt::ALT + key)); \
    keyList.append(QKeySequence(Qt::META + key))
#else
    #define ADD_HOTKEY(keyList, key) \
    keyList.append(QKeySequence(Qt::ALT + key))
#endif


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
    m_loadRunAction = new QAction(QIcon(":/icons/open_run.png"), tr("Load && Run..."), this);
    m_loadRunAction->setToolTip(tr("Load file and run (Alt-F3)"));
    QList<QKeySequence> loadRunKeysList;
    ADD_HOTKEY(loadRunKeysList, Qt::Key_F3);
    //loadRunKeysList.append(QKeySequence(Qt::ALT + Qt::Key_F3));
    //loadRunKeysList.append(QKeySequence(Qt::META + Qt::Key_F3));
    m_loadRunAction->setShortcuts(loadRunKeysList);
    addAction(m_loadRunAction);
    fileMenu->addAction(m_loadRunAction);
    m_toolBar->addAction(m_loadRunAction);
    connect(m_loadRunAction, SIGNAL(triggered()), this, SLOT(onLoadRun()));

    // Load
    m_loadAction = new QAction(QIcon(":/icons/open.png"), tr("Load..."), this);
    m_loadAction->setToolTip(tr("Load file (Alt-L)"));
    QList<QKeySequence> loadKeysList;
    ADD_HOTKEY(loadKeysList, Qt::Key_L);
    //loadKeysList.append(QKeySequence(Qt::ALT + Qt::Key_L));
    //loadKeysList.append(QKeySequence(Qt::META + Qt::Key_L));
    m_loadAction->setShortcuts(loadKeysList);
    addAction(m_loadAction);
    fileMenu->addAction(m_loadAction);
    m_toolBar->addAction(m_loadAction);
    connect(m_loadAction, SIGNAL(triggered()), this, SLOT(onLoad()));

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
    m_toolBar->addAction(m_loadWavAction);
    connect(m_loadWavAction, SIGNAL(triggered()), this, SLOT(onLoadWav()));

    fileMenu->addSeparator();
    m_toolBar->addSeparator();

    // Select disk A
    m_diskAAction = new QAction(QIcon(":/icons/disk_a.png"), tr("Disk A..."), this);
    m_diskAAction->setToolTip(tr("Load disk A image (Alt-A)"));
    QList<QKeySequence> diskAKeysList;
    ADD_HOTKEY(diskAKeysList, Qt::Key_A);
    //diskAKeysList.append(QKeySequence(Qt::ALT + Qt::Key_A));
    //diskAKeysList.append(QKeySequence(Qt::META + Qt::Key_A));
    m_diskAAction->setShortcuts(diskAKeysList);
    addAction(m_diskAAction);
    fileMenu->addAction(m_diskAAction);
    m_toolBar->addAction(m_diskAAction);
    connect(m_diskAAction, SIGNAL(triggered()), this, SLOT(onDiskA()));

    // Select disk B
    m_diskBAction = new QAction(QIcon(":/icons/disk_b.png"), tr("Disk B..."), this);
    m_diskBAction->setToolTip(tr("Load disk B image (Alt-B)"));
    QList<QKeySequence> diskBKeysList;
    ADD_HOTKEY(diskBKeysList, Qt::Key_B);
    //diskBKeysList.append(QKeySequence(Qt::ALT + Qt::Key_B));
    //diskBKeysList.append(QKeySequence(Qt::META + Qt::Key_B));
    m_diskBAction->setShortcuts(diskBKeysList);
    addAction(m_diskBAction);
    fileMenu->addAction(m_diskBAction);
    m_toolBar->addAction(m_diskBAction);
    connect(m_diskBAction, SIGNAL(triggered()), this, SLOT(onDiskB()));

    m_menuDiskSeparator = fileMenu->addSeparator();

    // Load RAM disk
    m_loadRamDiskAction = new QAction(QIcon(":/icons/edd.png"), tr("Load RAM Disk..."), this);
    m_loadRamDiskAction->setToolTip(tr("Load RAM Disk image"));
    QList<QKeySequence> loadRamDiskKeysList;
    ADD_HOTKEY(loadRamDiskKeysList, Qt::Key_E);
    //loadRamDiskKeysList.append(QKeySequence(Qt::ALT + Qt::Key_E));
    //loadRamDiskKeysList.append(QKeySequence(Qt::META + Qt::Key_E));
    m_loadRamDiskAction->setShortcuts(loadRamDiskKeysList);
    addAction(m_loadRamDiskAction);
    fileMenu->addAction(m_loadRamDiskAction);
    m_toolBar->addAction(m_loadRamDiskAction);
    connect(m_loadRamDiskAction, SIGNAL(triggered()), this, SLOT(onLoadRamDisk()));

    m_toolbarDiskSeparator = m_toolBar->addSeparator();

    // Save RAM disk
    m_saveRamDiskAction = new QAction(tr("Save RAM Disk..."), this);
    //m_saveRamDiskAction->setToolTip(tr("Save RAM Disk image"));
    fileMenu->addAction(m_saveRamDiskAction);
    connect(m_saveRamDiskAction, SIGNAL(triggered()), this, SLOT(onSaveRamDisk()));

    m_ramDiskSeparator = fileMenu->addSeparator();

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

    platformMenu->addSeparator();

    // Debug
    m_debugAction = new QAction(QIcon(":/icons/debug.png"), tr("Debug..."), this);
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
    ADD_HOTKEY(copyTextKeysList, Qt::SHIFT + Qt::Key_Insert);
    //copyTextKeysList.append(QKeySequence(Qt::ALT + Qt::SHIFT + Qt::Key_Insert));
    //copyTextKeysList.append(QKeySequence(Qt::META + Qt::SHIFT + Qt::Key_Insert));
    m_copyTextAction->setShortcuts(copyTextKeysList);
    addAction(m_copyTextAction);
    connect(m_copyTextAction, SIGNAL(triggered()), this, SLOT(onCopyText()));

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
    m_layoutButton->setIcon(QIcon(":/icons/keyboard.png"));
    m_layoutButton->setToolTip(tr("Keyboard layout"));
    m_layoutButton->setMenu(layoutMenu);
    m_layoutButton->setPopupMode(QToolButton::InstantPopup);
    m_toolBar->addWidget(m_layoutButton);

    // Tape hook on/off
    m_tapeHookAction = new QAction(QIcon(":/icons/tape.png"), tr("Tape hook"), this);
    m_tapeHookAction->setCheckable(true);
    m_tapeHookAction->setToolTip(tr("Tape hook"));
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
    m_cropAction = new QAction(QIcon(":/icons/crop.png"), tr("Visible area"), this);
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
    m_aspectAction = new QAction(QIcon(":/icons/aspect.png"), tr("Aspect"), this);
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
    m_wideScreenAction = new QAction(QIcon(":/icons/wide.png"), tr("Wide screen (16:9)"), this);
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
    m_smoothingAction = new QAction(QIcon(":/icons/smooth.png"), tr("Smoothing"), this);
    m_smoothingAction->setCheckable(true);
    m_smoothingAction->setToolTip(tr("Smoothing (Alt-S)"));
    QList<QKeySequence> smoothingKeysList;
    ADD_HOTKEY(smoothingKeysList, Qt::Key_S);
    //smoothingKeysList.append(QKeySequence(Qt::ALT + Qt::Key_S));
    //smoothingKeysList.append(QKeySequence(Qt::META + Qt::Key_S));
    m_smoothingAction->setShortcuts(smoothingKeysList);
    addAction(m_smoothingAction);
    m_toolBar->addAction(m_smoothingAction);
    settingsMenu->addAction(m_smoothingAction);
    connect(m_smoothingAction, SIGNAL(triggered()), this, SLOT(onSmoothing()));

    // Font
    m_fontAction = new QAction(QIcon(":/icons/font.png"), tr("Font"), this);
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

    // Mute
    m_muteAction = new QAction(QIcon(":/icons/mute.png"), tr("Mute"), this);
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

    QMenu* presetMenu = viewMenu->addMenu(tr("Presets"));

    // 1x preset
    m_preset1xAction = new QAction(tr("Preset: 1x"), this);
    //m_preset1xAction->setToolTip(tr("Preset: 1x (Alt-1)"));
    QList<QKeySequence> preset1xKeysList;
    ADD_HOTKEY(preset1xKeysList, Qt::Key_1);
    //preset1xKeysList.append(QKeySequence(Qt::ALT + Qt::Key_1));
    //preset1xKeysList.append(QKeySequence(Qt::META + Qt::Key_1));
    m_preset1xAction->setShortcuts(preset1xKeysList);
    addAction(m_preset1xAction);
    presetMenu->addAction(m_preset1xAction);
    connect(m_preset1xAction, SIGNAL(triggered()), this, SLOT(on1x()));

    // 2x preset
    m_preset2xAction = new QAction(tr("Preset: 2x"), this);
    //m_preset2xAction->setToolTip(tr("Preset: 2x (Alt-2)"));
    QList<QKeySequence> preset2xKeysList;
    ADD_HOTKEY(preset2xKeysList, Qt::Key_2);
    //preset2xKeysList.append(QKeySequence(Qt::ALT + Qt::Key_2));
    //preset2xKeysList.append(QKeySequence(Qt::META + Qt::Key_2));
    m_preset2xAction->setShortcuts(preset2xKeysList);
    addAction(m_preset2xAction);
    presetMenu->addAction(m_preset2xAction);
    connect(m_preset2xAction, SIGNAL(triggered()), this, SLOT(on2x()));

    // 3x preset
    m_preset3xAction = new QAction(tr("Preset: 3x"), this);
    //m_preset3xAction->setToolTip(tr("Preset: 3x (Alt-3)"));
    QList<QKeySequence> preset3xKeysList;
    ADD_HOTKEY(preset3xKeysList, Qt::Key_3);
    //preset3xKeysList.append(QKeySequence(Qt::ALT + Qt::Key_3));
    //preset3xKeysList.append(QKeySequence(Qt::META + Qt::Key_3));
    m_preset3xAction->setShortcuts(preset3xKeysList);
    addAction(m_preset3xAction);
    presetMenu->addAction(m_preset3xAction);
    connect(m_preset3xAction, SIGNAL(triggered()), this, SLOT(on3x()));

    // 4x preset
    m_preset4xAction = new QAction(tr("Preset: 4x"), this);
    //m_preset4xAction->setToolTip(tr("Preset: 4x (Alt-4)"));
    QList<QKeySequence> preset4xKeysList;
    ADD_HOTKEY(preset4xKeysList, Qt::Key_4);
    //preset4xKeysList.append(QKeySequence(Qt::ALT + Qt::Key_4));
    //preset4xKeysList.append(QKeySequence(Qt::META + Qt::Key_4));
    m_preset4xAction->setShortcuts(preset4xKeysList);
    addAction(m_preset4xAction);
    presetMenu->addAction(m_preset4xAction);
    connect(m_preset4xAction, SIGNAL(triggered()), this, SLOT(on4x()));

    // 5x preset
    m_preset5xAction = new QAction(tr("Preset: 5x"), this);
    //m_preset5xAction->setToolTip(tr("Preset: 5x (Alt-5)"));
    QList<QKeySequence> preset5xKeysList;
    ADD_HOTKEY(preset5xKeysList, Qt::Key_5);
    //preset5xKeysList.append(QKeySequence(Qt::ALT + Qt::Key_5));
    //preset5xKeysList.append(QKeySequence(Qt::META + Qt::Key_5));
    m_preset5xAction->setShortcuts(preset5xKeysList);
    addAction(m_preset5xAction);
    presetMenu->addAction(m_preset5xAction);
    connect(m_preset5xAction, SIGNAL(triggered()), this, SLOT(on5x()));

    presetMenu->addSeparator();

    // 2x3 preset
    m_preset2x3Action = new QAction(tr("Preset: 2x3"), this);
    //m_preset2x3Action->setToolTip(tr("Preset: 2x3 (Alt-6)"));
    QList<QKeySequence> preset2x3KeysList;
    ADD_HOTKEY(preset2x3KeysList, Qt::Key_6);
    //preset2x3KeysList.append(QKeySequence(Qt::ALT + Qt::Key_6));
    //preset2x3KeysList.append(QKeySequence(Qt::META + Qt::Key_6));
    m_preset2x3Action->setShortcuts(preset2x3KeysList);
    addAction(m_preset2x3Action);
    presetMenu->addAction(m_preset2x3Action);
    connect(m_preset2x3Action, SIGNAL(triggered()), this, SLOT(on2x3()));

    // 3x5 preset
    m_preset3x5Action = new QAction(tr("Preset: 3x5"), this);
    //m_preset3x5Action->setToolTip(tr("Preset: 3x5 (Alt-7)"));
    QList<QKeySequence> preset3x5KeysList;
    ADD_HOTKEY(preset3x5KeysList, Qt::Key_7);
    //preset3x5KeysList.append(QKeySequence(Qt::ALT + Qt::Key_7));
    //preset3x5KeysList.append(QKeySequence(Qt::META + Qt::Key_7));
    m_preset3x5Action->setShortcuts(preset3x5KeysList);
    addAction(m_preset3x5Action);
    presetMenu->addAction(m_preset3x5Action);
    connect(m_preset3x5Action, SIGNAL(triggered()), this, SLOT(on3x5()));

    // 4x6 preset
    m_preset4x6Action = new QAction(tr("Preset: 4x6"), this);
    //m_preset4x6Action->setToolTip(tr("Preset: 4x6 (Alt-8)"));
    QList<QKeySequence> preset4x6KeysList;
    ADD_HOTKEY(preset4x6KeysList, Qt::Key_8);
    //preset4x6KeysList.append(QKeySequence(Qt::ALT + Qt::Key_8));
    //preset4x6KeysList.append(QKeySequence(Qt::META + Qt::Key_8));
    m_preset4x6Action->setShortcuts(preset4x6KeysList);
    addAction(m_preset4x6Action);
    presetMenu->addAction(m_preset4x6Action);
    connect(m_preset4x6Action, SIGNAL(triggered()), this, SLOT(on4x6()));

    presetMenu->addSeparator();

    // Stretch preset
    m_presetStretchAction = new QAction(tr("Preset: Stretch"), this);
    //m_presetStretcgAction->setToolTip(tr("Preset: Stretch (Alt-9)"));
    QList<QKeySequence> presetStretchKeysList;
    ADD_HOTKEY(presetStretchKeysList, Qt::Key_9);
    //presetStretchKeysList.append(QKeySequence(Qt::ALT + Qt::Key_9));
    //presetStretchKeysList.append(QKeySequence(Qt::META + Qt::Key_9));
    m_presetStretchAction->setShortcuts(presetStretchKeysList);
    addAction(m_presetStretchAction);
    presetMenu->addAction(m_presetStretchAction);
    connect(m_presetStretchAction, SIGNAL(triggered()), this, SLOT(onStretch()));

    // Fit preset
    m_presetFitAction = new QAction(tr("Preset: Fit"), this);
    //m_presetFitAction->setToolTip(tr("Preset: Fit (Alt-0)"));
    QList<QKeySequence> presetFitKeysList;
    ADD_HOTKEY(presetFitKeysList, Qt::Key_0);
    //presetFitKeysList.append(QKeySequence(Qt::ALT + Qt::Key_0));
    //presetFitKeysList.append(QKeySequence(Qt::META + Qt::Key_0));
    m_presetFitAction->setShortcuts(presetFitKeysList);
    addAction(m_presetFitAction);
    presetMenu->addAction(m_presetFitAction);
    connect(m_presetFitAction, SIGNAL(triggered()), this, SLOT(onFit()));

    viewMenu->addSeparator();
    viewMenu->addAction(m_screenshotAction);
    viewMenu->addAction(m_copyImageAction);
    viewMenu->addAction(m_copyTextAction);
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

    m_presetButton = new QToolButtonWA(this);
    m_presetButton->setFocusPolicy(Qt::NoFocus);
    m_presetButton->setIcon(QIcon(":/icons/preset.png"));
    m_presetButton->setToolTip(tr("Window preset"));
    m_presetButton->setMenu(presetMenu);
    m_presetButton->setPopupMode(QToolButton::InstantPopup);
    m_toolBar->addWidget(m_presetButton);

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

    if (platformGroup == "rk86") {
        hasColor = true;
        m_colorLabel->setVisible(false);

        m_colorMonoAction->setVisible(true);
        m_colorMonoAction->setEnabled(true);

        m_colorColor1Action->setVisible(true);
        m_colorColor1Action->setEnabled(true);
        m_colorColor1Action->setText(tr("Color (Tolkalin)"));
        m_colorColor1Action->setData("color1");

        m_colorColor2Action->setVisible(true);
        m_colorColor2Action->setEnabled(true);
        m_colorColor2Action->setText(tr("Color (Akimenko)"));
        m_colorColor2Action->setData("color2");
    } else if (platformGroup == "apogey" || platformGroup == "orion" || platformGroup == "lvov" || platformGroup == "vector" || platformGroup == "pk8000") {
        hasColor = true;

        m_colorMonoAction->setVisible(true);
        m_colorMonoAction->setEnabled(true);

        m_colorColor1Action->setVisible(true);
        m_colorColor1Action->setEnabled(true);
        m_colorColor1Action->setText(tr("Color"));
        m_colorColor1Action->setData("color");

        m_colorColor2Action->setVisible(false);
        m_colorColor2Action->setEnabled(false);
    } else if (platformGroup == "spec") {
        hasColor = true;

        m_colorMonoAction->setVisible(true);
        m_colorMonoAction->setEnabled(true);

        m_colorColor1Action->setVisible(true);
        m_colorColor1Action->setEnabled(true);
        m_colorColor1Action->setText(tr("4-color"));
        m_colorColor1Action->setData("4color");

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
                                 platformGroup == "mikro80" || platformGroup == "ut88");

    m_platformConfigAction->setVisible(PlatformConfigDialog::hasConfig(QString::fromUtf8(getPlatformObjectName().c_str())));
}


void MainWindow::createDebugActions()
{
    /*m_menuBar = menuBar();
    m_menuBar->setContextMenuPolicy(Qt::PreventContextMenu);
    m_toolBar = new QToolBar(this);
    m_toolBar->setFloatable(false);
    //m_toolBar->setMovable(false);
    m_toolBar->setContextMenuPolicy(Qt::PreventContextMenu);
    m_toolBar->setIconSize(QSize(16, 16));
    addToolBar(m_toolBar);
    m_toolBar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);*/

    // 1x preset
    m_preset1xAction = new QAction(tr("Preset: 1x"), this /*m_menuBar*/);
    //m_preset1xAction->setToolTip(tr("Preset: 1x (Alt-1)"));
    QList<QKeySequence> preset1xKeysList;
    ADD_HOTKEY(preset1xKeysList, Qt::Key_1);
    //preset1xKeysList.append(QKeySequence(Qt::ALT + Qt::Key_1));
    //preset1xKeysList.append(QKeySequence(Qt::META + Qt::Key_1));
    m_preset1xAction->setShortcuts(preset1xKeysList);
    addAction(m_preset1xAction);
    //presetMenu->addAction(m_preset1xAction);
    connect(m_preset1xAction, SIGNAL(triggered()), this, SLOT(on1x()));

    // 2x preset
    m_preset2xAction = new QAction(tr("Preset: 2x"), this /*m_menuBar*/);
    //m_preset2xAction->setToolTip(tr("Preset: 2x (Alt-2)"));
    QList<QKeySequence> preset2xKeysList;
    ADD_HOTKEY(preset2xKeysList, Qt::Key_2);
    //preset2xKeysList.append(QKeySequence(Qt::ALT + Qt::Key_2));
    //preset2xKeysList.append(QKeySequence(Qt::META + Qt::Key_2));
    m_preset2xAction->setShortcuts(preset2xKeysList);
    addAction(m_preset2xAction);
    //presetMenu->addAction(m_preset2xAction);
    connect(m_preset2xAction, SIGNAL(triggered()), this, SLOT(on2x()));

    // 3x preset
    m_preset3xAction = new QAction(tr("Preset: 3x"), this /*m_menuBar*/);
    //m_preset3xAction->setToolTip(tr("Preset: 3x (Alt-3)"));
    QList<QKeySequence> preset3xKeysList;
    ADD_HOTKEY(preset3xKeysList, Qt::Key_3);
    //preset3xKeysList.append(QKeySequence(Qt::ALT + Qt::Key_3));
    //preset3xKeysList.append(QKeySequence(Qt::META + Qt::Key_3));
    m_preset3xAction->setShortcuts(preset3xKeysList);
    addAction(m_preset3xAction);
    //presetMenu->addAction(m_preset3xAction);
    connect(m_preset3xAction, SIGNAL(triggered()), this, SLOT(on3x()));

    // Fit preset
    m_presetFitAction = new QAction(tr("Preset: Fit"), this /*m_menuBar*/);
    //m_presetFitAction->setToolTip(tr("Preset: Fit (Alt-0)"));
    QList<QKeySequence> presetFitKeysList;
    ADD_HOTKEY(presetFitKeysList, Qt::Key_0);
    //presetFitKeysList.append(QKeySequence(Qt::ALT + Qt::Key_0));
    //presetFitKeysList.append(QKeySequence(Qt::META + Qt::Key_0));
    m_presetFitAction->setShortcuts(presetFitKeysList);
    addAction(m_presetFitAction);
    //presetMenu->addAction(m_presetFitAction);
    connect(m_presetFitAction, SIGNAL(triggered()), this, SLOT(onFit()));

    // Stretch preset
    m_presetStretchAction = new QAction(tr("Preset: Stretch"), this /*m_menuBar*/);
    //m_presetStretchAction->setToolTip(tr("Preset: Stretch (Alt-9)"));
    QList<QKeySequence> presetStretchKeysList;
    ADD_HOTKEY(presetStretchKeysList, Qt::Key_9);
    //presetStretchKeysList.append(QKeySequence(Qt::ALT + Qt::Key_9));
    //presetStretchKeysList.append(QKeySequence(Qt::META + Qt::Key_9));
    m_presetStretchAction->setShortcuts(presetStretchKeysList);
    addAction(m_presetStretchAction);
    //presetMenu->addAction(m_presetStretchAction);
    connect(m_presetStretchAction, SIGNAL(triggered()), this, SLOT(onStretch()));

    //viewMenu->addSeparator();

    m_fullscreenAction = new QAction(tr("Fullscreen mode"), this /*m_menuBar*/);
    //m_fullscreenAction->setCheckable(true);
    //setToolTip(tr("Fullscreen mode (Alt-Enter)"));
    QList<QKeySequence> fullscreenKeysList;
    ADD_HOTKEY(fullscreenKeysList, Qt::Key_Return);
    //fullscreenKeysList.append(QKeySequence(Qt::ALT + Qt::Key_Return));
    //fullscreenKeysList.append(QKeySequence(Qt::META + Qt::Key_Return));
    m_fullscreenAction->setShortcuts(fullscreenKeysList);
    connect(m_fullscreenAction, SIGNAL(triggered()), this, SLOT(onFullscreen()));
    addAction(m_fullscreenAction);
    //viewMenu->addAction(m_fullscreenAction);

    m_fullwindowAction = new QAction(tr("Hide menu and buttons"), this /*m_menuBar*/);
    //m_fullwindowAction->setCheckable(true);
    //setToolTip(tr("Hide menu and buttons (Alt-\)"));
    QList<QKeySequence> fullwindowKeysList;
    ADD_HOTKEY(fullwindowKeysList, Qt::Key_Backslash);
    //fullwindowKeysList.append(QKeySequence(Qt::ALT + Qt::Key_Backslash));
    //fullwindowKeysList.append(QKeySequence(Qt::META + Qt::Key_Backslash));
    m_fullwindowAction->setShortcuts(fullwindowKeysList);
    connect(m_fullwindowAction, SIGNAL(triggered()), this, SLOT(onFullwindow()));
    addAction(m_fullwindowAction);
    //viewMenu->addAction(m_fullwindowAction);
}


void MainWindow::incFrameCount()
{
    if (m_frameCount == 0)
        m_firstFpsCoutnerFrameTime = palGetCounter();
    else
        m_lastFpsCoutnerFrameTime = palGetCounter();
    ++m_frameCount;
};


void MainWindow::mouseClick(int x, int y, PalMouseKey key)
{
    if (m_windowType != EWT_DEBUG)
        return;

    m_palWindow->mouseClick(x, y, key);
}


void MainWindow::onFpsTimer()
{
    uint64_t delta = m_lastFpsCoutnerFrameTime - m_firstFpsCoutnerFrameTime;

    if (m_windowType != EWT_EMULATION)
        return;

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


    unsigned speed = emuGetEmulationSpeedFactor();
    m_speedLabel->setVisible(speed != 1);
    m_speedLabel->setText(speed ? QString::number(speed) + "x" : tr("Paused"));


    std::string platform = m_palWindow->getPlatformObjectName() + ".";

    std::string crtMode = "";
    if (m_palWindow)
        crtMode= emuGetPropertyValue(platform + "crtRenderer", "crtMode");
    m_crtModeLabel->setText(QString::fromUtf8(crtMode.c_str()));
    m_crtModeLabel->setVisible(crtMode != "");

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
        emuSysReq(m_palWindow, SR_SPEEDUP);
        return;
    }
    unsigned unicodeKey = evt->text()[0].unicode();
    emuKeyboard(m_palWindow, translateKey(evt), true, unicodeKey);
}


void MainWindow::keyReleaseEvent(QKeyEvent* evt)
{
    if (evt->key() == Qt::Key_End && !(evt->modifiers() & Qt::KeypadModifier)) {
        emuSysReq(m_palWindow, SR_SPEEDNORMAL);
        return;
    }
    unsigned unicodeKey = evt->text()[0].unicode();
    emuKeyboard(m_palWindow, translateKey(evt), false, unicodeKey);
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    emuSysReq(m_palWindow, SR_CLOSE);
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


void MainWindow::onReset()
{
    emuSysReq(m_palWindow, SR_RESET);
}


void MainWindow::onPause()
{
    emuSysReq(m_palWindow, SR_PAUSE);
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


void MainWindow::on2x3()
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
}


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
}


void MainWindow::onLoadRun()
{
    emuSysReq(m_palWindow, SR_LOADRUN);
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
}


void MainWindow::onDiskB()
{
    emuSysReq(m_palWindow, SR_DISKB);
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
    emuSysReq(m_palWindow, SR_ANTIALIASING);
    saveConfig();
}


void MainWindow::onPlatformSelect()
{
    QAction* action = (QAction*)sender();
    std::string platform(action->data().toString().toUtf8().constData());

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


void MainWindow::onPlatformHelp()
{
    std::string helpFile = palMakeFullFileName(emuGetPropertyValue(m_palWindow->getPlatformObjectName(), "helpFile"));
    HelpDialog::execute(QString::fromUtf8(helpFile.c_str()), false);
}


void MainWindow::onAbout()
{
    AboutDialog* dialog = new AboutDialog(this);
    dialog->execute();
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


void MainWindow::onLoadRamDisk()
{
    emuSysReq(m_palWindow, SR_LOADRAMDISK);
}


void MainWindow::onSaveRamDisk()
{
    emuSysReq(m_palWindow, SR_SAVERAMDISK);
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


void MainWindow::updateActions()
{
    std::string platform = "";
    if (m_palWindow)
        platform = m_palWindow->getPlatformObjectName();
    if (platform == "")
        return;

    platform += ".";
    std::string val;

    val = emuGetPropertyValue(platform + "diskA", "label");
    m_diskAAction->setVisible(val != "");

    val = emuGetPropertyValue(platform + "diskB", "label");
    m_diskBAction->setVisible(val != "");

    bool disksVisible = m_diskAAction->isVisible() || m_diskBAction->isVisible();
    m_menuDiskSeparator->setVisible(disksVisible);
    m_toolbarDiskSeparator->setVisible(disksVisible);

    bool ramDiskPresent = emuGetPropertyValue(platform + "ramDisk", "name") != "";
    m_loadRamDiskAction->setVisible(ramDiskPresent);
    m_saveRamDiskAction->setVisible(ramDiskPresent);
    m_ramDiskSeparator->setVisible(ramDiskPresent);

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

    val = emuGetPropertyValue(platform + "window", "antialiasing");
    if (val == "")
        m_smoothingAction->setVisible(false);
    else {
        m_smoothingAction->setVisible(true);
        m_smoothingAction->setChecked(val == "yes");
    }

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
                m_colorLabel->setText((*it)->text());
                break;
            }
        }
    }

    m_wideScreenAction->setEnabled(emuGetPropertyValue(m_palWindow->getPlatformObjectName() + ".window", "wideScreen") != "custom");
}
