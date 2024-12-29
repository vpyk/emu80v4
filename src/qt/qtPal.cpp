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

#include <sstream>

#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QThread>
#include <QAudioOutput>
#include <QTranslator>
#include <QSettings>
#include <QSurfaceFormat>
#include <QElapsedTimer>
#include <QDirIterator>
#include <QDateTime>
#include <QDebug>
#include <QMessageBox>
#include <QClipboard>
#include <QAudioFormat>


#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  #include <QRegularExpression>
  #include <QAudioSink>
  #include <QMediaDevices>
  #define AUDIO_SINK_TYPE QAudioSink
#else
  #include <QRegExp>
  #define QRegularExpression QRegExp
  #define AUDIO_SINK_TYPE QAudioOutput
#endif


#include "qtMainWindow.h"
#include "qtRenderHelper.h"
#include "qtPal.h"
#include "qtPalWindow.h"
#include "qtChoosePlatformDialog.h"
#include "qtPlatformConfig.h"
#include "qtAudioDevice.h"

#include "../Pal.h"
#include "../EmuTypes.h"
#include "../EmuCalls.h"


using namespace std;

static string basePath;

static int sampleRate = 48000;
//static int frameRate = 100;
static bool vsync = true;

static bool isRunning = false;
static bool dontRun = false;

static QElapsedTimer timer;
static QApplication* application;
static QTranslator* translator;

QApplication* getApplication() {return application;};


bool palQtInit(int& argc, char** argv)
{
#ifdef Q_OS_WIN32
    // Graphics driver options
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-opengl", 7) == 0) {
            QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
            break;
        }
        else if (strncmp(argv[i], "-angle", 6) == 0) {
            QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
            break;
        }
        else if (strncmp(argv[i], "-dx9", 4) == 0) {
            QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
            qputenv("QT_ANGLE_PLATFORM", "d3d9");
            break;
        }
        else if (strncmp(argv[i], "-dx11", 5) == 0) {
            QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
            qputenv("QT_ANGLE_PLATFORM", "d3d11");
            break;
        }
        else if (strncmp(argv[i], "-warp", 5) == 0) {
            QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
            qputenv("QT_ANGLE_PLATFORM", "warp");
            break;
        }
        if (strncmp(argv[i], "-soft", 5) == 0) {
            QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
            break;
        }
    }
#endif

    static int one = 1;
    application = new QApplication(one, argv); // suppress passing additional arguments

    QString p = QCoreApplication::applicationDirPath() + "/_settings/" + "emu80.ini";
    QDir settingsDir(QCoreApplication::applicationDirPath() + "/_settings");
    if (settingsDir.exists()) {
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QCoreApplication::applicationDirPath());
        QCoreApplication::setOrganizationName("_settings");
    } else
        QCoreApplication::setOrganizationName("Emu80");
    QCoreApplication::setApplicationName("emu80");

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings settings;

    settings.beginGroup("system");

    QString locale = "system";
    if (!settings.contains("locale")) {
        settings.setValue("locale", locale);
    } else
        locale = settings.value("locale").toString();

    /*QString glDriver = "es";
    if (!settings.contains("glDriver")) {
        settings.setValue("glDriver", glDriver);
    } else
        glDriver = settings.value("glDriver").toString();*/

    /*frameRate = 100;
    if (!settings.contains("maxFps"))
        settings.setValue("maxFps", frameRate);
    else
        frameRate = settings.value("maxFps").toInt();

    bool limitFps = true;
    if (!settings.contains("limitFps"))
        settings.setValue("limitFps", limitFps);
    else
        limitFps = settings.value("limitFps").toBool();
    if (!limitFps)
        frameRate = 0;*/

    vsync = true;
    if (!settings.contains("vsync"))
        settings.setValue("vsync", vsync ? "yes" : "no");
    else
        vsync = settings.value("vsync") == "yes";

    sampleRate = 48000;
    if (!settings.contains("sampleRate"))
        settings.setValue("sampleRate", sampleRate);
    else
        sampleRate = settings.value("sampleRate").toInt();

    settings.endGroup();

    QSurfaceFormat format;
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSwapInterval(vsync ? 1 : 0);
    QSurfaceFormat::setDefaultFormat(format);

    g_renderHelper = new RenderHelper(nullptr);

    ::basePath = application->applicationDirPath().toUtf8().constData();
    ::basePath += '/';

    if (locale == "system")
        locale = QLocale::system().name();
    translator = new QTranslator();
    if (translator->load("emu80_" + locale, ":/translations"))
        application->installTranslator(translator);

    return true;
}


