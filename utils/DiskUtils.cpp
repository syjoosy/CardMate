#include "DiskUtils.h"
#include <wx/dir.h>
#include <wx/stdpaths.h>
#include <wx/notebook.h>
#include <wx/filename.h>
#include <wx/regex.h>
#include <wx/thread.h>
#include <wx/wx.h>
#include <wx/clipbrd.h>
#include <wx/artprov.h>

wxArrayString DiskUtils::GetAvailableDisks() {
    wxArrayString output;
    wxArrayString disks;
    
    long exitCode = wxExecute("diskutil list", output, wxEXEC_SYNC);
    if (exitCode != 0 || output.IsEmpty()) {
        return disks;
    }
    
    for (auto& line : output) {
        line.Trim(true).Trim(false);
        if (line.StartsWith("/dev/disk")) {
            disks.Add(line);
        }
    }
    
    return disks;
}

wxString DiskUtils::GetDiskSize(const wxString& diskPath) {
    wxString cmd;
    cmd.Printf("sh -c \"diskutil info %s | awk -F': ' '/Disk Size/ {print $2}' | awk '{print $1}'\"", 
               diskPath);
    
    wxArrayString info;
    long ret = wxExecute(cmd, info, wxEXEC_SYNC);
    if (ret == 0 && !info.IsEmpty()) {
        return "Disk size: " + info[0] + "GB";
    }
    
    return "Не удалось получить размер диска";
}

bool DiskUtils::ExecuteCommand(const wxString& cmd, wxArrayString& output) {
    long exitCode = wxExecute(cmd, output, wxEXEC_SYNC);
    return exitCode == 0;
}