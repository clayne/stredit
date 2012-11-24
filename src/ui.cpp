/*  StrEdit

    A STRINGS, ILSTRINGS and DLSTRINGS file editor designed for mod translators.

    Copyright (C) 2012    WrinklyNinja

    This file is part of StrEdit.

    StrEdit is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version.

    StrEdit is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with StrEdit.  If not, see
    <http://www.gnu.org/licenses/>.
*/

#include "ui.h"

#include <stdexcept>
#include <algorithm>
#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
#include <wx/aboutdlg.h>

BEGIN_EVENT_TABLE ( MainFrame, wxFrame )
    EVT_CLOSE (MainFrame::OnClose )

    EVT_MENU ( wxID_OPEN , MainFrame::OnOpenFile )
    EVT_MENU ( wxID_SAVE , MainFrame::OnSaveFile )
    EVT_MENU ( wxID_SAVEAS , MainFrame::OnSaveFile )
    EVT_MENU ( wxID_EXIT , MainFrame::OnQuit )
    EVT_MENU ( wxID_HELP , MainFrame::OnViewReadme )
    EVT_MENU ( wxID_ABOUT , MainFrame::OnAbout )

    EVT_LIST_ITEM_SELECTED ( LIST_Strings , MainFrame::OnStringSelect)
    EVT_LIST_ITEM_DESELECTED ( LIST_Strings , MainFrame::OnStringDeselect)

    EVT_SEARCHCTRL_SEARCH_BTN ( SEARCH_Strings , MainFrame::OnStringFilter )
    EVT_SEARCHCTRL_CANCEL_BTN ( SEARCH_Strings , MainFrame::OnStringFilterCancel )
END_EVENT_TABLE()

IMPLEMENT_APP(StrEditApp)

using namespace std;
using namespace stredit;

namespace stredit {
    //UI helper functions.
    wxString translate(char * cstr) {
        return wxString(boost::locale::translate(cstr).str().c_str(), wxConvUTF8);
    }

    wxString translate(string str) {
        return wxString(boost::locale::translate(str).str().c_str(), wxConvUTF8);
    }

    wxString FromUTF8(string str) {
        return wxString(str.c_str(), wxConvUTF8);
    }

    wxString FromUTF8(boost::format f) {
        return FromUTF8(f.str());
    }
}

//Draws the main window when program starts.
bool StrEditApp::OnInit() {
    MainFrame *frame = new MainFrame(wxT("StrEdit"));
    //frame->SetIcon(wxIconLocation(prog_filename));
    frame->Show(true);
    SetTopWindow(frame);

    return true;
}

VirtualList::VirtualList(wxWindow * parent, wxWindowID id) : wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_VIRTUAL) {
    attr = new wxListItemAttr();
}

wxString VirtualList::OnGetItemText(long item, long column) const {
    if (column == 0) {
        if (internalData[item].lDist == 0)
            return "";
        else
            return wxString::Format(wxT("%i"), internalData[item].lDist);
    } else if (column == 1)
        return wxString::Format(wxT("%i"), internalData[item].id);
    else if (column == 2)
        return FromUTF8(internalData[item].oldString);
    else
        return FromUTF8(internalData[item].newString);
}

wxListItemAttr * VirtualList::OnGetItemAttr(long item) const {
    if (internalData[item].edited)
        attr->SetTextColour(*wxRED);
    else
        attr->SetTextColour(*wxBLACK);
    return attr;
}

