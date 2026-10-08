/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2017
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

#ifndef CHPLATFORMDLG_H
#define CHPLATFORMDLG_H

#include <wx/wx.h>
#include <wx/button.h>
#include <wx/listbox.h>

#include <vector>

#include "../EmuTypes.h"

class ChPlatformDlg : public wxDialog
{
    public:
        ChPlatformDlg(wxDialog *dlg, const wxString& title);
        ~ChPlatformDlg();

        bool execute(std::vector<PlatformInfo>& pi, int& pos, bool& newWnd, wxString runFileName, bool setDef);

    protected:
		static const long ID_OKBUTTON;
		static const long ID_CANCELBUTTON;
		static const long ID_LISTBOX;

        wxListBox* listBox;

    private:
        wxBoxSizer* m_vSizer;
        wxCheckBox* m_newWndCheckBox;
        wxCheckBox* m_saveCheckBox;

        //void OnCancel(wxCommandEvent& event);
        //void OnOk(wxCommandEvent& event);
        void OnClose(wxCloseEvent& event);
        void OnListDblClk(wxCommandEvent& event);
        DECLARE_EVENT_TABLE()
};

#endif // CHPLATFORMDLG_H
