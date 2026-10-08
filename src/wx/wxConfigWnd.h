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

#ifndef CONFIGWND_H
#define CONFIGWND_H

#include <map>
#include <list>

#include <wx/notebook.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/radiobox.h>
#include <wx/textctrl.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>


class ConfigWndTab;
class ConfigWndRadioSelector;

class ConfigWnd: public wxDialog
{
	public:

		ConfigWnd(wxWindow* parent);
		virtual ~ConfigWnd();

		void addTab(int tabId, wxString tabName);
		void removeTab(int tabId);
        void setCurTabId(int tabId);
        void addRadioSelector(int tabId, int column, wxString caption, wxString object, wxString property, wxString* items, wxString* values, int nItems, int selectedItem);
        void updateConfig();
        void setOptFileName(int tabId, wxString optFileName);

	protected:

		static const long ID_RADIOBOX1;
		static const long ID_RADIOBOX2;
		static const long ID_NOTEBOOK;

		friend class ConfigWndTab;
		friend class ConfigWndRadioSelector;

	private:

		wxNotebook* m_notebook;
		wxBoxSizer* m_mainVSizer;
		wxPanel* m_helpPanel;
        wxCheckBox* m_saveCheckBox;
        std::map<int, ConfigWndTab*> m_tabsMap;
        std::list<ConfigWndRadioSelector*> m_selectorList;

        void OnOk(wxCommandEvent& event);
        void OnApply(wxCommandEvent& event);
        void OnCancel(wxCommandEvent& event);
        void OnChangePage(wxBookCtrlEvent& event);

        void Save();

		DECLARE_EVENT_TABLE()
};

class ConfigWndTab
{
    public:
        ConfigWndTab(ConfigWnd* configWnd, wxString tabName, int tabId);
        ~ConfigWndTab();

        wxRadioBox* addRadioSelector(int column, wxString caption, wxString property, wxString* items, int nItems, int selectedItem);

		friend class ConfigWnd;

    private:
        int m_tabId;
        ConfigWnd* m_configWnd;
        wxPanel* m_tabPanel;
        wxBoxSizer* m_tabHSizer;
        wxBoxSizer* m_tabVSizers[3];
        wxString m_optFileName;
        //list<RadioBox*> m_radioBoxList;
};


class ConfigWndRadioSelector
{
    public:
        ConfigWndRadioSelector(ConfigWnd* configWnd, int tabId, wxRadioBox* radioBox, wxString object, wxString property, int nItems, wxString* values, int selectedItem);

        void applyChoice();
        void revertChoice();
        void rereadChoice();

        inline int getTabId() {return m_tabId;}

        friend class ConfigWnd;

    private:
        ConfigWnd* m_configWnd;
        int m_tabId;
        wxRadioBox* m_radioBox;
        wxString m_object;
        wxString m_property;
        int m_nItems;
        wxString* m_values;
        int m_selectedItem;
};

#endif
