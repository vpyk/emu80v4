/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2016-2024
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

#include "wxConfigWnd.h"
#include "../Pal.h"
#include "../EmuTypes.h"
#include "../EmuCalls.h"

#include <wx/intl.h>
#include <wx/string.h>
#include <wx/file.h>
#include <wx/filename.h>

using namespace std;

//const long ConfigWnd::ID_RADIOBOX1 = wxNewId();
//const long ConfigWnd::ID_APPLYBUTTON = wxNewId();
//const long ConfigWnd::ID_OKBUTTON = wxNewId();
//const long ConfigWnd::ID_CANCELBUTTON = wxNewId();
const long ConfigWnd::ID_NOTEBOOK = wxNewId();

wxBEGIN_EVENT_TABLE(ConfigWnd,wxDialog)
    EVT_BUTTON(wxID_OK, ConfigWnd::OnOk)
    EVT_BUTTON(wxID_APPLY, ConfigWnd::OnApply)
    EVT_BUTTON(wxID_CANCEL, ConfigWnd::OnCancel)
    EVT_NOTEBOOK_PAGE_CHANGED(ID_NOTEBOOK, ConfigWnd::OnChangePage)
wxEND_EVENT_TABLE()

ConfigWnd::ConfigWnd(wxWindow* parent)
{
	Create(parent, wxID_ANY, "Emu80 Configuration", wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE, _T("wxID_ANY"));
	m_mainVSizer = new wxBoxSizer(wxVERTICAL);

	m_notebook = new wxNotebook(this, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, 0, _T("ID_m_notebook"));

	m_mainVSizer->Add(m_notebook, 1, wxEXPAND, 0);

	wxPanel* bottomPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, _T("wxID_ANY"));
	wxBoxSizer* bottomHSizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton* okBtn = new wxButton(bottomPanel, wxID_OK, wxT("Ok"));
    okBtn->SetDefault();
    wxButton* applyBtn = new wxButton(bottomPanel, wxID_APPLY, wxT("Apply"));
    wxButton* cancelBtn = new wxButton(bottomPanel, wxID_CANCEL, wxT("Cancel"));
    //cancelBtn->

    m_saveCheckBox = new wxCheckBox(bottomPanel, 0, wxT("Save"));
    m_saveCheckBox->SetValue(false);

    bottomHSizer->Add(m_saveCheckBox, 0, wxLEFT|wxTOP|wxBOTTOM|wxALIGN_CENTER_VERTICAL, 5);

	bottomHSizer->Add(0,0,1, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
    bottomHSizer->Add(okBtn, 0, wxLEFT|wxTOP|wxBOTTOM|wxALIGN_CENTER_VERTICAL, 5);
    bottomHSizer->Add(applyBtn, 0, wxLEFT|wxTOP|wxBOTTOM|wxALIGN_CENTER_VERTICAL, 5);
    bottomHSizer->Add(cancelBtn, 0, wxLEFT|wxTOP|wxBOTTOM|wxALIGN_CENTER_VERTICAL|wxRIGHT, 5);

	bottomPanel->SetSizer(bottomHSizer);
	m_mainVSizer->Add(bottomPanel, 0, wxEXPAND, 0);
	bottomHSizer->Fit(bottomPanel);
	bottomHSizer->SetSizeHints(bottomPanel);
	SetSizer(m_mainVSizer);

	m_mainVSizer->Fit(this);
	m_mainVSizer->SetSizeHints(this);


	m_helpPanel = new wxPanel(m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, _T("wxID_ANY"));
	m_notebook->AddPage(m_helpPanel, _("Help"), false);

	wxBoxSizer* tabSizer = new wxBoxSizer(wxHORIZONTAL);
	m_helpPanel->SetSizer(tabSizer);

	wxString helpString1 =
        L"Управление:\n\n"
        "Alt-F12\t– окно конфигурации\n"
        "Alt-F11\t– reset\n"
        "Alt-F10\t– закрыть текущее окно\n"
        "Alt-F9\t– выбор платформы\n"
        "Alt-L\t– загрузка файла\n"
        "Alt-W\t– загрузка WAV-файла\n"
        "Alt-F3\t– загрузка файла с запуском\n"
        "Alt-D\t– отладчик\n"
        "Alt-X\t– выход\n"
        "Alt-Q\t– раскладка Querty\n"
        "Alt-J\t– раскладка Йцукен\n"
        "Alt-K\t– «Умная» раскладка\n"
        "Alt-A\t– выбор диска A\n"
        "Alt-B\t– выбор диска B\n"
        "Alt-Shift-C\t– выбор диска 3(C)\n"
        "Alt-Shift-D\t– выбор диска 4(D)\n"
        "Alt-E\t– выбор электронного диска\n"
        "Alt-O\t– сохранение электронного диска\n"
        "Alt-Shift-H\t– выбор жесткого диска"
        ;

	wxString helpString2 =
        L"Alt-1..5\t– пресет: 1x-5x, автоподстройка размера окна\n"
        "Alt-8\t– пресет: 1.5x, автоподстройка размера окна\n"
        "Alt-9\t– пресет: 2.5x, автоподстройка размера окна\n"
        "Alt-0\t– пресет: измен. размер окна, вписать или растянуть\n"
        "Alt-Shift-M\t– то же, что и Alt-0 + максимизировать окно\n"
        "Alt-F\t– оригинальный/альтернативный шрифт\n"
        "Alt-C\t– переключить режим цвета\n"
        "Alt-R\t– переключить учитывание Aspect Ratio\n"
        "Alt-V\t– переключить отображение только видимой области экрана\n"
        "Alt-N\t– переключить формат экрана 4:3 / 16:9\n"
        "Alt-H\t– скриншот экрана\n"
        "Alt-Shift-Ins\t– копировать текстовый экран\n"
        "Alt-M\t– переключение беззвучного режима\n"
        "Alt-Enter\t– полноэкранный режим\n"
        "Shift-Alt-P\t– выбор файла для перенаправления печати\n"
        "End\t– ускоренная работа при нажатой клавише (4x)\n"
        "Alt-End\t– макс. скорость при нажатой клавише\n"
        "Pause, Alt-P\t– пауза/возобновление\n"
        "Alt-PgUp, Alt-PgDn\t- ускорение/замедление\n"
        "Alt-Up, Alt-Down\t- точное ускорение/замедление\n"
        "Alt-Home\t- нормальная скорость\n"
        "Alt-F1\t– эта подсказка\n\n"
        "Вместо клавиши Alt может быть использована Meta (Win)"
        ;

	wxStaticText* helpText1 = new wxStaticText(m_helpPanel, wxID_ANY, helpString1);
    wxFont font1 = helpText1->GetFont();
    font1.SetPointSize(font1.GetPointSize() - font1.GetPointSize() / 5);
    helpText1->SetFont(font1);
	tabSizer->Add(helpText1, 1, wxTOP|wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 12);

	wxStaticText* helpText2 = new wxStaticText(m_helpPanel, wxID_ANY, helpString2);
    wxFont font2 = helpText2->GetFont();
    font2.SetPointSize(font2.GetPointSize() - font2.GetPointSize() / 5);
    helpText2->SetFont(font2);
	tabSizer->Add(helpText2, 2, wxTOP|wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 12);

	m_mainVSizer->Fit(this);
	m_mainVSizer->SetSizeHints(this);
}

