#ifndef ROOTDIALOG_H
#define ROOTDIALOG_H

#include <wx/dialog.h>

class RootDialog : public wxDialog {
public:
    RootDialog(wxWindow* parent, const wxString& cmd);
};

#endif