const string& palGetBasePath()
{
    return basePath;
}


static AUDIO_SINK_TYPE* audio = nullptr;

static EmuAudioIoDevice* audioDevice = nullptr;


void palQtQuit()
{
    if (audio)
        audio->stop();
    if (audioDevice)
        delete audioDevice;

    delete translator;
    delete application;
}


void palStart()
{
    //emuSetPropertyValue("emulation", "maxFps", QString::number(frameRate).toStdString());      // !!!
    emuSetPropertyValue("emulation", "vsync", vsync ? "yes" : "no");                           // !!!
    emuSetPropertyValue("emulation", "sampleRate", QString::number(sampleRate).toStdString()); // !!!

    QAudioFormat format;
    format.setSampleRate(sampleRate);
    format.setChannelCount(2);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    format.setSampleFormat(QAudioFormat::Int16);
    format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
#else
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QMediaDevices devices;
    audio = new QAudioSink(devices.defaultAudioOutput(), format);
#else
    audio = new QAudioOutput(format);
#endif

    audio->setBufferSize(sampleRate / 5);
    audioDevice = new EmuAudioIoDevice(sampleRate/*, frameRate*/);
    audioDevice->start();
    audio->start(audioDevice);

    isRunning = true;

    g_renderHelper->start();
}


void palPause()
{
    audio->suspend();
}


void palResume()
{
    audio->resume();
}


void palExecute()
{
    if (!dontRun)
        application->exec();
}


/*bool palSetFrameRate(int)
{
    // Not used in Qt version
    return true;
}*/


bool palSetVsync(bool)
{
    // Not used in Qt version
    return true;
}


bool palSetSampleRate(int)
{
    // Not used in Qt version
    return true;
}


int palGetSampleRate()
{
    return ::sampleRate;
    return true;
}


string palMakeFullFileName(string fileName)
{
    if (fileName[0] == '\0' || fileName[0] == '/' || fileName[0] == '\\' || (fileName.size() > 1 && fileName[1] == ':'))
        return fileName;
    string fullFileName(::basePath);
    fullFileName += fileName;
    return fullFileName;
  return "";
}


int palReadFromFile(const string& fileName, int offset, int sizeToRead, uint8_t* buffer, bool useBasePath)
{
    string fullFileName;
    if (useBasePath)
        fullFileName = palMakeFullFileName(fileName);
    else
        fullFileName = fileName;

    int nBytesRead;
    QFile file(QString::fromUtf8(fullFileName.c_str()));
    if (file.open(QIODevice::ReadOnly)) {
        file.seek(offset);
        nBytesRead = file.read((char *)buffer, sizeToRead);
        file.close();
        return nBytesRead;
    } else
        return 0;
}


uint8_t* palReadFile(const string& fileName, int &fileSize, bool useBasePath)
{
    string fullFileName;
    if (useBasePath)
        fullFileName = palMakeFullFileName(fileName);
    else
        fullFileName = fileName;

    QFile file(QString::fromUtf8(fullFileName.c_str()));
    if (file.open(QIODevice::ReadOnly)) {
        fileSize = file.size();
        uint8_t* buf = new uint8_t[fileSize];
        fileSize = file.read((char *)buf, fileSize);
        file.close();
        return buf;
    } else
        return nullptr;
}


void palRequestForQuit()
{
    if (isRunning)
        application->quit();
    else
        dontRun = true;
}


void palPlaySample(int16_t sample)
{
    if (audioDevice)
        audioDevice->addSample(sample);
}


void palPlaySample(int16_t left, int16_t right)
{
    if (audioDevice)
        audioDevice->addSample(left, right);
}


uint64_t palGetCounter()
{
    return timer.nsecsElapsed();
}


