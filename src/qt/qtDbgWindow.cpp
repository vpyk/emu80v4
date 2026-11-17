/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2026
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

#include <functional>

#include <QAction>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QSettings>
#include <QCloseEvent>
#include <QDockWidget>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>

#include "qtDbgWindow.h"
#include "qtDbgCodeView.h"
#include "qtDbgRegsView.h"
#include "qtDbgMemView.h"
#include "qtDbgBreakpoints.h"
#include "qtDbgStackView.h"
#include "qtDbgDevicesView.h"
#include "qtDbgStyle.h"
#include "qtMainWindow.h"

#include "../Globals.h"
#include "../Emulation.h"
#include "../EmuWindow.h"
#include "../Platform.h"
#include "../DebugSession.h"
#include "../DebugTarget.h"
#include "../Cpu.h"


namespace {

// Increased when the set of docks changes: an older saved layout is not restored then
const int c_layoutVersion = 1;

} // namespace


DbgWindow::DbgWindow(Platform* platform)
    : QMainWindow(nullptr), m_platform(platform), m_session(platform->getDebugSession())
{
    setWindowTitle(tr("Debugger: %1").arg(QString::fromUtf8(platform->getWindow()->getCaption().c_str())));
    // the memory view of 16 bytes per line with the breakpoints table beside the code
    // and the registers, 30 lines of code and 12 lines of memory (the dock size hint)
    DbgMetrics metrics(dbgFont());
    QSize size(metrics.charWidth * 100, metrics.lineHeight * 42 + 120);
    QRect screen = QGuiApplication::primaryScreen()->availableGeometry();
    resize(qMin(size.width(), screen.width() * 9 / 10), qMin(size.height(), screen.height() * 9 / 10));

    m_codeView = new DbgCodeView(m_session, this);
    setCentralWidget(m_codeView);
    m_codeView->setFocus(); // gets the keyboard when the window is activated
    connect(this, &DbgWindow::stateChanged, m_codeView, &DbgCodeView::onStateChanged);
    connect(this, &DbgWindow::breakpointsChanged, m_codeView, &DbgCodeView::onBreakpointsChanged);
    connect(this, &DbgWindow::runStateChanged, m_codeView, &DbgCodeView::onRunStateChanged);

    createActions();
    createDocks();

    m_statusLabel = new QLabel(this);
    statusBar()->addWidget(m_statusLabel, 1);
    m_ticksLabel = new QLabel(this);
    m_ticksLabel->setToolTip(tr("CPU ticks of the last run (since the previous stop) and since the counter reset"));
    statusBar()->addPermanentWidget(m_ticksLabel);
    m_ticksBase = cpuTicks();

    m_defaultState = saveState(c_layoutVersion);
    m_defaultSize = QMainWindow::size();
    restoreLayout();

    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(100);
    connect(m_pollTimer, &QTimer::timeout, this, &DbgWindow::onPoll);

    checkState(true);
}


DbgWindow::~DbgWindow()
{
    if (isVisible())
        saveLayout();
}


// The window geometry and docks, common for all platforms
void DbgWindow::saveLayout()
{
    QSettings settings;
    settings.beginGroup("debugger");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState(c_layoutVersion));
    settings.endGroup();
}


void DbgWindow::restoreLayout()
{
    QSettings settings;
    settings.beginGroup("debugger");
    QByteArray geometry = settings.value("geometry").toByteArray();
    if (!geometry.isEmpty())
        restoreGeometry(geometry);
    QByteArray state = settings.value("state").toByteArray();
    if (!state.isEmpty())
        restoreState(state, c_layoutVersion);
    settings.endGroup();
}


void DbgWindow::onResetLayout()
{
    restoreState(m_defaultState, c_layoutVersion);
    resize(m_defaultSize);
}


QAction* DbgWindow::createAction(const QString& text, const QString& shortText, DbgIconKind icon,
                                 void (DbgWindow::*slot)(),
                                 QMenu* menu, QToolBar* toolBar)
{
    QAction* action = new QAction(dbgIcon(icon), text, this);
    action->setIconText(shortText); // tool bar
    action->setShortcutContext(Qt::WindowShortcut);
    connect(action, &QAction::triggered, this, slot);
    menu->addAction(action);
    toolBar->addAction(action);
    return action;
}


