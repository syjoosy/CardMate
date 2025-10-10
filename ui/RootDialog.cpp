#include "RootDialog.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/statbmp.h>
#include <wx/artprov.h>
#include <wx/clipbrd.h>

RootDialog::RootDialog(wxWindow* parent, const wxString& cmd)
    : wxDialog(parent, wxID_ANY, "Error: Administrator rights are required", 
               wxDefaultPosition, wxSize(500, 220)) {
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* topSizer = new wxBoxSizer(wxHORIZONTAL);

    wxBitmap errorIcon = wxArtProvider::GetBitmap(wxART_ERROR, wxART_MESSAGE_BOX, wxSize(32, 32));
    wxStaticBitmap* icon = new wxStaticBitmap(this, wxID_ANY, errorIcon);
    topSizer->Add(icon, 0, wxALL | wxALIGN_TOP, 10);

    wxStaticText* message = new wxStaticText(this, wxID_ANY,
        "The program is running without elevated rights!\n\n"
        "To make the program work correctly, open Terminal and run the command:"
    );
    topSizer->Add(message, 1, wxALL | wxALIGN_CENTER_VERTICAL, 10);
    mainSizer->Add(topSizer, 0, wxEXPAND);

    wxTextCtrl* cmdField = new wxTextCtrl(this, wxID_ANY, cmd,
                                          wxDefaultPosition, wxDefaultSize,
                                          wxTE_READONLY);
    mainSizer->Add(cmdField, 0, wxALL | wxEXPAND, 10);

    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* copyBtn = new wxButton(this, wxID_ANY, "Copy command");
    wxButton* closeBtn = new wxButton(this, wxID_OK, "Close");
    btnSizer->Add(copyBtn, 0, wxALL, 5);
    btnSizer->Add(closeBtn, 0, wxALL, 5);
    mainSizer->Add(btnSizer, 0, wxALIGN_CENTER);

    SetSizerAndFit(mainSizer);

    copyBtn->Bind(wxEVT_BUTTON, [cmd](wxCommandEvent&) {
        if (wxTheClipboard->Open()) {
            wxTheClipboard->SetData(new wxTextDataObject(cmd));
            wxTheClipboard->Close();
        }
    });
}