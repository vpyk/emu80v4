/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2023
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

#include <QAction>
#include <QActionGroup>
#include <QMenuBar>

#include "qtMainWindow.h"

#include "../Globals.h"
#include "../Emulation.h"
#include "../Debugger.h"
#include "../DbgCalls.h"


void MainWindow::onDbgRun()
{
    DebugWindow* dbg = emuGetDebugger(m_palWindow);
    if (dbg)
        dbg->sendCmd(DCMD_RUN);
}


void MainWindow::onDbgStep()
{
    DebugWindow* dbg = emuGetDebugger(m_palWindow);
    if (dbg)
        dbg->sendCmd(DCMD_STEP);
}


void MainWindow::onDbgOver()
{
    DebugWindow* dbg = emuGetDebugger(m_palWindow);
    if (dbg)
        dbg->sendCmd(DCMD_OVER);
}


void MainWindow::onDbgHere()
{
    DebugWindow* dbg = emuGetDebugger(m_palWindow);
    if (dbg)
        dbg->sendCmd(DCMD_HERE);
}


void MainWindow::onDbgSkip()
{
    DebugWindow* dbg = emuGetDebugger(m_palWindow);
    if (dbg)
        dbg->sendCmd(DCMD_SKIP);
}


void MainWindow::onDbgSaveMem()
{
    DebugWindow* dbg = emuGetDebugger(m_palWindow);
    if (dbg)
        dbg->sendCmd(DCMD_SAVEMEM);
}


void MainWindow::onDbgBreakpoint()
{
    DebugWindow* dbg = emuGetDebugger(m_palWindow);
    if (dbg)
        dbg->sendCmd(DCMD_BPOINT);
}


void MainWindow::onDbgMnemo()
{
    DebugWindow* dbg = emuGetDebugger(m_palWindow);
    if (dbg)
        dbg->sendCmd(DCMD_MNEMO);
}


void MainWindow::onDbgMini()
{
    DebugWindow* dbg = emuGetDebugger(m_palWindow);
    if (dbg)
        dbg->sendCmd(DCMD_MINI);
}

void MainWindow::onDbgCode()
{
    DebugWindow* dbg = emuGetDebugger(m_palWindow);
    if (dbg)
        dbg->sendCmd(DCMD_CODE);
}


void MainWindow::onDbgData()
{
    DebugWindow* dbg = emuGetDebugger(m_palWindow);
    if (dbg)
        dbg->sendCmd(DCMD_DATA);
}


void MainWindow::onDbgRegs()
{
    DebugWindow* dbg = emuGetDebugger(m_palWindow);
    if (dbg)
        dbg->sendCmd(DCMD_REGS);
}


void MainWindow::onDbgFlags()
{
    DebugWindow* dbg = emuGetDebugger(m_palWindow);
    if (dbg)
        dbg->sendCmd(DCMD_FLAGS);
}


void MainWindow::onDbgAddr()
{
    DebugWindow* dbg = emuGetDebugger(m_palWindow);
    if (dbg)
        dbg->sendCmd(DCMD_ADDR);
}


void MainWindow::onDbgEdit()
{
    DebugWindow* dbg = emuGetDebugger(m_palWindow);
    if (dbg)
        dbg->sendCmd(DCMD_EDIT);
}


