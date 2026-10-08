/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2022
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

#ifndef RENDERHELPER_H
#define RENDERHELPER_H

#include <list>

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QMutex>

class PaintWidget;
class MainWindow;

class RenderHelper : public QObject
{
    Q_OBJECT
public:
    explicit RenderHelper(QWidget *parent = 0);
    virtual ~RenderHelper();
    void setPaintWidget(PaintWidget* widget);
    void start();
    void pause();
    void resume();
    void addWindow(MainWindow* window);
    void removeWindow(MainWindow* window);
    void updateConfig();

private:
    QTimer* m_timer;
    PaintWidget* m_widget = nullptr;
    std::list<MainWindow*> m_windowList;
    bool m_paused = false;

private slots:
    void onTimer();
};

extern RenderHelper* g_renderHelper;

#endif // RENDERHELPER_H