uint64_t palGetCounterFreq()
{
    return 1000000000;
}


void palDelay(uint64_t time)
{
    QThread::currentThread()->usleep(time / 1000);
}


std::string palOpenFileDialog(std::string title, std::string filter, bool write, PalWindow* window)
{
    QSettings settings;
    settings.beginGroup("dirs");

    QString keyPrefix;
    if (window)
        keyPrefix = QString::fromUtf8(window->getPlatformObjectName().c_str()) + "/";
    else
        keyPrefix = "system/";

    QString dirKey = keyPrefix + (write ? "saveDir" : "openDir");
    QString dir = settings.value(dirKey).toString();

    QString filterKey = keyPrefix + (write ? "saveFilter" : "openFilter");
    QString lastFilter = settings.value(filterKey).toString();

    QString qFilter = QString::fromUtf8(filter.c_str());
    QString itemDesc, itemFilter;
    QString newFilter;
    int item = 0;
    do {
        itemDesc = qFilter.section('|', item, item);
        itemFilter = qFilter.section('|', item + 1, item + 1);
        if (itemDesc != "" && itemFilter != "") {
            if (item != 0)
                newFilter += ";;";
            newFilter += itemDesc.section('(', 0, 0).trimmed();
            newFilter += " (";
            int extItem = 0;
            QString ext;
            do {
                ext = itemFilter.section(QRegularExpression("[;,]"), extItem, extItem);
                if (ext != "") {
                    if (extItem != 0)
                        newFilter += " ";
                    newFilter += ext;
                }
                ++extItem;
            } while (ext != "");
            newFilter += ")";
        }
        item += 2;
    } while (itemDesc != "");
    QString fileName = "";
    QWidget* parent = nullptr;
    if (window)
        parent = window->getQtWindow();
    if (write) {
        QFileDialog fileDialog(parent);
        fileDialog.setWindowTitle(QString::fromUtf8(title.c_str()));
        fileDialog.setAcceptMode(QFileDialog::QFileDialog::AcceptSave);
        fileDialog.setNameFilter(newFilter);
        fileDialog.setDirectory(dir);
        if (lastFilter != "" && newFilter.contains(lastFilter))
            fileDialog.selectNameFilter(lastFilter);
        g_renderHelper->pause();
        if (fileDialog.exec() == QDialog::Accepted) {
            fileName = fileDialog.selectedFiles().constFirst(); // Qt >= 5.6
            if (dirKey != "") {
                dir = fileDialog.directory().absolutePath();
                lastFilter = fileDialog.selectedNameFilter();
                settings.setValue(dirKey, dir);
                settings.setValue(filterKey, lastFilter);
            }
        }
        g_renderHelper->resume();
    }
    else {
        QFileDialog fileDialog(parent);
        fileDialog.setWindowTitle(QString::fromUtf8(title.c_str()));
        fileDialog.setAcceptMode(QFileDialog::QFileDialog::AcceptOpen);
        fileDialog.setFileMode(QFileDialog::ExistingFiles);
        fileDialog.setNameFilter(newFilter);
        fileDialog.setDirectory(dir);
        if (lastFilter != "" && newFilter.contains(lastFilter))
            fileDialog.selectNameFilter(lastFilter);
        g_renderHelper->pause();
        if (fileDialog.exec() == QDialog::Accepted) {
            fileName = fileDialog.selectedFiles().constFirst(); // Qt >= 5.6
            if (dirKey != "") {
                dir = fileDialog.directory().absolutePath();
                lastFilter = fileDialog.selectedNameFilter();
                settings.setValue(dirKey, dir);
                settings.setValue(filterKey, lastFilter);
            }
        }
        g_renderHelper->resume();
    }
    settings.endGroup();
    emuResetKeys(window);
    return fileName.toUtf8().constData();
}