ConfigWnd::~ConfigWnd()
{
    for (auto it = m_selectorList.begin(); it != m_selectorList.end(); it++)
        delete (*it);
}


void ConfigWnd::OnOk(wxCommandEvent& event)
{
    OnApply(event);
    Hide();
}


void ConfigWnd::OnCancel(wxCommandEvent&)
{
    for (auto it = m_selectorList.begin(); it != m_selectorList.end(); it++)
        (*it)->revertChoice();
    Hide();
}


void ConfigWnd::OnChangePage(wxBookCtrlEvent&)
{
    // Notebook page changed
    int pageNo = m_notebook->GetSelection();
    for (auto it = m_tabsMap.begin(); it != m_tabsMap.end(); it++)
        if (m_notebook->FindPage(it->second->m_tabPanel) == pageNo) {
            wxString fileName = it->second->m_optFileName;
            if (fileName == "")
                m_saveCheckBox->Hide();
            else {
                wxString name, ext;
                wxFileName::SplitPath(fileName, nullptr, &name, &ext);
                if (ext != "")
                    name = name + "." + ext;
                m_saveCheckBox->SetLabel(L"Save " + name);
                m_saveCheckBox->Show();
            }
            return;
        }
    m_saveCheckBox->Hide();
    return;
}


void ConfigWnd::Save()
{
    // Save opt file

    int pageNo = m_notebook->GetSelection();
    for (auto it = m_tabsMap.begin(); it != m_tabsMap.end(); it++)
        if (m_notebook->FindPage(it->second->m_tabPanel) == pageNo) {
            wxString name = it->second->m_optFileName;
            int tabId = it->second->m_tabId;
            if (name == "")
                break;
            wxFile file(name, wxFile::write);
            //file.Write(name);

            for (auto it = m_selectorList.begin(); it != m_selectorList.end(); it++) {
                if ((*it)->m_tabId == tabId) {
                    wxString obj = (*it)->m_object;

                    if (obj.Find('.') != wxNOT_FOUND)
                        obj = obj.AfterLast('.');
                    else if (obj != "emulation" && obj != "wavReader") // workaround for global opt-file
                        obj = "platform";

                    wxString line = obj + "." + (*it)->m_property + " = " + (*it)->m_values[(*it)->m_selectedItem] + "\n";
                    file.Write(line);
                }
            }

            file.Close();
            break;
        }
}


