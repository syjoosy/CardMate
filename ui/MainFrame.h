#ifndef MAINFRAME_H
#define MAINFRAME_H

#include "../threads/Events.h"
#include <wx/frame.h>

class wxTextCtrl;
class wxGauge;
class wxChoice;
class wxNotebook;

class MainFrame : public wxFrame {
public:
    MainFrame(const wxString& title);

private:
    // UI Elements
    wxNotebook* m_notebook;
    
    // Flash tab
    wxTextCtrl* m_filePathText;
    wxTextCtrl* m_diskPathText;
    wxTextCtrl* m_fileSizeText;
    wxTextCtrl* m_diskSizeText;
    wxGauge*    m_progressBar;
    wxTextCtrl* m_logWindow;

    // Backup tab
    wxTextCtrl* m_backupDiskText;
    wxTextCtrl* m_backupFileText;
    wxGauge*    m_backupProgress;
    wxTextCtrl* m_backupLog;

    // Format tab
    wxTextCtrl* m_formatDiskText;
    wxTextCtrl* m_formatDiskSizeText;
    wxChoice*   m_fsChoice;
    wxTextCtrl* m_formatLog;
    wxTextCtrl* diskNameText;

    unsigned long long m_fileSizeBytes = 0;

    // Event handlers
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnOpenFile(wxCommandEvent& event);
    void OnSelectDisk(wxCommandEvent& event);
    void OnStartWrite(wxCommandEvent& event);
    void OnUpdate(wxThreadEvent& event);
    void OnSelectDiskBackup(wxCommandEvent& event);
    void OnSaveImage(wxCommandEvent& event);
    void OnStartBackup(wxCommandEvent& event);
    void OnSelectFormatDisk(wxCommandEvent& event);
    void OnStartFormat(wxCommandEvent& event);

    // Helper methods
    void CreateFlashTab(wxWindow* parent);
    void CreateBackupTab(wxWindow* parent);
    void CreateFormatTab(wxWindow* parent);
    void ShowDiskSelectionDialog(wxTextCtrl* diskText, wxTextCtrl* sizeText, const wxString& title);
};

#endif