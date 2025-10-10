#ifndef DISKUTILS_H
#define DISKUTILS_H

#include <wx/arrstr.h>

class DiskUtils {
public:
    static wxArrayString GetAvailableDisks();
    static wxString GetDiskSize(const wxString& diskPath);
    static bool ExecuteCommand(const wxString& cmd, wxArrayString& output);
};

#endif