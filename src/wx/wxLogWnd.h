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

#ifndef LOGWND_H
#define LOGWND_H

#include <string>


//(*Headers(LogWnd)
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/frame.h>
//*)

class LogWnd : public wxFrame
{
	public:

		LogWnd(wxWindow* parent,wxWindowID id=wxID_ANY,const wxPoint& pos=wxDefaultPosition,const wxSize& size=wxDefaultSize);
		virtual ~LogWnd();

		void addText(wxString s);

		//wxButton* Button1;
		//wxPanel* Panel1;

	protected:

		static const long ID_HIDEBUTTON;
		static const long ID_QUITBUTTON;
		//static const long ID_TEXTCTRL1;
		//static const long ID_PANEL1;

	private:

		wxTextCtrl* textCtrl;

        void OnClose(wxCloseEvent &event);
		void OnHideButtonClick(wxCommandEvent& event);
		void OnQuitButtonClick(wxCommandEvent& event);

		DECLARE_EVENT_TABLE()
};


#endif

