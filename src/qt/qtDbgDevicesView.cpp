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

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>

#include "qtDbgDevicesView.h"
#include "qtDbgStyle.h"

#include "../DebugSession.h"
#include "../Platform.h"


DbgDevicesView::DbgDevicesView(DebugSession* session, QWidget* parent)
    : QScrollArea(parent), m_session(session)
{
    setWidgetResizable(true);

    m_content = new QWidget;
    m_content->setAutoFillBackground(true);
    m_layout = new QHBoxLayout(m_content);
    int cw = DbgMetrics(dbgFont()).charWidth;
    m_layout->setSpacing(cw * 3);
    m_layout->setContentsMargins(cw, cw / 2, cw, cw / 2);

    m_emptyLabel = new QLabel(tr("No device information"), m_content);
    m_emptyLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_layout->addWidget(m_emptyLabel);
    m_layout->addStretch(1);

    setWidget(m_content);
    render();
}


void DbgDevicesView::onStateChanged(bool newStop)
{
    m_running = m_session->isRunning();
    std::vector<DebugDeviceInfo> infos = m_session->getDevicesInfo();

    // the set of devices is fixed for a platform, but rebuild the columns if it differs
    bool same = infos.size() == m_devices.size();
    for (unsigned i = 0; same && i < infos.size(); i++)
        same = infos[i].name == m_devices[i].name;
    if (!same) {
        for (Device& dev : m_devices)
            delete dev.label;
        m_devices.clear();
        for (const DebugDeviceInfo& info : infos) {
            Device dev;
            dev.name = info.name;
            dev.label = new QLabel(m_content);
            dev.label->setFont(dbgFont());
            dev.label->setTextFormat(Qt::RichText);
            dev.label->setTextInteractionFlags(Qt::TextSelectableByMouse);
            dev.label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            m_layout->insertWidget(m_layout->count() - 1, dev.label); // before the stretch
            m_devices.push_back(dev);
        }
    }

    for (unsigned i = 0; i < infos.size(); i++) {
        Device& dev = m_devices[i];
        QStringList lines = QString::fromUtf8(infos[i].info.c_str()).split('\n');
        while (!lines.isEmpty() && lines.last().isEmpty())
            lines.removeLast();
        if (newStop && m_hasState)
            dev.prevLines = dev.lines;
        else if (!m_hasState)
            dev.prevLines = lines;
        dev.lines = lines;
    }
    m_hasState = true;
    render();
}


void DbgDevicesView::onRunStateChanged(bool running)
{
    m_running = running;
    render();
}


void DbgDevicesView::render()
{
    DbgColors colors(palette());
    bool stale = m_running;

    QPalette pal = m_content->palette();
    pal.setColor(QPalette::Window, colors.bg(stale));
    pal.setColor(QPalette::WindowText, colors.fg(stale));
    m_content->setPalette(pal);

    m_emptyLabel->setVisible(m_hasState && m_devices.empty());

    // object names without the platform prefix ("korvet.pic" -> "pic")
    std::string prefix = m_session->getPlatform()->getName() + ".";

    for (Device& dev : m_devices) {
        std::string name = dev.name.compare(0, prefix.size(), prefix) == 0 ? dev.name.substr(prefix.size()) : dev.name;
        QString html = QString("<pre style=\"margin:0\"><span style=\"color:%1\">%2</span>\n")
                       .arg(colors.dimFg(stale).name(), QString::fromUtf8(name.c_str()).toHtmlEscaped());
        for (int i = 0; i < dev.lines.size(); i++) {
            bool changed = i >= dev.prevLines.size() || dev.prevLines[i] != dev.lines[i];
            QColor color = changed && !stale ? colors.changedText : colors.fg(stale);
            html += QString("<span style=\"color:%1\">%2</span>\n").arg(color.name(), dev.lines[i].toHtmlEscaped());
        }
        html += "</pre>";
        dev.label->setText(html);
    }
}


// the theme has changed
void DbgDevicesView::changeEvent(QEvent* event)
{
    QScrollArea::changeEvent(event);
    if (event->type() == QEvent::PaletteChange)
        render();
}
