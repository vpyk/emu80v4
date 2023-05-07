/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2023
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

#include <string>
#include <sstream>
#include <iostream>

#include <wx/app.h>
#include <wx/filedlg.h>
#include <wx/dir.h>
#include <wx/filename.h>

//#include "../PalSdl.h"
#include "wxPal.h"
#include "wxChPlatformDlg.h"
#include "wxConfigWnd.h"
#include "wxLogWnd.h"

using namespace std;

static ConfigWnd* configWnd;
static LogWnd* logWnd;
static wxString runFileName = "";

wxIMPLEMENT_APP_NO_MAIN(wxApp);

bool palWxInit(int argc, char** argv)
{
    if (wxEntryStart(argc, argv)) {
       	configWnd = new ConfigWnd(0);
        logWnd = new LogWnd(0);
        return true;
    }
    return false;
}


void palWxQuit()
{
    wxEntryCleanup();
}


string palOpenFileDialog(string title, string filter, bool write, PalWindow*)
{
    //palPause();
    wxString wxFilter = wxString::FromUTF8(filter.c_str());
    wxString wxTitle = wxString::FromUTF8(title.c_str());
    wxFileDialog* openFileDialog = NULL;
    if (write) {
        openFileDialog = new wxFileDialog(NULL, wxTitle, "", "", wxFilter, wxFD_SAVE | wxFD_CHANGE_DIR | wxFD_OVERWRITE_PROMPT);
    } else {
        openFileDialog = new wxFileDialog(NULL, wxTitle, "", "", wxFilter, wxFD_OPEN | wxFD_CHANGE_DIR | wxFD_FILE_MUST_EXIST);
    }
    string res = "";
    if (openFileDialog->ShowModal() != wxID_CANCEL) {
        res = openFileDialog->GetPath().utf8_str();
    }
    openFileDialog->Destroy();
    //palResume();
    return res;
}


void palGetDirContent(const string& dir, list<PalFileInfo*>& fileList)
{
    wxString dirName = wxString::FromUTF8(dir.c_str());
    wxDir aDir(dirName);

    if (!aDir.IsOpened())
        return;

    wxString fileName;

    bool cont = aDir.GetFirst(&fileName, wxEmptyString, wxDIR_DIRS);
    while (cont)
    {
        PalFileInfo* newFile = new PalFileInfo;
        newFile->fileName = fileName.utf8_str();

        newFile->isDir = true;
        newFile->size = 0;

        wxFileName file;
        file.AssignDir(dirName + fileName);

        wxDateTime fileTime = file.GetModificationTime();

        newFile->year = fileTime.GetYear();
        newFile->month = fileTime.GetMonth() + 1;
        newFile->day = fileTime.GetDay();
        newFile->hour = fileTime.GetHour();
        newFile->minute = fileTime.GetMinute();
        newFile->second = fileTime.GetSecond();

        fileList.push_back(newFile);

        cont = aDir.GetNext(&fileName);
    }

    cont = aDir.GetFirst(&fileName, wxEmptyString, wxDIR_FILES);
    while (cont)
    {
        PalFileInfo* newFile = new PalFileInfo;
        newFile->fileName = fileName.utf8_str();

        wxFileName file(dirName, fileName);

        newFile->isDir = false;
        newFile->size = (uint32_t)file.GetSize().ToULong();

        wxDateTime fileTime = file.GetModificationTime();

        newFile->year = fileTime.GetYear();
        newFile->month = fileTime.GetMonth() + 1;
        newFile->day = fileTime.GetDay();
        newFile->hour = fileTime.GetHour();
        newFile->minute = fileTime.GetMinute();
        newFile->second = fileTime.GetSecond();
        fileList.push_back(newFile);

        cont = aDir.GetNext(&fileName);
    }
}


bool palChoosePlatform(vector<PlatformInfo>& pi, int& pos, bool& newWnd, bool setDef, PalWindow* wnd)
{
    ChPlatformDlg* dlg = new ChPlatformDlg(0L, _("Choose platform"));
    return dlg->execute(pi, pos, newWnd, runFileName, setDef);
}


bool palChooseConfiguration(std::string platformName, PalWindow* wnd)
{
    return false;
}


void palSetRunFileName(std::string runFileName) {
    ::runFileName = wxString::FromUTF8(runFileName.c_str());
}

void palShowConfigWindow(int curTabId)
{
    configWnd->setCurTabId(curTabId);
    configWnd->Show();
    configWnd->Raise();
}


void palAddTabToConfigWindow(int tabId, string tabName)
{
    configWnd->addTab(tabId, wxString::FromUTF8(tabName.c_str()));
}


void palRemoveTabFromConfigWindow(int tabId)
{
    configWnd->removeTab(tabId);
}


void palAddRadioSelectorToTab(int tabId, int column, string caption, string object, string property, SelectItem* items, int nItems)
{
    wxString* wsItems = new wxString[nItems];
    wxString* wsValues = new wxString[nItems];
    int selectedItem = 0;
    for (int i = 0; i < nItems; i++) {
        wsItems[i] = wxString::FromUTF8(items[i].name.c_str());
        wsValues[i] = wxString::FromUTF8(items[i].value.c_str());
        if (items[i].selected)
            selectedItem = i;
    }
    configWnd->addRadioSelector(tabId, column, wxString::FromUTF8(caption.c_str()), wxString::FromUTF8(object.c_str()),
                                wxString::FromUTF8(property.c_str()), wsItems, wsValues, nItems, selectedItem);
}


void palSetTabOptFileName(int tabId, string optFileName)
{
    wxString wxOptFileName = wxString::FromUTF8(optFileName.c_str());
    configWnd->setOptFileName(tabId, wxOptFileName);
}


void palUpdateConfig()
{
    configWnd->updateConfig();
}


void palGetPalDefines(std::list<std::string>& defineList)
{
    defineList.push_back("SDL");
    defineList.push_back("WX");
}


void palGetPlatformDefines(std::string platformName, std::map<std::string, std::string>& definesMap)
{

}


void palWxProcessMessages()
{
    wxYield();
    wxGetApp().ProcessPendingEvents();
}


void palLog(string s)
{
    logWnd->addText(wxString::FromUTF8(s.c_str()));
    logWnd->Show();
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


void palMsgBox(string msg, bool critical)
{
    wxMessageBox(wxString::FromUTF8(msg.c_str()),
                 wxT("Emu80"),
                 wxOK | (critical ? wxICON_ERROR : wxICON_INFORMATION));
}


EmuLog emuLog;
