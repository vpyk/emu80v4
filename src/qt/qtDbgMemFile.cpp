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

#include <vector>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>

#include "qtDbgMemFile.h"
#include "qtDbgMemView.h"
#include "qtDbgStyle.h"

#include "../DebugSession.h"


namespace {

QString hex(unsigned value, int digits)
{
    return QString("%1").arg(value, digits, 16, QChar('0')).toUpper();
}

// The directory of the last memory file, common for all platforms
const char* c_dirKey = "debugger/memoryFileDir";

} // namespace


DbgMemFileDialog::DbgMemFileDialog(DebugSession* session, Mode mode, DbgMemFileParams& params,
                                   int memory, unsigned addr, QWidget* parent)
    : QDialog(parent), m_session(session), m_mode(mode), m_params(params)
{
    setWindowTitle(mode == MODE_SAVE ? tr("Save memory") : tr("Load memory"));

    int memCount = int(session->getMemories().size());
    if (!m_params.valid || m_params.memory >= memCount) {
        m_params.memory = memory < memCount ? memory : 0;
        unsigned size = memorySize();
        m_params.start = mode == MODE_SAVE ? 0 : (addr < size ? addr : 0);
        m_params.end = size - 1;
    }

    QFont font = dbgFont();
    int cw = DbgMetrics(font).charWidth;

    m_memoryCombo = new QComboBox(this);
    m_memoryCombo->addItems(dbgMemoryNames(session));
    m_memoryCombo->setCurrentIndex(m_params.memory);

    auto addrEdit = [&](unsigned value) {
        QLineEdit* edit = new QLineEdit(this);
        edit->setFont(font);
        edit->setMaxLength(8);
        edit->setFixedWidth(cw * 10 + 6);
        edit->setText(hex(value, addrDigits()));
        connect(edit, &QLineEdit::textChanged, this, &DbgMemFileDialog::updateState);
        return edit;
    };
    m_startEdit = addrEdit(m_params.start);

    QHBoxLayout* rangeLayout = new QHBoxLayout;
    rangeLayout->addWidget(m_startEdit);
    if (mode == MODE_SAVE) {
        m_endEdit = addrEdit(m_params.end);
        QPushButton* wholeButton = new QPushButton(tr("Whole memory"), this);
        wholeButton->setAutoDefault(false);
        connect(wholeButton, &QPushButton::clicked, this, &DbgMemFileDialog::onWholeMemory);
        rangeLayout->addWidget(new QLabel(tr("to"), this));
        rangeLayout->addWidget(m_endEdit);
        rangeLayout->addWidget(wholeButton);
    }
    rangeLayout->addStretch(1);

    m_fileEdit = new QLineEdit(m_params.fileName, this);
    m_fileEdit->setMinimumWidth(cw * 40);
    connect(m_fileEdit, &QLineEdit::textChanged, this, &DbgMemFileDialog::updateState);
    QPushButton* browseButton = new QPushButton("...", this);
    browseButton->setAutoDefault(false);
    browseButton->setToolTip(tr("Choose a file"));
    connect(browseButton, &QPushButton::clicked, this, &DbgMemFileDialog::onBrowse);
    QHBoxLayout* fileLayout = new QHBoxLayout;
    fileLayout->addWidget(m_fileEdit, 1);
    fileLayout->addWidget(browseButton);

    m_infoLabel = new QLabel(this);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttons->button(QDialogButtonBox::Ok);
    m_okButton->setText(mode == MODE_SAVE ? tr("Save") : tr("Load"));
    connect(buttons, &QDialogButtonBox::accepted, this, &DbgMemFileDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QFormLayout* layout = new QFormLayout(this);
    layout->addRow(tr("Memory:"), m_memoryCombo);
    layout->addRow(mode == MODE_SAVE ? tr("Addresses (hex):") : tr("Address (hex):"), rangeLayout);
    layout->addRow(tr("File:"), fileLayout);
    layout->addRow(QString(), m_infoLabel);
    layout->addRow(buttons);

    connect(m_memoryCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &DbgMemFileDialog::onMemoryChanged);

    updateState();
    if (m_fileEdit->text().isEmpty())
        m_fileEdit->setFocus();
    else {
        m_startEdit->setFocus();
        m_startEdit->selectAll();
    }
}


unsigned DbgMemFileDialog::memorySize()
{
    return m_session->getMemories()[m_memoryCombo ? m_memoryCombo->currentIndex() : m_params.memory].size;
}


int DbgMemFileDialog::addrDigits()
{
    return memorySize() > 0x10000 ? 5 : 4;
}


// Another memory: the range is reset to the whole memory (save) or to its start (load)
void DbgMemFileDialog::onMemoryChanged()
{
    m_startEdit->setText(hex(0, addrDigits()));
    if (m_endEdit)
        m_endEdit->setText(hex(memorySize() - 1, addrDigits()));
    updateState();
}


void DbgMemFileDialog::onWholeMemory()
{
    m_startEdit->setText(hex(0, addrDigits()));
    m_endEdit->setText(hex(memorySize() - 1, addrDigits()));
}


void DbgMemFileDialog::onBrowse()
{
    QSettings settings;
    QString dir = m_fileEdit->text().trimmed();
    if (dir.isEmpty())
        dir = settings.value(c_dirKey).toString();
    QString filter = tr("Binary files (*.bin);;All files (*)");
    QString fileName = m_mode == MODE_SAVE ?
                       QFileDialog::getSaveFileName(this, windowTitle(), dir, filter) :
                       QFileDialog::getOpenFileName(this, windowTitle(), dir, filter);
    if (fileName.isEmpty())
        return;
    settings.setValue(c_dirKey, QFileInfo(fileName).absolutePath());
    m_fileEdit->setText(QDir::toNativeSeparators(fileName));
    m_confirmedFile = m_fileEdit->text();
}


bool DbgMemFileDialog::getAddr(QLineEdit* edit, unsigned& addr)
{
    bool ok;
    addr = edit->text().trimmed().toUInt(&ok, 16);
    return ok && addr < memorySize();
}


bool DbgMemFileDialog::check(QString& info, bool& error)
{
    error = true;
    unsigned size = memorySize();
    unsigned start, end;
    QString range = tr("The address must be from %1 to %2").arg(hex(0, addrDigits()), hex(size - 1, addrDigits()));
    if (!getAddr(m_startEdit, start)) {
        info = range;
        return false;
    }

    if (m_mode == MODE_SAVE) {
        if (!getAddr(m_endEdit, end)) {
            info = range;
            return false;
        }
        if (end < start) {
            info = tr("The end address is less than the start one");
            return false;
        }
        info = tr("%1 bytes").arg(end - start + 1);
        error = false; // a file may be not chosen yet
        return !m_fileEdit->text().trimmed().isEmpty();
    }

    QString fileName = m_fileEdit->text().trimmed();
    if (fileName.isEmpty()) {
        info.clear();
        error = false;
        return false;
    }
    QFileInfo fileInfo(fileName);
    if (!fileInfo.isFile()) {
        info = tr("The file is not found");
        return false;
    }
    qint64 fileSize = fileInfo.size();
    if (fileSize == 0) {
        info = tr("The file is empty");
        return false;
    }
    if (fileSize > qint64(size - start)) {
        info = tr("The file (%1 bytes) doesn't fit in memory from this address").arg(fileSize);
        return false;
    }
    info = tr("%1 bytes to %2-%3").arg(fileSize)
           .arg(hex(start, addrDigits()), hex(start + unsigned(fileSize) - 1, addrDigits()));
    error = false;
    return true;
}


void DbgMemFileDialog::updateState()
{
    QString info;
    bool error;
    m_okButton->setEnabled(check(info, error));
    QPalette pal = palette();
    if (error)
        pal.setColor(QPalette::WindowText, DbgColors(palette()).changedText);
    m_infoLabel->setPalette(pal);
    m_infoLabel->setText(info);
}


void DbgMemFileDialog::accept()
{
    QString info;
    bool error;
    if (!check(info, error) || m_session->isRunning())
        return;
    getAddr(m_startEdit, m_params.start);
    if (m_endEdit)
        getAddr(m_endEdit, m_params.end);
    m_params.memory = m_memoryCombo->currentIndex();
    m_params.fileName = m_fileEdit->text().trimmed();
    m_params.valid = true;

    if (m_mode == MODE_SAVE ? save() : load())
        QDialog::accept();
}


bool DbgMemFileDialog::save()
{
    const QString& fileName = m_params.fileName;
    if (fileName != m_confirmedFile && QFileInfo(fileName).exists() &&
            QMessageBox::question(this, windowTitle(), tr("The file %1 exists. Overwrite it?").arg(fileName))
            != QMessageBox::Yes)
        return false;

    std::vector<char> data;
    data.reserve(m_params.end - m_params.start + 1);
    for (unsigned addr = m_params.start; addr <= m_params.end; addr++)
        data.push_back(char(m_session->readMem(m_params.memory, addr)));

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly) || file.write(data.data(), qint64(data.size())) != qint64(data.size())) {
        QMessageBox::warning(this, windowTitle(), tr("Can't write the file %1").arg(fileName));
        return false;
    }
    QSettings().setValue(c_dirKey, QFileInfo(fileName).absolutePath());
    return true;
}


bool DbgMemFileDialog::load()
{
    const QString& fileName = m_params.fileName;
    QFile file(fileName);
    QByteArray data;
    if (file.open(QIODevice::ReadOnly))
        data = file.readAll();
    if (data.isEmpty() || unsigned(data.size()) > memorySize() - m_params.start) {
        QMessageBox::warning(this, windowTitle(), tr("Can't read the file %1").arg(fileName));
        return false;
    }
    int unchanged = 0;
    for (int i = 0; i < data.size(); i++) {
        m_session->writeMem(m_params.memory, m_params.start + i, uint8_t(data[i]));
        if (m_session->readMem(m_params.memory, m_params.start + i) != uint8_t(data[i]))
            unchanged++;
    }
    QSettings().setValue(c_dirKey, QFileInfo(fileName).absolutePath());
    if (unchanged) // ROM or a device that doesn't keep the written values
        QMessageBox::warning(this, windowTitle(),
                             tr("%1 of %2 bytes read back differ from the file: the memory may be read-only")
                             .arg(unchanged).arg(data.size()));
    return true;
}