void MainWindow::createDebugActions()
{
    bool swapF5F9 = g_emulation->getDebuggerOptions().swapF5F9;

    m_menuBar = menuBar();
    m_menuBar->setContextMenuPolicy(Qt::PreventContextMenu);

    /*m_toolBar = new QToolBar(this);
    m_toolBar->setFloatable(false);
    //m_toolBar->setMovable(false);
    m_toolBar->setContextMenuPolicy(Qt::PreventContextMenu);
    m_toolBar->setIconSize(QSize(16, 16));
    addToolBar(m_toolBar);
    m_toolBar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);*/

    QMenu* fileMenu = m_menuBar->addMenu(tr("File"));

    // Save memory
    QAction* dbgSaveMemAction = new QAction(tr("Save memory..."), this);
    QKeySequence dbgSaveMemKey(Qt::Key_F12);
    dbgSaveMemAction->setShortcut(dbgSaveMemKey);
    addAction(dbgSaveMemAction);
    fileMenu->addAction(dbgSaveMemAction);
    connect(dbgSaveMemAction, SIGNAL(triggered()), this, SLOT(onDbgSaveMem()));

    QMenu* debugMenu = m_menuBar->addMenu(tr("Debug"));

    // Run
    QAction* dbgRunAction = new QAction(tr("Run"), this);
    QKeySequence dbgRunKey(swapF5F9 ? Qt::Key_F5 : Qt::Key_F9);
    dbgRunAction->setShortcut(dbgRunKey);
    addAction(dbgRunAction);
    debugMenu->addAction(dbgRunAction);
    connect(dbgRunAction, SIGNAL(triggered()), this, SLOT(onDbgRun()));

    // Step
    QAction* dbgStepAction = new QAction(tr("Step"), this);
    QKeySequence dbgStepKey(Qt::Key_F7);
    dbgStepAction->setShortcut(dbgStepKey);
    addAction(dbgStepAction);
    debugMenu->addAction(dbgStepAction);
    connect(dbgStepAction, SIGNAL(triggered()), this, SLOT(onDbgStep()));

    // Over
    QAction* dbgOverAction = new QAction(tr("Step over"), this);
    QKeySequence dbgOverKey(Qt::Key_F8);
    dbgOverAction->setShortcut(dbgOverKey);
    addAction(dbgOverAction);
    debugMenu->addAction(dbgOverAction);
    connect(dbgOverAction, SIGNAL(triggered()), this, SLOT(onDbgOver()));

    // Skip
    QAction* dbgSkipAction = new QAction(tr("Skip"), this);
    QKeySequence dbgSkipKey(Qt::Key_U);
    dbgSkipAction->setShortcut(dbgSkipKey);
    addAction(dbgSkipAction);
    debugMenu->addAction(dbgSkipAction);
    connect(dbgSkipAction, SIGNAL(triggered()), this, SLOT(onDbgSkip()));

    // Here
    QAction* dbgHereAction = new QAction(tr("Here"), this);
    QKeySequence dbgHereKey(Qt::Key_F4);
    dbgHereAction->setShortcut(dbgHereKey);
    addAction(dbgHereAction);
    debugMenu->addAction(dbgHereAction);
    connect(dbgHereAction, SIGNAL(triggered()), this, SLOT(onDbgHere()));

    // Breakpoint
    QAction* dbgBreakpointAction = new QAction(tr("Add breakpoint"), this);
    QKeySequence dbgBreakpointKey(swapF5F9 ? Qt::Key_F9 : Qt::Key_F5);
    dbgBreakpointAction->setShortcut(dbgBreakpointKey);
    addAction(dbgBreakpointAction);
    debugMenu->addAction(dbgBreakpointAction);
    connect(dbgBreakpointAction, SIGNAL(triggered()), this, SLOT(onDbgBreakpoint()));

    debugMenu->addSeparator();

    // Mnemo
    QAction* dbgMnemoAction = new QAction(tr("Toggle mnemonics"), this);
    QKeySequence dbgMnemoKey(Qt::Key_Z);
    dbgMnemoAction->setShortcut(dbgMnemoKey);
    addAction(dbgMnemoAction);
    debugMenu->addAction(dbgMnemoAction);
    connect(dbgMnemoAction, SIGNAL(triggered()), this, SLOT(onDbgMnemo()));

    // Mini
    QAction* dbgMiniAction = new QAction(tr("Toggle mini view"), this);
    QKeySequence dbgMiniKey(Qt::Key_M);
    dbgMiniAction->setShortcut(dbgMiniKey);
    addAction(dbgMiniAction);
    debugMenu->addAction(dbgMiniAction);
    connect(dbgMiniAction, SIGNAL(triggered()), this, SLOT(onDbgMini()));

    QMenu* sectionMenu = m_menuBar->addMenu(tr("Section"));

    // Code
    QAction* dbgCodeAction = new QAction(tr("Code (C, Esc)"), this);
    sectionMenu->addAction(dbgCodeAction);
    connect(dbgCodeAction, SIGNAL(triggered()), this, SLOT(onDbgCode()));

    // Data
    QAction* dbgDataAction = new QAction(tr("Data (D)"), this);
    sectionMenu->addAction(dbgDataAction);
    connect(dbgDataAction, SIGNAL(triggered()), this, SLOT(onDbgData()));

    // Regs
    QAction* dbgRegsAction = new QAction(tr("Registers (R)"), this);
    sectionMenu->addAction(dbgRegsAction);
    connect(dbgRegsAction, SIGNAL(triggered()), this, SLOT(onDbgRegs()));

    // Flags
    QAction* dbgFlagsAction = new QAction(tr("Flags (F)"), this);
    sectionMenu->addAction(dbgFlagsAction);
    connect(dbgFlagsAction, SIGNAL(triggered()), this, SLOT(onDbgFlags()));

    sectionMenu->addSeparator();

    // Addr
    QAction* dbgAddrAction = new QAction(tr("Enter address (A)"), this);
    sectionMenu->addAction(dbgAddrAction);
    connect(dbgAddrAction, SIGNAL(triggered()), this, SLOT(onDbgAddr()));

    // Edit
    QAction* dbgEditAction = new QAction(tr("Edit data (F2, Enter)"), this);
    sectionMenu->addAction(dbgEditAction);
    connect(dbgEditAction, SIGNAL(triggered()), this, SLOT(onDbgEdit()));

    QMenu* windowMenu = m_menuBar->addMenu(tr("Window"));

    QActionGroup* windowGroup = new QActionGroup(this);

    // 1x preset
    m_preset1xAction = new QAction(m_1xIcon, "1x", this /*m_menuBar*/);
    QList<QKeySequence> preset1xKeysList;
    ADD_HOTKEY(preset1xKeysList, Qt::Key_1);
    m_preset1xAction->setShortcuts(preset1xKeysList);
    addAction(m_preset1xAction);
    windowMenu->addAction(m_preset1xAction);
    connect(m_preset1xAction, SIGNAL(triggered()), this, SLOT(on1x()));

    // 2x preset
    m_preset2xAction = new QAction(m_2xIcon, "2x", this /*m_menuBar*/);
    QList<QKeySequence> preset2xKeysList;
    ADD_HOTKEY(preset2xKeysList, Qt::Key_2);
    m_preset2xAction->setShortcuts(preset2xKeysList);
    addAction(m_preset2xAction);
    windowMenu->addAction(m_preset2xAction);
    connect(m_preset2xAction, SIGNAL(triggered()), this, SLOT(on2x()));

    // 3x preset
    m_preset3xAction = new QAction(m_3xIcon, "3x", this /*m_menuBar*/);
    QList<QKeySequence> preset3xKeysList;
    ADD_HOTKEY(preset3xKeysList, Qt::Key_3);
    m_preset3xAction->setShortcuts(preset3xKeysList);
    addAction(m_preset3xAction);
    windowMenu->addAction(m_preset3xAction);
    connect(m_preset3xAction, SIGNAL(triggered()), this, SLOT(on3x()));

    // Fit preset
    m_presetFitAction = new QAction(m_resizableIcon, tr("Fit"), this /*m_menuBar*/);
    QList<QKeySequence> presetFitKeysList;
    ADD_HOTKEY(presetFitKeysList, Qt::Key_0);
    m_presetFitAction->setShortcuts(presetFitKeysList);
    addAction(m_presetFitAction);
    windowMenu->addAction(m_presetFitAction);
    connect(m_presetFitAction, SIGNAL(triggered()), this, SLOT(onFit()));

    windowGroup->addAction(m_preset1xAction);
    windowGroup->addAction(m_preset2xAction);
    windowGroup->addAction(m_preset3xAction);
    windowGroup->addAction(m_presetFitAction);

    windowMenu->addSeparator();

    m_fullscreenAction = new QAction(tr("Fullscreen mode"), this /*m_menuBar*/);
    m_fullscreenAction->setCheckable(true);
    QList<QKeySequence> fullscreenKeysList;
    ADD_HOTKEY(fullscreenKeysList, Qt::Key_Return);
    m_fullscreenAction->setShortcuts(fullscreenKeysList);
    connect(m_fullscreenAction, SIGNAL(triggered()), this, SLOT(onFullscreen()));
    addAction(m_fullscreenAction);
    windowMenu->addAction(m_fullscreenAction);

    m_fullwindowAction = new QAction(tr("Hide menu and buttons"), this /*m_menuBar*/);
    m_fullwindowAction->setCheckable(true);
    //setToolTip(tr("Hide menu and buttons (Alt-\)"));
    QList<QKeySequence> fullwindowKeysList;
    ADD_HOTKEY(fullwindowKeysList, Qt::Key_Backslash);
    m_fullwindowAction->setShortcuts(fullwindowKeysList);
    connect(m_fullwindowAction, SIGNAL(triggered()), this, SLOT(onFullwindow()));
    addAction(m_fullwindowAction);
    windowMenu->addAction(m_fullwindowAction);
}
