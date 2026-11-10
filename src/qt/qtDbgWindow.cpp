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

#include <QAction>
#include <QCloseEvent>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>

#include "qtDbgWindow.h"
#include "qtMainWindow.h"

#include "../Globals.h"
#include "../Emulation.h"
#include "../EmuWindow.h"
#include "../Platform.h"
#include "../DebugSession.h"


DbgWindow::DbgWindow(Platform* platform)
    : QMainWindow(nullptr), m_platform(platform), m_session(platform->getDebugSession())
{
    setWindowTitle(tr("Debugger: %1").arg(QString::fromUtf8(platform->getWindow()->getCaption().c_str())));
    resize(900, 640);

    createActions();

    m_statusLabel = new QLabel(this);
    statusBar()->addWidget(m_statusLabel, 1);

    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(100);
    connect(m_pollTimer, &QTimer::timeout, this, &DbgWindow::onPoll);

    checkState(true);
}


QAction* DbgWindow::createAction(const QString& text, const QString& shortText, void (DbgWindow::*slot)(),
                                 QMenu* menu, QToolBar* toolBar)
{
    QAction* action = new QAction(text, this);
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
    toolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    m_runAction = createAction(tr("Run"), tr("Run"), &DbgWindow::onRun, debugMenu, toolBar);
    m_breakAction = createAction(tr("Break"), tr("Break"), &DbgWindow::onBreak, debugMenu, toolBar);
    debugMenu->addSeparator();
    toolBar->addSeparator();
    m_stepIntoAction = createAction(tr("Step into"), tr("Into", "tool bar"), &DbgWindow::onStepInto, debugMenu, toolBar);
    m_stepOverAction = createAction(tr("Step over"), tr("Over", "tool bar"), &DbgWindow::onStepOver, debugMenu, toolBar);
    m_stepOutAction = createAction(tr("Step out"), tr("Out", "tool bar"), &DbgWindow::onStepOut, debugMenu, toolBar);
    m_runToCursorAction = createAction(tr("Run to cursor"), tr("To cursor", "tool bar"), &DbgWindow::onRunToCursor, debugMenu, toolBar);
    m_skipAction = createAction(tr("Skip instruction"), tr("Skip", "tool bar"), &DbgWindow::onSkip, debugMenu, toolBar);

    updateShortcuts();
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


void DbgWindow::refreshStop()
{
    if (isVisible())
        checkState(false);
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
        if (!running)
            emit stateChanged(newStop);
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
    QAction* stoppedActions[] = {m_runAction, m_stepIntoAction, m_stepOverAction, m_stepOutAction, m_skipAction};
    for (QAction* action : stoppedActions)
        action->setEnabled(!m_running);
    m_runToCursorAction->setEnabled(false); // needs the code view
    m_breakAction->setEnabled(m_running);

    if (m_running)
        m_statusLabel->setText(tr("Running. The data shown is from the last stop"));
    else
        m_statusLabel->setText(stopText());

    emit runStateChanged(m_running);
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
    // the cursor address comes from the code view (next stage)
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
    checkState(false);
    m_pollTimer->start();
}


void DbgWindow::hideEvent(QHideEvent* event)
{
    m_pollTimer->stop();
    QMainWindow::hideEvent(event);
}