void DbgWindow::createActions()
{
    QMenu* debugMenu = menuBar()->addMenu(tr("Debug"));

    QToolBar* toolBar = addToolBar(tr("Debug"));
    toolBar->setObjectName("debugToolBar");
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolBar->setIconSize(QSize(16, 16));

    m_runAction = createAction(tr("Run"), tr("Run"), DI_RUN, &DbgWindow::onRun, debugMenu, toolBar);
    m_breakAction = createAction(tr("Break"), tr("Break"), DI_BREAK, &DbgWindow::onBreak, debugMenu, toolBar);
    debugMenu->addSeparator();
    toolBar->addSeparator();
    m_stepIntoAction = createAction(tr("Step into"), tr("Into", "tool bar"), DI_STEP_INTO, &DbgWindow::onStepInto, debugMenu, toolBar);
    m_stepOverAction = createAction(tr("Step over"), tr("Over", "tool bar"), DI_STEP_OVER, &DbgWindow::onStepOver, debugMenu, toolBar);
    m_stepOutAction = createAction(tr("Step out"), tr("Out", "tool bar"), DI_STEP_OUT, &DbgWindow::onStepOut, debugMenu, toolBar);
    m_runToCursorAction = createAction(tr("Run to cursor"), tr("To cursor", "tool bar"), DI_RUN_TO_CURSOR, &DbgWindow::onRunToCursor, debugMenu, toolBar);
    m_skipAction = createAction(tr("Skip instruction"), tr("Skip", "tool bar"), DI_SKIP, &DbgWindow::onSkip, debugMenu, toolBar);
    debugMenu->addSeparator();
    m_resetTicksAction = debugMenu->addAction(tr("Reset tick counter"), this, &DbgWindow::onResetTicks);
    debugMenu->addSeparator();
    // F12 as in the classic debug window; also in the context menu of the dumps
    m_saveMemAction = debugMenu->addAction(tr("Save memory..."), this, &DbgWindow::onSaveMemory);
    m_loadMemAction = debugMenu->addAction(tr("Load memory..."), this, &DbgWindow::onLoadMemory);
    m_saveMemAction->setShortcut(QKeySequence("F12"));
    m_loadMemAction->setShortcut(QKeySequence("Shift+F12"));
    m_saveMemAction->setShortcutContext(Qt::WindowShortcut);
    m_loadMemAction->setShortcutContext(Qt::WindowShortcut);

    // Code view actions, also in its context menu
    QMenu* codeMenu = menuBar()->addMenu(tr("Code"));
    auto codeAction = [&](const QString& text, void (DbgCodeView::*slot)()) {
        QAction* action = new QAction(text, this);
        action->setShortcutContext(Qt::WindowShortcut);
        connect(action, &QAction::triggered, m_codeView, slot);
        codeMenu->addAction(action);
        addAction(action);
        return action;
    };
    m_breakpointAction = codeAction(tr("Toggle breakpoint"), &DbgCodeView::toggleBreakpoint);
    m_setPcAction = codeAction(tr("Set PC to cursor"), &DbgCodeView::setPcToCursor);
    codeMenu->addSeparator();
    m_gotoPcAction = codeAction(tr("Go to PC") + "\tHome", &DbgCodeView::gotoPc); // Home is handled by the view
    m_gotoAddrAction = codeAction(tr("Go to address..."), &DbgCodeView::gotoAddress);
    // only in the code view: Ctrl-M is "show value in memory" in the stack view too
    m_showInMemoryAction = new QAction(tr("Show in memory"), this);
    m_showInMemoryAction->setShortcut(QKeySequence("Ctrl+M"));
    m_showInMemoryAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_showInMemoryAction, &QAction::triggered, this, [this]() {showInMemory(m_codeView->getCursorAddr());});
    codeMenu->addAction(m_showInMemoryAction);

    QAction* separator = new QAction(this);
    separator->setSeparator(true);
    m_codeView->addActions({m_breakpointAction, m_runToCursorAction, m_setPcAction, separator,
                            m_gotoPcAction, m_gotoAddrAction, m_showInMemoryAction});

    updateShortcuts();
}


