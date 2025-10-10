#ifndef BACKUPTHREAD_H
#define BACKUPTHREAD_H

#include "../threads/Events.h"
#include <wx/thread.h>
#include <wx/string.h>
#include <wx/event.h>

class wxWindow;


class BackupThread : public wxThread {
public:
    BackupThread(wxWindow* parent, const wxString& srcDisk, const wxString& dstFile);
    
protected:
    virtual ExitCode Entry() override;

private:
    wxWindow* m_parent;
    wxString m_srcDisk;
    wxString m_dstFile;

    void SendUpdate(int percent, const wxString& msg);
    void SendLog(const wxString& msg);
};

#endif