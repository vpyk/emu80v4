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

#ifndef QTDBGDEVICESVIEW_H
#define QTDBGDEVICESVIEW_H

#include <string>
#include <vector>

#include <QScrollArea>
#include <QStringList>

class QHBoxLayout;
class QLabel;
class DebugSession;


// Debug information of the platform devices (getDebugInfo() of the objects): a column
// per device with its object name and text. The texts are narrow (made for the classic
// debug window), so the columns go side by side. Lines changed since the previous
// stop are highlighted.
//
// The information is read only while the emulation is stopped; while it runs the view
// keeps the data of the last stop and shows it as outdated.
class DbgDevicesView : public QScrollArea
{
    Q_OBJECT

public:
    DbgDevicesView(DebugSession* session, QWidget* parent = nullptr);

public slots:
    void onStateChanged(bool newStop);
    void onRunStateChanged(bool running);

protected:
    void changeEvent(QEvent* event) override;

private:
    struct Device {
        std::string name;
        QLabel* label;
        QStringList lines;
        QStringList prevLines;
    };

    DebugSession* m_session;
    bool m_running = true;
    bool m_hasState = false;

    QWidget* m_content;
    QHBoxLayout* m_layout;
    QLabel* m_emptyLabel;
    std::vector<Device> m_devices;

    void render();
};


#endif // QTDBGDEVICESVIEW_H