void DbgWindow::createDocks()
{
    m_viewMenu = menuBar()->addMenu(tr("View"));
    QMenu* gotoMenu = m_viewMenu->addMenu(tr("Go to panel"));
    m_viewMenu->addSeparator();

    m_regsView = new DbgRegsView(m_session, this);
    connect(this, &DbgWindow::stateChanged, m_regsView, &DbgRegsView::onStateChanged);
    connect(this, &DbgWindow::runStateChanged, m_regsView, &DbgRegsView::onRunStateChanged);

    QDockWidget* regsDock = new QDockWidget(tr("Registers"), this);
    regsDock->setObjectName("registersDock");
    regsDock->setWidget(m_regsView);
    addDockWidget(Qt::RightDockWidgetArea, regsDock);
    m_viewMenu->addAction(regsDock->toggleViewAction());

    m_stackView = new DbgStackView(m_session, this);
    connect(this, &DbgWindow::stateChanged, m_stackView, &DbgStackView::onStateChanged);
    connect(this, &DbgWindow::runStateChanged, m_stackView, &DbgStackView::onRunStateChanged);
    connect(m_stackView, &DbgStackView::showInCode, this, [this](uint16_t addr) {
        m_codeView->showAddress(addr);
        m_codeView->setFocus();
    });

    QDockWidget* stackDock = new QDockWidget(tr("Stack"), this);
    stackDock->setObjectName("stackDock");
    stackDock->setWidget(m_stackView);
    addDockWidget(Qt::RightDockWidgetArea, stackDock); // under the registers
    m_viewMenu->addAction(stackDock->toggleViewAction());

    // two dumps, the second one is hidden by default
    for (int i = 0; i < 2; i++) {
        DbgMemPanel* panel = new DbgMemPanel(m_session, this);
        connect(this, &DbgWindow::stateChanged, panel, &DbgMemPanel::onStateChanged);
        connect(this, &DbgWindow::runStateChanged, panel, &DbgMemPanel::onRunStateChanged);
        connect(panel, &DbgMemPanel::showInCode, this, [this](uint16_t addr) {
            m_codeView->showAddress(addr);
            m_codeView->setFocus();
        });
        connect(panel, &DbgMemPanel::activated, this, [this, i]() {m_activeMem = i;});

        QDockWidget* dock = new QDockWidget(tr("Memory %1").arg(i + 1), this);
        dock->setObjectName(i == 0 ? "memoryDock" : "memoryDock2");
        dock->setWidget(panel);
        addDockWidget(Qt::BottomDockWidgetArea, dock);
        if (i > 0) {
            tabifyDockWidget(m_memDocks[0], dock);
            dock->hide();
        }
        m_viewMenu->addAction(dock->toggleViewAction());
        m_memPanels[i] = panel;
        m_memDocks[i] = dock;

        QAction* separator = new QAction(this);
        separator->setSeparator(true);
        panel->getView()->addActions({separator, m_saveMemAction, m_loadMemAction});
    }
    QDockWidget* memDock = m_memDocks[0];

    m_bpPanel = new DbgBreakpointsPanel(m_session, this);
    m_bpPanel->setAddressProvider([this]() {return m_codeView->getCursorAddr();});
    connect(this, &DbgWindow::breakpointsChanged, m_bpPanel, &DbgBreakpointsPanel::onBreakpointsChanged);
    connect(this, &DbgWindow::stateChanged, m_bpPanel, &DbgBreakpointsPanel::onStateChanged);
    connect(m_bpPanel, &DbgBreakpointsPanel::showInCode, this, [this](uint16_t addr) {
        m_codeView->showAddress(addr);
        m_codeView->setFocus();
    });
    connect(m_bpPanel, &DbgBreakpointsPanel::showInMemory, this, &DbgWindow::showInMemory);
    connect(m_stackView, &DbgStackView::showInMemory, this, &DbgWindow::showInMemory);

    QDockWidget* bpDock = new QDockWidget(tr("Breakpoints"), this);
    bpDock->setObjectName("breakpointsDock");
    bpDock->setWidget(m_bpPanel);
    addDockWidget(Qt::BottomDockWidgetArea, bpDock);
    tabifyDockWidget(memDock, bpDock);
    m_viewMenu->addAction(bpDock->toggleViewAction());

    DbgDevicesView* devicesView = new DbgDevicesView(m_session, this);
    connect(this, &DbgWindow::stateChanged, devicesView, &DbgDevicesView::onStateChanged);
    connect(this, &DbgWindow::runStateChanged, devicesView, &DbgDevicesView::onRunStateChanged);

    QDockWidget* devicesDock = new QDockWidget(tr("Devices"), this);
    devicesDock->setObjectName("devicesDock");
    devicesDock->setWidget(devicesView);
    addDockWidget(Qt::BottomDockWidgetArea, devicesDock);
    tabifyDockWidget(bpDock, devicesDock);
    m_viewMenu->addAction(devicesDock->toggleViewAction());

    memDock->raise();

    m_viewMenu->addSeparator();
    m_viewMenu->addAction(tr("Reset layout"), this, &DbgWindow::onResetLayout);

    m_panels = {{nullptr, m_codeView}, {regsDock, m_regsView}, {stackDock, m_stackView},
                {m_memDocks[0], m_memPanels[0]->getView()}, {m_memDocks[1], m_memPanels[1]->getView()},
                {bpDock, m_bpPanel}, {devicesDock, devicesView}};
    createPanelActions(gotoMenu);
}