void ConfigWnd::OnApply(wxCommandEvent&)
{
    for (auto it = m_selectorList.begin(); it != m_selectorList.end(); it++)
        (*it)->applyChoice();
    if (m_saveCheckBox->GetValue())
        Save();
    Raise();
}

void ConfigWnd::addTab(int tabId, wxString tabName)
{
    ConfigWndTab* newTab = new ConfigWndTab(this, tabName, tabId);
    m_tabsMap.insert(make_pair(tabId, newTab));
}


void ConfigWnd::removeTab(int tabId)
{
    for (auto it = m_selectorList.begin(); it != m_selectorList.end();)
        if ((*it)->getTabId() == tabId)
            m_selectorList.erase(it++);
        else
            it++;

    ConfigWndTab* tab = m_tabsMap[tabId];
    delete tab;
    m_tabsMap.erase(tabId);
}


void ConfigWnd::setCurTabId(int tabId)
{
    int pageNum;
    switch (tabId) {
        case TabIds::TABID_NONE:  // 0
            break;
        case TabIds::TABID_HELP:
            pageNum = m_notebook->FindPage(m_helpPanel);
            if (pageNum != wxNOT_FOUND )
                m_notebook->SetSelection(pageNum);
            break;
        case TabIds::TABID_GENERAL:
            break;
        default:
            pageNum = m_notebook->FindPage(m_tabsMap[tabId]->m_tabPanel);
            if (pageNum != wxNOT_FOUND )
                m_notebook->SetSelection(pageNum);
    }
}


void ConfigWnd::addRadioSelector(int tabId, int column, wxString caption, wxString object, wxString property, wxString* items, wxString* values, int nItems, int selectedItem)
{
    ConfigWndTab* tab = m_tabsMap[tabId];

    wxRadioBox* radioBox = nullptr;

    if (tab) {
        radioBox = tab->addRadioSelector(column, caption, property, items, nItems, selectedItem);
        ConfigWndRadioSelector* rs = new ConfigWndRadioSelector(this, tabId, radioBox, object, property, nItems, values, selectedItem);
        m_selectorList.push_back(rs);
    }
}


