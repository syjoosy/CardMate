#ifndef FLASHTHREAD_H
#define FLASHTHREAD_H

#include "../threads/Events.h"
#include <wx/thread.h>
#include <wx/string.h>
#include <wx/event.h>

class wxWindow;


class FlashThread : public wxThread {
public:
    FlashThread(wxWindow* parent, const wxString& srcFile, const wxString& dstFile);
    
protected:
    virtual ExitCode Entry() override;

private:
    wxWindow* m_parent;
    wxString m_srcFile;
    wxString m_dstFile;

    void SendUpdate(int percent, const wxString& msg);
    void SendLog(const wxString& msg);
};

#endif