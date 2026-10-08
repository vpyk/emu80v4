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

#include "../Pal.h"

#include "wxLogWnd.h"

using namespace std;

//(*InternalHeaders(LogWnd)
#include <wx/intl.h>
#include <wx/string.h>
//*)

const long LogWnd::ID_HIDEBUTTON = wxNewId();
const long LogWnd::ID_QUITBUTTON = wxNewId();
//const long LogWnd::ID_TEXTCTRL1 = wxNewId();
//const long LogWnd::ID_PANEL1 = wxNewId();

BEGIN_EVENT_TABLE(LogWnd, wxFrame)
    EVT_CLOSE(LogWnd::OnClose)
    EVT_BUTTON(ID_HIDEBUTTON, LogWnd::OnHideButtonClick)
    EVT_BUTTON(ID_QUITBUTTON, LogWnd::OnQuitButtonClick)
END_EVENT_TABLE()

LogWnd::LogWnd(wxWindow* parent,wxWindowID id,const wxPoint& /*pos*/,const wxSize& /*size*/)
{
	Create(parent, id, _("Emu80 error log"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE, _T("id"));
	SetClientSize(wxSize(500, 300));
	//Move(wxDefaultPosition);

    wxPanel *panel = new wxPanel(this, -1);
    wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
    //panel->SetSizer(vbox);

    textCtrl = new wxTextCtrl(panel, -1, _(""), wxDefaultPosition, wxDefaultSize, /*wxTE_AUTO_SCROLL|*/wxTE_PROCESS_ENTER|wxTE_MULTILINE|wxTE_READONLY, wxDefaultValidator, _T("ID_TEXTCTRL1"));
    vbox->Add(textCtrl, 1, wxALL|wxEXPAND, 5);

    wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
    vbox->Add(hbox, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 5);

    wxButton *hideBtn = new wxButton(panel, ID_HIDEBUTTON, wxT("Hide"));
    hideBtn->SetDefault();
    wxButton *quitBtn = new wxButton(panel, ID_QUITBUTTON, wxT("Quit"));

    hbox->Add(hideBtn, 0, wxALIGN_CENTER_VERTICAL, 0);
    hbox->Add(quitBtn, 0, wxLEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 5);

    panel->SetSizer(vbox);
    vbox->Fit(panel);
    vbox->SetSizeHints(panel);

    //Centre();
}

LogWnd::~LogWnd()
{
	//(*Destroy(LogWnd)
	//*)
}


void LogWnd::OnClose(wxCloseEvent&)
{
    Hide();
}


void LogWnd::OnHideButtonClick(wxCommandEvent&)
{
    Hide();
}


void LogWnd::OnQuitButtonClick(wxCommandEvent&)
{
    palRequestForQuit();
}


void LogWnd::addText(wxString s)
{
    (*textCtrl) << s/* << '\n'*/;
}