// Keyboard access to the panels, as the letter keys of the classic debug window
// (Alt-D is "Break", so memory is Alt-M)
void DbgWindow::createPanelActions(QMenu* menu)
{
    auto panelAction = [&](const QString& text, const char* key, std::function<void()> slot) {
        QAction* action = new QAction(text, this);
        action->setShortcut(QKeySequence(key));
        action->setShortcutContext(Qt::WindowShortcut);
        connect(action, &QAction::triggered, this, slot);
        menu->addAction(action);
        addAction(action);
        return action;
    };
    QAction* codeAction = panelAction(tr("Code"), "Alt+C", [this]() {activatePanel(0);});
    codeAction->setShortcuts({QKeySequence("Alt+C"), QKeySequence(Qt::Key_Escape)});
    panelAction(tr("Registers"), "Alt+R", [this]() {activatePanel(1);});
    panelAction(tr("Stack"), "Alt+S", [this]() {activatePanel(2);});
    panelAction(tr("Memory"), "Alt+M", [this]() {activateMemoryPanel();});
    panelAction(tr("Breakpoints"), "Alt+B", [this]() {activatePanel(5);});
    panelAction(tr("Devices"), "Alt+V", [this]() {activatePanel(6);});
    menu->addSeparator();
    panelAction(tr("Next panel"), "Ctrl+Tab", [this]() {activateNextPanel(true);});
    panelAction(tr("Previous panel"), "Ctrl+Shift+Tab", [this]() {activateNextPanel(false);});
}


// The panel with the keyboard focus, -1 if none
int DbgWindow::currentPanel()
{
    QWidget* focus = QApplication::focusWidget();
    if (!focus)
        return -1;
    for (int i = 0; i < int(m_panels.size()); i++) {
        const Panel& panel = m_panels[i];
        if (panel.dock ? panel.dock->isAncestorOf(focus) : focus == m_codeView)
            return i;
    }
    return -1;
}


// Not closed (it may be under another tab)
bool DbgWindow::isPanelOpen(int index)
{
    QDockWidget* dock = m_panels[index].dock;
    return !dock || dock->toggleViewAction()->isChecked();
}


// Opens the panel if it's closed, brings its tab to front and gives it the keyboard
void DbgWindow::activatePanel(int index)
{
    if (index == currentPanel())
        return;
    const Panel& panel = m_panels[index];
    if (panel.dock) {
        panel.dock->show();
        panel.dock->raise();
    }
    panel.focus->window()->activateWindow(); // a floating dock is a window of its own
    panel.focus->setFocus(Qt::ShortcutFocusReason);
}


// The open panels in turn
void DbgWindow::activateNextPanel(bool forward)
{
    int n = int(m_panels.size());
    int index = currentPanel();
    if (index < 0)
        index = 0;
    for (int i = 1; i < n; i++) {
        int next = (index + (forward ? i : n - i)) % n;
        if (isPanelOpen(next)) {
            activatePanel(next);
            return;
        }
    }
}


// The last active dump; pressed again in a dump, the other one if it's open
void DbgWindow::activateMemoryPanel()
{
    const int first = 3; // the dumps in m_panels
    int index = currentPanel();
    bool inMemory = index == first || index == first + 1;
    int target = inMemory ? (index == first ? first + 1 : first) : first + m_activeMem;
    if (!isPanelOpen(target))
        target = inMemory ? index : first; // the first dump is opened if closed
    activatePanel(target);
}