void ConfigWnd::updateConfig()
{
    for (auto it = m_selectorList.begin(); it != m_selectorList.end(); it++)
        (*it)->rereadChoice();
}


void ConfigWnd::setOptFileName(int tabId, wxString optFileName)
{
    m_tabsMap[tabId]->m_optFileName = optFileName;
}



ConfigWndTab::ConfigWndTab(ConfigWnd* configWnd, wxString tabName, int tabId)
{
	m_configWnd = configWnd;
	m_tabId = tabId;
	m_tabPanel = new wxPanel(configWnd->m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, _T("wxID_ANY"));
	m_configWnd->m_notebook->AddPage(m_tabPanel, tabName, false);

	m_tabHSizer = new wxBoxSizer(wxHORIZONTAL);

	for (int i = 0; i < 3; i++) {
        m_tabVSizers[i] = new wxBoxSizer(wxVERTICAL);
        m_tabHSizer->Add(m_tabVSizers[i], 0, wxALL|wxEXPAND, 0);
	}
	m_tabPanel->SetSizer(m_tabHSizer);
	//tabHSizer->Fit(tabPanel);
	//tabHSizer->SetSizeHints(tabPanel);
}


ConfigWndTab::~ConfigWndTab()
{
    int pageNum = m_configWnd->m_notebook->FindPage(m_tabPanel);
    if (pageNum != wxNOT_FOUND )
        m_configWnd->m_notebook->DeletePage(pageNum);
}


wxRadioBox* ConfigWndTab::addRadioSelector(int column, wxString caption, wxString /*property*/, wxString* items, int nItems, int selectedItem)
{
	wxRadioBox* radioBox = new wxRadioBox(m_tabPanel, wxID_ANY, caption, wxDefaultPosition, wxDefaultSize, nItems, items, 1, wxRA_SPECIFY_COLS, wxDefaultValidator, _T("ID_RADIOBOX2"));
	radioBox->SetSelection(selectedItem);
	m_tabVSizers[column - 1]->Add(radioBox, 0, wxTOP|wxLEFT|wxRIGHT|wxEXPAND, 5);
    m_tabVSizers[column - 1]->Layout();
	//m_radioBoxList.push_back(radioBox);

	m_tabHSizer->Fit(m_tabPanel);
	m_tabHSizer->SetSizeHints(m_tabPanel);
	m_configWnd->m_notebook->Fit();
	m_configWnd->m_mainVSizer->Fit(m_configWnd);
	m_configWnd->m_mainVSizer->SetSizeHints(m_configWnd);

	return radioBox;
}



ConfigWndRadioSelector::ConfigWndRadioSelector(ConfigWnd* configWnd, int tabId, wxRadioBox* radioBox, wxString object, wxString property, int nItems, wxString* values, int selectedItem)
{
    m_configWnd = configWnd;
    m_tabId = tabId;
    m_radioBox = radioBox;
    m_object = object;
    m_property = property;
    m_nItems = nItems;
    m_values = values;
    m_selectedItem = selectedItem;
}


void ConfigWndRadioSelector::applyChoice()
{
    int curItem = m_radioBox->GetSelection();
    if (curItem != m_selectedItem) {
        m_selectedItem = curItem;
        string object, property, value;
        object = m_object.utf8_str();
        property = m_property.utf8_str();
        value = m_values[m_selectedItem].utf8_str();
        emuSetPropertyValue(object, property, value);
    }
}


void ConfigWndRadioSelector::revertChoice()
{
    m_radioBox->SetSelection(m_selectedItem);
}


void ConfigWndRadioSelector::rereadChoice()
{
    string object, property, value;
    object = m_object.utf8_str();
    property = m_property.utf8_str();
    string curValue = emuGetPropertyValue(object, property);
    value = wxString::FromUTF8(curValue.c_str());

    for (int i=0; i<m_nItems; i++) {
        if (m_values[i] == value) {
            if (m_selectedItem != i) {
                m_selectedItem = i;
                m_radioBox->SetSelection(m_selectedItem);
            }
            break;
        }
    }
}
