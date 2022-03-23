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

#ifndef QTMAINWINDOW_H
#define QTMAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QToolButton>

#include "../PalKeys.h"
#include "../EmuTypes.h"

class PalWindow;
class PaintWidget;
class SettingsDialog;


const int LAST_FILES_QTY = 7;

class LastFileList
{
public:
    LastFileList(const QString& type) : m_type(type) {}

    void setPlatformName(const QString& name);
    void addToLastFiles(const QString& fileName);
    int getSize() {return m_list.size();}
    QString getLastFile();
    void tuneActions(QAction** actions);

private:
    QString m_platform;
    QString m_type;
    QList<QString> m_list;
    bool m_loaded = false;

    QString getKeyPrefix();
    void loadLastFiles();
    void saveLastFiles();
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    //inline RenderHelper* getHelper() {return m_helper;}
    inline PaintWidget* getPaintWidget() {return m_paintWidget;}

    void incFrameCount();

    void setPalWindow(PalWindow* palWindow);
    PalWindow* getPalWindow() {return m_palWindow;}
    void showWindow();
    void mouseClick(int x, int y, PalMouseKey key);

    void setClientSize(int width, int height);
    void adjustClientSize();
    void setFullScreen(bool fullscreen);

    void updateConfig();
    void updateActions();

    std::string getPlatformObjectName();
    std::string getPlatformGroupName();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent* evt) override;
    void keyReleaseEvent(QKeyEvent* evt) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onFpsTimer();

    void onLoad();
    void onLoadRun();
    void onLoadWav();
    void onReset();
    void onPause();
    void onForwardOn();
    void onForwardOff();
    void onDebug();
    void onColorMode();
    void onColorSelect();
    void on1x();
    void on2x();
    void on3x();
    void on4x();
    void on5x();
    void on2x3();
    void on3x5();
    void on4x6();
    void onStretch();
    void onFit();
    void onFullscreen();
    void onFullwindow();
    void onExit();
    void onDiskA();
    void onUnmountDiskA();
    void onReadOnlyDiskA();
    void onDiskALastFiles();
    void onAutoMountDiskA();
    void onDiskB();
    void onUnmountDiskB();
    void onReadOnlyDiskB();
    void onDiskBLastFiles();
    void onAutoMountDiskB();
    void onDiskC();
    void onUnmountDiskC();
    void onReadOnlyDiskC();
    void onDiskCLastFiles();
    void onAutoMountDiskC();
    void onDiskD();
    void onUnmountDiskD();
    void onReadOnlyDiskD();
    void onDiskDLastFiles();
    void onAutoMountDiskD();
    void onCrop();
    void onAspect();
    void onWideScreen();
    void onFont();
    void onSmoothing();
    void onSmoothingSelect();
    void onPlatform();
    void onPlatformConfig();
    void onPlatformSelect();
    void onTapeHook();
    void onPrinterCapture();
    void onMuteTape();
    void onScreenshot();
    void onCopyImage();
    void onCopyText();
    void onSettings();
    void onPlatformHelp();
    void onAbout();
    void onQwerty();
    void onJcuken();
    void onSmart();
    void onResetPlatform();
    void onResetAll();
    void onMute();
    void onFastReset();
    void onLoadRamDisk();
    void onSaveRamDisk();

