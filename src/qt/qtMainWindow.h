/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2024
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
#include <QMap>

#include "../PalKeys.h"
#include "../EmuTypes.h"

class PalWindow;
class PaintWidget;
class SettingsDialog;


const int LAST_FILES_QTY = 7;
const int LAST_PLATFORMS_QTY = 5;

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
    PalWindow* getPalWindow() {return m_windowType != EWT_UNDEFINED ? m_palWindow : nullptr;}
    void showWindow();
    void hideWindow();
    void mouseClick(int x, int y, PalMouseKey key);
    void mouseDrag(int x, int y);

    void setClientSize(int width, int height);
    void adjustClientSize();
    void setFullScreen(bool fullscreen);

    void updateConfig();
    void updateActions();

    std::string getPlatformObjectName() {return m_platformName;}
    std::string getPlatformGroupName() {return m_platformGroupName;}

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent* evt) override;
    void keyReleaseEvent(QKeyEvent* evt) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onFpsTimer();
    void onQuit();

    void onLoad();
    void onLoadRun();
    void onLoadWav();
    void onReset();
    void onPause();
    void onForwardOn();
    void onFullThrottleOn();
    void onForwardOff();
    void onSpeedUp();
    void onSpeedDown();
    void onSpeedUpFine();
    void onSpeedDownFine();
    void onSpeedNormal();
    void onDebug();
    void onColorMode();
    void onColorSelect();
    void onPreset();
    void on1x();
    void on2x();
    void on3x();
    void on4x();
    void on5x();
    void on15x();
    void on25x();
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
    void onHdd();
    void onUnmountHdd();
    void onReadOnlyHdd();
    void onHddLastFiles();
    void onAutoMountHdd();
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
    void onPaste();
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
    void onEdd();
    void onEddSave();
    void onEddSaveAs();
    void onEddUnassign();
    void onEddAutoLoad();
    void onEddAutoSave();
    void onEddLastFiles();
    void onEdd2();
    void onEdd2Save();
    void onEdd2SaveAs();
    void onEdd2Unassign();
    void onEdd2AutoLoad();
    void onEdd2AutoSave();
    void onEdd2LastFiles();
    void onLoadRunLastFiles();
    void onLoadLastFiles();

    void onDbgRun();
    void onDbgStep();
    void onDbgOver();
    void onDbgHere();
    void onDbgSkip();
    void onDbgBreakpoint();
    void onDbgSaveMem();
    void onDbgMnemo();
    void onDbgMini();

    void onDbgCode();
    void onDbgData();
    void onDbgRegs();
    void onDbgFlags();
    void onDbgAddr();
    void onDbgEdit();

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
    QLabel* m_pasteLabel;
    void createActions();
    void fillPlatformListMenu();
    void tuneMenu();
    void createDebugActions();
    PalKeyCode translateKey(QKeyEvent* evt);
    void saveConfig();
    void savePosition();
    void updateLastFiles();
    void updateLastPlatforms(QString platform);
    void updateMountToolTip(QAction* action, const QString& fileName);

    LastFileList m_loaderLastFiles = LastFileList("loader");
    LastFileList m_fddLastFiles = LastFileList("fdd");
    LastFileList m_hddLastFiles = LastFileList("hdd");
    LastFileList m_eddLastFiles = LastFileList("edd");

    QMap<QString, QString> m_platformNames;
    QTimer m_fpsTimer;
    int m_frameCount = 0;
    int m_fpsTimerCnt = 0;
    uint64_t m_firstFpsCoutnerFrameTime = 0;
    uint64_t m_lastFpsCoutnerFrameTime = 0;
    bool m_controlsCreated = false;
    bool m_fullscreenMode = false;

    std::string m_platformName;
    std::string m_platformGroupName;

    int m_clientWidth = 1;
    int m_clientHeight = 1;

    bool m_showFirstTime = true;
    QPoint m_hiddenWindowPos = {0, 0};

    QPoint m_savedWindowPos = {0, 0};
    QSize m_savedWindowSize = {0, 0};

    SettingsDialog* m_settingsDialog = nullptr;

    QIcon m_diskAOnIcon             = QIcon(":/icons/disk_a.png");
    QIcon m_diskAOffIcon            = QIcon(":/icons/disk_a_off.png");
    QIcon m_diskBOnIcon             = QIcon(":/icons/disk_b.png");
    QIcon m_diskBOffIcon            = QIcon(":/icons/disk_b_off.png");
    QIcon m_diskCOnIcon             = QIcon(":/icons/disk_c.png");
    QIcon m_diskCOffIcon            = QIcon(":/icons/disk_c_off.png");
    QIcon m_diskDOnIcon             = QIcon(":/icons/disk_d.png");
    QIcon m_diskDOffIcon            = QIcon(":/icons/disk_d_off.png");
    QIcon m_hddOnIcon               = QIcon(":/icons/hdd_on.png");
    QIcon m_hddOffIcon              = QIcon(":/icons/hdd.png");
    QIcon m_eddOnIcon               = QIcon(":/icons/edd_on.png");
    QIcon m_eddOffIcon              = QIcon(":/icons/edd.png");
    QIcon m_edd2OnIcon              = QIcon(":/icons/edd2_on.png");
    QIcon m_edd2OffIcon             = QIcon(":/icons/edd2.png");
    QIcon m_smoothingNearestIcon    = QIcon(":/icons/sm_nearest.png");
    QIcon m_smoothingBilinearIcon   = QIcon(":/icons/sm_bilinear.png");
    QIcon m_smoothingSharpIcon      = QIcon(":/icons/sm_sharp.png");
    QIcon m_presetIcon              = QIcon(":/icons/preset.png");
    QIcon m_1xIcon                  = QIcon(":/icons/1x.png");
    QIcon m_2xIcon                  = QIcon(":/icons/2x.png");
    QIcon m_3xIcon                  = QIcon(":/icons/3x.png");
    QIcon m_4xIcon                  = QIcon(":/icons/4x.png");
    QIcon m_5xIcon                  = QIcon(":/icons/5x.png");
    QIcon m_15xIcon                 = QIcon(":/icons/15x.png");
    QIcon m_25xIcon                 = QIcon(":/icons/25x.png");
    QIcon m_resizableIcon           = QIcon(":/icons/resizable.png");

    QMenu* m_loadRunMenu = nullptr;
    QMenu* m_loadMenu = nullptr;
    QMenu* m_colorModeMenu = nullptr;
    QMenu* m_platformListMenu = nullptr;
    QMenu* m_smoothingMenu = nullptr;
    QMenu* m_presetMenu = nullptr;
    QMenu* m_diskAMenu = nullptr;
    QMenu* m_diskBMenu = nullptr;
    QMenu* m_diskCMenu = nullptr;
    QMenu* m_diskDMenu = nullptr;
    QMenu* m_hddMenu = nullptr;
    QMenu* m_eddMenu = nullptr;
    QMenu* m_edd2Menu = nullptr;

    QMenuBar* m_menuBar = nullptr;
    QToolBar* m_toolBar = nullptr;
    QStatusBar* m_statusBar = nullptr;

    QToolButton* m_layoutButton;
    QToolButton* m_presetButton;

    QAction* m_loadAction;
    QAction* m_loadMenuAction;
    QAction* m_loadRunAction;
    QAction* m_loadRunMenuAction;
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
    QAction* m_hddAction;
    QAction* m_hddMenuAction;
    QAction* m_hddUnmountAction;
    QAction* m_hddReadOnlyAction;
    QAction* m_hddAutoMountAction;
    QAction* m_eddAction;
    QAction* m_eddMenuAction;
    QAction* m_eddUnassignAction;
    QAction* m_eddAutoLoadAction;
    QAction* m_eddAutoSaveAction;
    QAction* m_eddSaveAction;
    QAction* m_eddSaveAsAction;
    QAction* m_edd2Action;
    QAction* m_edd2MenuAction;
    QAction* m_edd2UnassignAction;
    QAction* m_edd2AutoLoadAction;
    QAction* m_edd2AutoSaveAction;
    QAction* m_edd2SaveAction;
    QAction* m_edd2SaveAsAction;
    QAction* m_menuDiskSeparator;
    QAction* m_menuHddSeparator;
    QAction* m_toolbarDiskSeparator;
    QAction* m_menuEddSeparator;
    QAction* m_exitAction;
    QAction* m_platformSelectAction;
    QAction* m_platformConfigAction;
    QAction* m_resetAction;
    QAction* m_pauseAction;
    QAction* m_forwardAction;
    QAction* m_speedUpAction;
    QAction* m_speedDownAction;
    QAction* m_speedUpFineAction;
    QAction* m_speedDownFineAction;
    QAction* m_speedNormalAction;
    QAction* m_debugAction;
    QAction* m_screenshotAction;
    QAction* m_copyImageAction;
    QAction* m_copyTextAction;
    QAction* m_pasteAction;
    QAction* m_settingsAction;
    QAction* m_tapeHookAction;
    QAction* m_muteTapeAction;
    QAction* m_colorMenuAction;
    QAction* m_colorAction;
    QAction* m_colorMonoOrigAction;
    QAction* m_colorMonoAction;
    QAction* m_colorGrayscaleAction;
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
    QAction* m_presetAction;
    QAction* m_preset1xAction;
    QAction* m_preset2xAction;
    QAction* m_preset3xAction;
    QAction* m_preset4xAction;
    QAction* m_preset5xAction;
    QAction* m_preset15xAction;
    QAction* m_preset25xAction;
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

    QAction* m_loadLastFilesActions[LAST_FILES_QTY];
    QAction* m_loadRunLastFilesActions[LAST_FILES_QTY];
    QAction* m_fddALastFilesActions[LAST_FILES_QTY];
    QAction* m_fddBLastFilesActions[LAST_FILES_QTY];
    QAction* m_fddCLastFilesActions[LAST_FILES_QTY];
    QAction* m_fddDLastFilesActions[LAST_FILES_QTY];
    QAction* m_hddLastFilesActions[LAST_FILES_QTY];
    QAction* m_eddLastFilesActions[LAST_FILES_QTY];
    QAction* m_edd2LastFilesActions[LAST_FILES_QTY];

    QAction* m_lastPlatformsActions[LAST_PLATFORMS_QTY];
};


#ifndef __APPLE__
    #define ADD_HOTKEY(keyList, key) \
    keyList.append(QKeySequence(Qt::ALT | key)); \
    keyList.append(QKeySequence(Qt::META | key))
#else
    #define ADD_HOTKEY(keyList, key) \
    keyList.append(QKeySequence(Qt::ALT | key))
#endif

#endif // QTMAINWINDOW_H
