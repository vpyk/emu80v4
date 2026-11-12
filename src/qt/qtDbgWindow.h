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

#ifndef QTDBGWINDOW_H
#define QTDBGWINDOW_H

#include <cstdint>
#include <vector>

#include <QMainWindow>

#include "qtDbgIcons.h"

class QAction;
class QDockWidget;
class QLabel;
class QMenu;
class QTimer;
class QToolBar;
class Platform;
class DebugSession;
class DbgCodeView;
class DbgRegsView;
class DbgMemPanel;
class DbgStackView;
class DbgBreakpointsPanel;


// The Qt debugger window. It stays open while the emulation runs; the views then
// keep the state of the last stop and are shown as outdated.
//
// The window learns about state changes by polling the debug session (run state,
// stop count, modify count, breakpoint list change count) while it is visible, so
// it sees changes made by other clients (MCP) as well.
class DbgWindow : public QMainWindow
{
    Q_OBJECT

public:
    DbgWindow(Platform* platform);

    void showStop();    // a stop of the UI: show, refresh and activate the window
    void refreshStop(); // a stop of another client or a platform change: refresh if shown

signals:
    // The machine state is to be shown again (a stop or a change by a client).
    // newStop is true for a new stop: views highlight changes since the previous one.
    void stateChanged(bool newStop);
    void breakpointsChanged();
    // The emulation is running (the views are outdated) or stopped
    void runStateChanged(bool running);

protected:
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void onRun();
    void onBreak();
    void onStepInto();
    void onStepOver();
    void onStepOut();
    void onRunToCursor();
    void onSkip();
    void onPoll();
    void onResetTicks();

private:
    Platform* m_platform;
    DebugSession* m_session;

    QAction* m_runAction;
    QAction* m_breakAction;
    QAction* m_stepIntoAction;
    QAction* m_stepOverAction;
    QAction* m_stepOutAction;
    QAction* m_runToCursorAction;
    QAction* m_skipAction;
    QAction* m_breakpointAction;
    QAction* m_setPcAction;
    QAction* m_gotoPcAction;
    QAction* m_gotoAddrAction;
    DbgCodeView* m_codeView;
    DbgRegsView* m_regsView;
    DbgMemPanel* m_memPanel;
    DbgStackView* m_stackView;
    DbgBreakpointsPanel* m_bpPanel;
    QMenu* m_viewMenu;
    QLabel* m_statusLabel;
    QLabel* m_ticksLabel;
    QAction* m_resetTicksAction;
    QTimer* m_pollTimer;

    // the state shown
    bool m_running = true;
    unsigned m_stopCount = 0;
    unsigned m_modifyCount = 0;
    unsigned m_bpChangeCount = 0;
    bool m_swapF5F9 = false;

    // CPU ticks: at the previous stop, at the counter reset, of the last run
    uint64_t m_prevStopTicks = 0;
    uint64_t m_ticksBase = 0;
    uint64_t m_lastRunTicks = 0;
    bool m_hasLastRunTicks = false;
    bool m_ticksStarted = false;

    // floating docks hidden with the window, to be shown with it again
    std::vector<QDockWidget*> m_hiddenFloatingDocks;

    QAction* createAction(const QString& text, const QString& shortText, DbgIconKind icon, void (DbgWindow::*slot)(),
                          QMenu* menu, QToolBar* toolBar);
    void createActions();
    void createDocks();
    void updateShortcuts();
    void checkState(bool forceRefresh);
    void updateRunState();
    uint64_t cpuTicks();
    void updateTicks(bool newStop);
    void activateEmuWindow();
    QString stopText();
};


#endif // QTDBGWINDOW_H