// The dialog starts with the memory and the cursor of the last active dump
void DbgWindow::memoryFileDialog(DbgMemFileDialog::Mode mode)
{
    if (m_session->isRunning())
        return;
    DbgMemView* view = m_memPanels[m_activeMem]->getView();
    DbgMemFileDialog dialog(m_session, mode, mode == DbgMemFileDialog::MODE_SAVE ? m_saveMemParams : m_loadMemParams,
                            view->getMemory(), view->getCursorAddr(), this);
    dialog.exec();
}


void DbgWindow::onSaveMemory()
{
    memoryFileDialog(DbgMemFileDialog::MODE_SAVE);
}


void DbgWindow::onLoadMemory()
{
    memoryFileDialog(DbgMemFileDialog::MODE_LOAD);
}


// In the last active dump if it's open, else in the first one
void DbgWindow::showInMemory(uint16_t addr)
{
    int i = m_memDocks[m_activeMem]->toggleViewAction()->isChecked() ? m_activeMem : 0;
    m_memDocks[i]->show();
    m_memDocks[i]->raise();
    m_memPanels[i]->showAddress(addr);
}


// The same keys as in the classic debug window
void DbgWindow::updateShortcuts()
{
    m_swapF5F9 = g_emulation->getDebuggerOptions().swapF5F9;

    m_runAction->setShortcut(QKeySequence(m_swapF5F9 ? "F5" : "F9"));
    m_breakAction->setShortcut(QKeySequence("Alt+D"));
    m_stepIntoAction->setShortcut(QKeySequence("F7"));
    m_stepOverAction->setShortcut(QKeySequence("F8"));
    m_stepOutAction->setShortcut(QKeySequence("F6"));
    m_runToCursorAction->setShortcut(QKeySequence("F4"));
    m_skipAction->setShortcut(QKeySequence("Shift+F8"));
    m_breakpointAction->setShortcut(QKeySequence(m_swapF5F9 ? "F9" : "F5"));
    m_setPcAction->setShortcut(QKeySequence("Ctrl+Shift+F10"));
    m_gotoAddrAction->setShortcut(QKeySequence("Ctrl+G"));

    QAction* actions[] = {m_runAction, m_breakAction, m_stepIntoAction, m_stepOverAction,
                          m_stepOutAction, m_runToCursorAction, m_skipAction};
    for (QAction* action : actions)
        action->setToolTip(action->text() + " (" + action->shortcut().toString(QKeySequence::NativeText) + ")");
}


void DbgWindow::showStop()
{
    if (m_swapF5F9 != g_emulation->getDebuggerOptions().swapF5F9)
        updateShortcuts();
    checkState(false);
    show();
    raise();
    activateWindow();
}


// Memory may have changed (e.g. a file loaded) without a new stop, so the state is shown again
void DbgWindow::refreshStop()
{
    if (isVisible())
        checkState(true);
}


void DbgWindow::onPoll()
{
    checkState(false);
}


// Compares the session state with the state shown and refreshes what has changed
void DbgWindow::checkState(bool forceRefresh)
{
    bool running = m_session->isRunning();
    unsigned stopCount = m_session->getStopCount();
    unsigned modifyCount = m_session->getModifyCount();
    unsigned bpChangeCount = m_session->getBreakpoints()->getChangeCount();

    if (running != m_running || stopCount != m_stopCount || forceRefresh) {
        bool newStop = !running && stopCount != m_stopCount;
        m_running = running;
        m_stopCount = stopCount;
        m_modifyCount = modifyCount;
        updateRunState();
        if (!running) {
            updateTicks(newStop);
            emit stateChanged(newStop);
        }
    } else if (modifyCount != m_modifyCount) {
        m_modifyCount = modifyCount;
        if (!running)
            emit stateChanged(false);
    }

    if (bpChangeCount != m_bpChangeCount || forceRefresh) {
        m_bpChangeCount = bpChangeCount;
        emit breakpointsChanged();
    }
}


void DbgWindow::updateRunState()
{
    QAction* stoppedActions[] = {m_runAction, m_stepIntoAction, m_stepOverAction, m_stepOutAction,
                                 m_runToCursorAction, m_skipAction, m_setPcAction, m_gotoPcAction, m_gotoAddrAction,
                                 m_showInMemoryAction, m_saveMemAction, m_loadMemAction};
    for (QAction* action : stoppedActions)
        action->setEnabled(!m_running);
    m_breakAction->setEnabled(m_running);

    if (m_running)
        m_statusLabel->setText(tr("Running. The data shown is from the last stop"));
    else
        m_statusLabel->setText(stopText());

    emit runStateChanged(m_running);
}