MainFrame::MainFrame(const wxChar *title) : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxDefaultSize) {
    //Set up menu bar first.
    wxMenuBar * MenuBar = new wxMenuBar();
    // File Menu
    wxMenu * FileMenu = new wxMenu();
    FileMenu->Append(wxID_OPEN);
    FileMenu->Append(wxID_SAVE);
    FileMenu->Append(wxID_SAVEAS);
    FileMenu->Append(wxID_EXIT);
    MenuBar->Append(FileMenu, translate("&File"));
    //Help Menu
    wxMenu * HelpMenu = new wxMenu();
    HelpMenu->Append(wxID_HELP);
    HelpMenu->Append(wxID_ABOUT);
    MenuBar->Append(HelpMenu, translate("&Help"));
    SetMenuBar(MenuBar);

    //Set up stuff in the frame.
    SetBackgroundColour(*wxWHITE);

    searchBox = new wxSearchCtrl(this, SEARCH_Strings, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);

    stringList = new VirtualList(this, LIST_Strings);
    stringList->InsertColumn(0, translate("Lev. Dist."));
    stringList->InsertColumn(1, translate("ID"));
    stringList->InsertColumn(2, translate("Original String"));
    stringList->InsertColumn(3, translate("New String"));

    originalTextBox = new wxTextCtrl(this, TEXT_Original, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
    newTextBox = new wxTextCtrl(this, TEXT_New, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);

    //Contents in one big resizing box.
    wxBoxSizer *bigBox = new wxBoxSizer(wxVERTICAL);

    bigBox->Add(searchBox, 0, wxEXPAND|wxLEFT|wxTOP|wxRIGHT, 5);
    bigBox->Add(stringList, 1, wxEXPAND|wxLEFT|wxRIGHT, 5);
    bigBox->Add(originalTextBox, 0, wxEXPAND|wxALL, 5);
    bigBox->Add(newTextBox, 0, wxEXPAND|wxLEFT|wxBOTTOM|wxRIGHT, 5);

    //Now set the layout and sizes.
    SetSizerAndFit(bigBox);
}

void MainFrame::OnOpenFile(wxCommandEvent& event) {
    //Display the OpenDialog, then get the strings from the picked files and
    //fill the string list with them.

    std::vector<str_data> strList;
    try {
        GetStrings("Skyrim_English.STRINGS", strList);
    } catch (runtime_error& e) {
        wxMessageBox(
            FromUTF8(e.what()),
            translate("StrEdit: Error"),
            wxOK | wxICON_ERROR,
            this);
    }
    //A progress dialog for the above loading would be good.
    sort(strList.begin(), strList.end(), compare_old_new);
    stringList->SetItemCount(strList.size());
    stringList->internalData = strList;
    int i = 0;
/*    for (std::vector<str_data>::const_iterator it=strList.begin(), endIt=strList.end(); it != endIt; ++it) {
        wxListItem item;
        item.SetId(i);
        item.SetText(it->oldString);

        stringList->InsertItem(item);
        if (it->lDist == 0)
            stringList->SetItem(i, 0, "");
        else
            stringList->SetItem(i, 0, wxString::Format(wxT("%i"), it->lDist));
        stringList->SetItem(i, 1, wxString::Format(wxT("%i"), it->id));
        stringList->SetItem(i, 2, FromUTF8(it->oldString));
        stringList->SetItem(i, 3, FromUTF8(it->newString));

        ++i;
    }*/
}

void MainFrame::OnSaveFile(wxCommandEvent& event) {
    if (event.GetId() == wxID_SAVE && !filePath.empty())
        SetStrings(filePath, stringList->internalData);
    else {
        //Display file picker dialog.
        wxFileDialog saveFileDialog(this, _("Save As..."), wxEmptyString, wxEmptyString,
            "Strings files (*.STRINGS;*.DLSTRINGS;*.ILSTRINGS)|*.STRINGS;*.DLSTRINGS;*.ILSTRINGS",
            wxFD_SAVE|wxFD_OVERWRITE_PROMPT);

        if (saveFileDialog.ShowModal() == wxID_CANCEL)
            return;

        filePath = saveFileDialog.GetPath().ToUTF8();
        try {
            SetStrings(filePath, stringList->internalData);
        } catch (runtime_error& e) {
            wxMessageBox(
                FromUTF8(e.what()),
                translate("StrEdit: Error"),
                wxOK | wxICON_ERROR,
                this);
        }
    }
}

void MainFrame::OnQuit(wxCommandEvent& event) {
    Close();
}

void MainFrame::OnClose(wxCloseEvent& event) {
    //Need to prompt save if strings have been edited.

    Destroy();
}

void MainFrame::OnViewReadme(wxCommandEvent& event) {
    if (boost::filesystem::exists(readme_path))
        wxLaunchDefaultApplication(readme_path);
    else  //No ReadMe exists, show a pop-up message saying so.
        wxMessageBox(
            FromUTF8(boost::format(boost::locale::translate("Error: \"%1%\" cannot be found.")) % readme_path),
            translate("StrEdit: Error"),
            wxOK | wxICON_ERROR,
            this);
}

void MainFrame::OnAbout(wxCommandEvent& event) {
    wxAboutDialogInfo aboutInfo;
    aboutInfo.SetName("StrEdit");
    aboutInfo.SetVersion(version_string);
    aboutInfo.SetDescription(translate("A STRINGS, ILSTRINGS and DLSTRINGS file editor designed for mod translators."));
    aboutInfo.SetCopyright("Copyright (C) 2012 WrinklyNinja.");
    aboutInfo.SetWebSite("https://github.com/WrinklyNinja/stredit");
    aboutInfo.SetLicence("This program is free software: you can redistribute it and/or modify\n"
    "it under the terms of the GNU General Public License as published by\n"
    "the Free Software Foundation, either version 3 of the License, or\n"
    "(at your option) any later version.\n"
    "\n"
    "This program is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "GNU General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU General Public License\n"
    "along with this program.  If not, see <http://www.gnu.org/licenses/>.");
    //aboutInfo.SetIcon(wxIconLocation(prog_filename));
    wxAboutBox(aboutInfo);
}


void MainFrame::OnStringSelect(wxListEvent& event) {
    //Load the selected strings.
    str_data data = stringList->internalData[event.GetIndex()];

    originalTextBox->SetValue(FromUTF8(data.oldString));
    newTextBox->SetValue(FromUTF8(data.newString));
}

void MainFrame::OnStringDeselect(wxListEvent& event) {
    //Apply the text in the newTextBox to the fourth column of the string list.
    //If the new text does not match the old text, set its colour to red.
    wxString currStr = stringList->internalData[event.GetIndex()].newString;
    wxString newStr = newTextBox->GetValue();

    if (currStr != newStr) {
        stringList->internalData[event.GetIndex()].newString = newStr.ToUTF8();
        stringList->internalData[event.GetIndex()].edited = true;
        stringList->SetItem(event.GetIndex(), 3, newStr);
    }
}

void MainFrame::OnStringFilter(wxCommandEvent& event) {

}

void MainFrame::OnStringFilterCancel(wxCommandEvent& event) {

}