private:
    PaintWidget* m_paintWidget;
    PalWindow* m_palWindow = nullptr;
    EmuWindowType m_windowType;
    QLabel* m_fpsLabel;
    QLabel* m_speedLabel;
    QLabel* m_crtModeLabel;
    QLabel* m_dmaTimeLabel;
    QLabel* m_imageSizeLabel;
    QLabel* m_kbdLabel;
    QLabel* m_colorLabel;
    QLabel* m_tapeLabel;
    QLabel* m_wavLabel;
    QLabel* m_prnLabel;
    void createActions();
    void fillPlatformListMenu();
    void tuneMenu();
    void createDebugActions();
    PalKeyCode translateKey(QKeyEvent* evt);
    void saveConfig();
    void updateLastFiles();

    LastFileList m_fddLastFiles = LastFileList("fdd");

    QTimer m_fpsTimer;
    int m_frameCount = 0;
    uint64_t m_firstFpsCoutnerFrameTime = 0;
    uint64_t m_lastFpsCoutnerFrameTime = 0;
    bool m_controlsCreated = false;
    bool m_fullscreenMode = false;
    bool m_sizable = true;

    int m_clientWidth = 1;
    int m_clientHeight = 1;

    bool m_showFirstTime = true;

    SettingsDialog* m_settingsDialog = nullptr;

    QIcon m_diskAOnIcon             = QIcon(":/icons/disk_a.png");
    QIcon m_diskAOffIcon            = QIcon(":/icons/disk_a_off.png");
    QIcon m_diskBOnIcon             = QIcon(":/icons/disk_b.png");
    QIcon m_diskBOffIcon            = QIcon(":/icons/disk_b_off.png");
    QIcon m_diskCOnIcon             = QIcon(":/icons/disk_c.png");
    QIcon m_diskCOffIcon            = QIcon(":/icons/disk_c_off.png");
    QIcon m_diskDOnIcon             = QIcon(":/icons/disk_d.png");
    QIcon m_diskDOffIcon            = QIcon(":/icons/disk_d_off.png");
    QIcon m_smoothingNearestIcon    = QIcon(":/icons/sm_nearest.png");
    QIcon m_smoothingBilinearIcon   = QIcon(":/icons/sm_bilinear.png");
    QIcon m_smoothingSharpIcon      = QIcon(":/icons/sm_sharp.png");

    QMenu* m_colorModeMenu = nullptr;
    QMenu* m_platformListMenu = nullptr;
    QMenu* m_smoothingMenu = nullptr;
    QMenu* m_diskAMenu = nullptr;
    QMenu* m_diskBMenu = nullptr;
    QMenu* m_diskCMenu = nullptr;
    QMenu* m_diskDMenu = nullptr;

    QMenuBar* m_menuBar = nullptr;
    QToolBar* m_toolBar = nullptr;
    QStatusBar* m_statusBar = nullptr;

    QToolButton* m_layoutButton;
    QToolButton* m_presetButton;

    QAction* m_loadAction;
    QAction* m_loadRunAction;
    QAction* m_loadWavAction;
    QAction* m_diskAAction;
    QAction* m_diskAMenuAction;
    QAction* m_diskAUnmountAction;
    QAction* m_diskAReadOnlyAction;
    QAction* m_diskAAutoMountAction;
    QAction* m_diskBAction;
    QAction* m_diskBMenuAction;
    QAction* m_diskBUnmountAction;
    QAction* m_diskBReadOnlyAction;
    QAction* m_diskBAutoMountAction;
    QAction* m_diskCAction;
    QAction* m_diskCMenuAction;
    QAction* m_diskCUnmountAction;
    QAction* m_diskCReadOnlyAction;
    QAction* m_diskCAutoMountAction;
    QAction* m_diskDAction;
    QAction* m_diskDMenuAction;
    QAction* m_diskDUnmountAction;
    QAction* m_diskDReadOnlyAction;
    QAction* m_diskDAutoMountAction;
    QAction* m_loadRamDiskAction;
    QAction* m_saveRamDiskAction;
    QAction* m_menuDiskSeparator;
    QAction* m_toolbarDiskSeparator;
    QAction* m_ramDiskSeparator;
    QAction* m_exitAction;
    QAction* m_platformSelectAction;
    QAction* m_platformConfigAction;
    QAction* m_resetAction;
    QAction* m_pauseAction;
    QAction* m_forwardAction;
    QAction* m_debugAction;
    QAction* m_screenshotAction;
    QAction* m_copyImageAction;
    QAction* m_copyTextAction;
    QAction* m_settingsAction;
    QAction* m_tapeHookAction;
    QAction* m_muteTapeAction;
    QAction* m_colorMenuAction;
    QAction* m_colorAction;
    QAction* m_colorMonoOrigAction;
    QAction* m_colorMonoAction;
    QAction* m_colorColor1Action;
    QAction* m_colorColor2Action;
    QAction* m_bwAction;
    QAction* m_cropAction;
    QAction* m_aspectAction;
    QAction* m_wideScreenAction;
    QAction* m_fontAction;
    QAction* m_fullscreenAction;
    QAction* m_fullwindowAction;
    QAction* m_smoothingAction;
    QAction* m_smoothingNearestAction;
    QAction* m_smoothingBilinearAction;
    QAction* m_smoothingSharpAction;
    QAction* m_preset1xAction;
    QAction* m_preset2xAction;
    QAction* m_preset3xAction;
    QAction* m_preset4xAction;
    QAction* m_preset5xAction;
    QAction* m_preset2x3Action;
    QAction* m_preset3x5Action;
    QAction* m_preset4x6Action;
    QAction* m_presetStretchAction;
    QAction* m_presetFitAction;
    QAction* m_platformHelpAction;
    QAction* m_aboutAction;
    QAction* m_qwertyAction;
    QAction* m_jcukenAction;
    QAction* m_smartAction;
    QAction* m_autosaveAction;
    QAction* m_muteAction;
    QAction* m_fastResetAction;
    QAction* m_printerCaptureAction;

    QAction* m_fddALastFilesActions[LAST_FILES_QTY];
    QAction* m_fddBLastFilesActions[LAST_FILES_QTY];
    QAction* m_fddCLastFilesActions[LAST_FILES_QTY];
    QAction* m_fddDLastFilesActions[LAST_FILES_QTY];
};


#endif // QTMAINWINDOW_H