uint64_t DbgWindow::cpuTicks()
{
    Cpu* cpu = m_session->getTarget()->getCpu();
    return cpu->getClock() / cpu->getKDiv();
}


// Ticks of the last run are known from the second stop seen by the window
void DbgWindow::updateTicks(bool newStop)
{
    uint64_t ticks = cpuTicks();
    if (newStop && m_ticksStarted) {
        m_lastRunTicks = ticks - m_prevStopTicks;
        m_hasLastRunTicks = true;
    }
    m_prevStopTicks = ticks;
    m_ticksStarted = true;
    if (m_hasLastRunTicks)
        m_ticksLabel->setText(tr("Ticks: +%1, total: %2").arg(m_lastRunTicks).arg(ticks - m_ticksBase));
    else
        m_ticksLabel->setText(tr("Ticks: total %1").arg(ticks - m_ticksBase));
}


void DbgWindow::onResetTicks()
{
    m_ticksBase = cpuTicks();
    updateTicks(false);
}


QString DbgWindow::stopText()
{
    const DebugStopInfo& info = m_session->getStopInfo();
    QString reason;
    switch (info.reason) {
    case DSR_BREAK:
        reason = tr("break");
        break;
    case DSR_BREAKPOINT:
        reason = tr("breakpoint #%1").arg(info.bpId);
        break;
    case DSR_STEP:
        reason = tr("step");
        break;
    case DSR_TIMER:
        reason = tr("timer");
        break;
    case DSR_HALT:
        reason = tr("HLT");
        break;
    case DSR_ILLEGAL:
        reason = tr("illegal instruction");
        break;
    default:
        return tr("Stopped");
    }
    QString text = tr("Stopped: %1").arg(reason);
    if (info.client == DC_EXTERNAL)
        text += " " + tr("(by MCP)");
    return text;
}


void DbgWindow::activateEmuWindow()
{
    PalWindow* wnd = m_platform->getWindow();
    if (wnd && wnd->getQtWindow()) {
        wnd->getQtWindow()->raise();
        wnd->getQtWindow()->activateWindow();
    }
}


void DbgWindow::onRun()
{
    m_session->run(DC_UI);
    checkState(false);
    activateEmuWindow();
}


void DbgWindow::onBreak()
{
    m_session->requestBreak(DC_UI);
}


void DbgWindow::onStepInto()
{
    m_session->stepInto(DC_UI);
}


void DbgWindow::onStepOver()
{
    if (m_session->stepOver(DC_UI)) {
        // runs until the return address
        checkState(false);
        activateEmuWindow();
    }
}


void DbgWindow::onStepOut()
{
    m_session->stepOut(DC_UI);
    checkState(false);
    activateEmuWindow();
}


void DbgWindow::onRunToCursor()
{
    m_session->runTo(m_codeView->getCursorAddr(), DC_UI);
    checkState(false);
    activateEmuWindow();
}


void DbgWindow::onSkip()
{
    if (m_session->isRunning())
        return;
    CpuDebugTarget* target = m_session->getTarget();
    uint16_t pc = target->getPC();
    m_session->setPC(uint16_t(pc + target->getInstrLength(pc)));
}


// Closing the window while stopped resumes the emulation, as the classic debug window does
void DbgWindow::closeEvent(QCloseEvent* event)
{
    if (!m_session->isRunning()) {
        m_session->run(DC_UI);
        activateEmuWindow();
    }
    event->accept();
}


void DbgWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    for (QDockWidget* dock : m_hiddenFloatingDocks)
        dock->show();
    m_hiddenFloatingDocks.clear();
    checkState(false);
    m_pollTimer->start();
}


// Floating docks are separate windows that stay on the screen when the main window
// is hidden (closed or minimized), so they are hidden with it
void DbgWindow::hideEvent(QHideEvent* event)
{
    m_pollTimer->stop();
    saveLayout(); // with the floating docks still shown
    for (QDockWidget* dock : findChildren<QDockWidget*>())
        if (dock->isFloating() && dock->isVisible()) {
            dock->hide();
            m_hiddenFloatingDocks.push_back(dock);
        }
    QMainWindow::hideEvent(event);
}
