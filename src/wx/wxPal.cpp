/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2018
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

#include <wx/app.h>
#include <wx/filedlg.h>

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


string palOpenFileDialog(string title, string filter, bool write)
{
    //palPause();
    wxString wxFilter = wxString::FromUTF8(filter.c_str());
    wxString wxTitle = wxString::FromUTF8(title.c_str());
    wxFileDialog openFileDialog(NULL, wxTitle, "", "", wxFilter, wxFD_OPEN | (write ? 0 : wxFD_FILE_MUST_EXIST));
    string res = "";
    if (openFileDialog.ShowModal() != wxID_CANCEL) {
        res = openFileDialog.GetPath().utf8_str();
    }
    //palResume();
    return res;
}


bool palChoosePlatform(vector<PlatformInfo>& pi, int& pos, bool& newWnd, bool setDef)
{
    ChPlatformDlg* dlg = new ChPlatformDlg(0L, _("Choose platform"));
    return dlg->execute(pi, pos, newWnd, runFileName, setDef);
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

EmuLog emuLog;