void palGetDirContent(const string& dir, list<PalFileInfo*>& fileList)
{
    QDirIterator it(QString::fromUtf8(dir.c_str()), QDir::AllEntries | QDir::NoDotAndDotDot);

    while (it.hasNext()) {
        it.next();
        PalFileInfo* newFile = new PalFileInfo;

        //newFile->fileName = it.fileName().toUtf8().constData();
        QFileInfo info = it.fileInfo();
        newFile->fileName = info.fileName().toUtf8().constData();
        newFile->isDir = info.isDir();
        newFile->size = info.size();
        QDateTime dateTime = info.lastModified();
        newFile->year = dateTime.date().year();
        newFile->month = dateTime.date().month();
        newFile->day = dateTime.date().day();
        newFile->hour = dateTime.time().hour();
        newFile->minute = dateTime.time().minute();
        newFile->second = dateTime.time().second();

        fileList.push_back(newFile);
    }
}


void palUpdateConfig()
{
    g_renderHelper->updateConfig();
}


bool palChoosePlatform(std::vector<PlatformInfo>& pi, int& pos, bool& newWnd, bool setDef, PalWindow* wnd) {
    QWidget* parent = nullptr;
    if (wnd)
        parent = wnd->getQtWindow();
    ChoosePlatformDialog* dialog = new ChoosePlatformDialog(parent);
    g_renderHelper->pause();
    bool res = dialog->execute(pi, pos, newWnd, "", setDef);
    g_renderHelper->resume();
    return res;
}


bool palChooseConfiguration(std::string platformName, PalWindow* wnd)
{
    QWidget* parent = nullptr;
    if (wnd)
        parent = wnd->getQtWindow();
    PlatformConfigDialog* dialog = new PlatformConfigDialog(parent);
    g_renderHelper->pause();
    bool res = dialog->configure(QString::fromUtf8(platformName.c_str()));
    delete dialog;
    g_renderHelper->resume();
    return res;
}


void palGetPlatformDefines(std::string platformName, std::map<std::string, std::string>& definesMap)
{
    string platformGroupName = platformName.substr(0, platformName.find(".",0));
    QSettings settings;
    settings.beginGroup(QString::fromUtf8(platformGroupName.c_str()) + "-config");
    QStringList keys = settings.allKeys();
    for (const auto& key: keys) {
        QString value = settings.value(key).toString();
        definesMap.insert(std::make_pair(key.toUtf8().constData(), value.toUtf8().constData()));
    }
    settings.endGroup();
}


std::string palGetDefaultPlatform()
{
    QSettings settings;
    settings.beginGroup("system");
    return settings.value("platform", "").toString().toUtf8().constData();
}


void palGetPalDefines(std::list<std::string>& defineList)
{
    defineList.push_back("QT");
}


void palCopyTextToClipboard(const char* text)
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    QString str = QString::fromUtf8(text);
    clipboard->setText(str);
}


string palGetTextFromClipboard()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    QString str = clipboard->text();
    return str.toUtf8().constData();
}


static string logStr = "";

void palLog(std::string str) {
    logStr += str;
    string::size_type pos = logStr.find("\n");
    while (pos != string::npos) {
        qDebug() << logStr.substr(0, pos).c_str();
        if (pos < logStr.size())
            logStr = logStr.substr(pos + 1);
        else
            logStr = "";
        pos = logStr.find("\n");
    }
}


EmuLog& EmuLog::operator<<(string s)
{
    palLog(s);
    return *this;
}


EmuLog& EmuLog::operator<<(const char* sz)
{
    string s = sz;
    palLog(s);
    return *this;
}


EmuLog& EmuLog::operator<<(int n)
{
    ostringstream oss;
    oss << n;
    string s = oss.str();
    palLog(s);
    return *this;
}


void palMsgBox(std::string msg, bool critical)
{
    QMessageBox msgBox(critical ? QMessageBox::Critical : QMessageBox::Information, "Emu80", QString::fromUtf8(msg.c_str()));
    msgBox.exec();
}


EmuLog emuLog;


// Unimplemented functions
void palSetRunFileName(std::string) {
}

void palShowConfigWindow(int) {
}

void palAddTabToConfigWindow(int, std::string) {
}

void palRemoveTabFromConfigWindow(int) {
}

void palAddRadioSelectorToTab(int, int, std::string, std::string, std::string, SelectItem*, int) {
}

void palSetTabOptFileName(int, string) {
}

void palWxProcessMessages() {
}
