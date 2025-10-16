#include "ui/MainFrame.h"
#include "ui/RootDialog.h"
#include "utils/Constants.h"

#include <wx/wx.h>
#include <wx/stdpaths.h>
#include <unistd.h>

class MyApp : public wxApp {
public:
    virtual bool OnInit();
};

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit() {
    setlocale(LC_ALL, "");
    if (geteuid() != 0) {
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
        wxString cmd = "sudo \"" + exePath + "\"";
        RootDialog dlg(nullptr, cmd);
        dlg.ShowModal();
        return false;
    }

    MainFrame* frame = new MainFrame(APP_NAME + " " + APP_VERSION);
    frame->Show(true);
    return true;
}