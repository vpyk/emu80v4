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

#include <wx/file.h>

#include "wxChPlatformDlg.h"

using namespace std;

//const long ChPlatformDlg::ID_OKBUTTON = wxNewId();
//const long ChPlatformDlg::ID_CANCELBUTTON = wxNewId();
const long ChPlatformDlg::ID_LISTBOX = wxNewId();

BEGIN_EVENT_TABLE(ChPlatformDlg, wxDialog)
    EVT_CLOSE(ChPlatformDlg::OnClose)
    //EVT_BUTTON(ID_CANCELBUTTON, ChPlatformDlg::OnCancel)
    //EVT_BUTTON(ID_OKBUTTON, ChPlatformDlg::OnOk)
    EVT_LISTBOX_DCLICK(ID_LISTBOX, ChPlatformDlg::OnListDblClk)
END_EVENT_TABLE()


ChPlatformDlg::ChPlatformDlg(wxDialog *dlg, const wxString &title) : wxDialog(dlg, -1, title)
{
    //SetSizeHints(wxDefaultSize, wxDefaultSize);
    m_vSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* hSizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton* okBtn = new wxButton(this, wxID_OK/*ID_OKBUTTON*/, wxT("Ok"));
    okBtn->SetDefault();
    wxButton* cancelBtn = new wxButton(this, wxID_CANCEL/*ID_CANCELBUTTON*/, wxT("Отмена"));

    hSizer->Add(okBtn, 0, wxALIGN_CENTER_VERTICAL, 0);
    hSizer->Add(cancelBtn, 0, wxLEFT|wxALIGN_CENTER_VERTICAL|wxLEFT, 5);

    listBox = new wxListBox(this, ID_LISTBOX/*, wxDefaultPosition, wxSize(300, 500)*/);
    wxFont font = listBox->GetFont();
    font.SetPointSize(font.GetPointSize() + font.GetPointSize() / 2);
    listBox->SetFont(font);

    m_vSizer->Add(listBox, 1, wxALL | wxEXPAND, 5);

    m_newWndCheckBox = new wxCheckBox(this, 0, wxT("Открыть в новом окне"));
    m_vSizer->Add(m_newWndCheckBox, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);

    m_saveCheckBox = new wxCheckBox(this, 0, wxT("Установить по умолчанию"));
    m_vSizer->Add(m_saveCheckBox, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);

    m_vSizer->Add(hSizer,  0, wxALIGN_RIGHT | wxRIGHT | wxLEFT | wxBOTTOM, 5);

    SetSizer(m_vSizer);
    //Layout();
    m_vSizer->Fit(this);
	//m_vSizer->SetSizeHints(this);
    Centre();
}


ChPlatformDlg::~ChPlatformDlg()
{
}

void ChPlatformDlg::OnClose(wxCloseEvent&)
{
    EndModal(wxID_CANCEL);
}

/*
void ChPlatformDlg::OnCancel(wxCommandEvent &event)
{
    EndModal(wxID_CANCEL);
}

void ChPlatformDlg::OnOk(wxCommandEvent &event)
{
    EndModal(wxID_OK);
}
*/

void ChPlatformDlg::OnListDblClk(wxCommandEvent&)
{
    EndModal(wxID_OK);
}

bool ChPlatformDlg::execute(vector<PlatformInfo>& pi, int& pos, bool& newWnd, wxString runFileName, bool setDef)
{
    m_saveCheckBox->SetValue(setDef);
    if (runFileName == "")
        m_saveCheckBox->Hide();
    for (auto it = pi.begin(); it != pi.end(); it++) {
        wxString wxs = wxString::FromUTF8((*it).platformName.c_str());
        listBox->InsertItems(1, &wxs, listBox->GetCount());
    }
    listBox->SetSelection(pos);
    m_vSizer->Fit(this);
	m_vSizer->SetSizeHints(this);
    listBox->SetFocus();
    if (ShowModal() == wxID_OK) {
        pos = listBox->GetSelection();
        newWnd = m_newWndCheckBox->GetValue();

        if (runFileName != "" && m_saveCheckBox->GetValue()) {
            wxFile file(runFileName, wxFile::write);
            file.Write("emulation.runPlatform = " + pi[pos].objName);
            file.Close();
        }

        return true;
    }
    return false;
}
