#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <wx/string.h>

const wxString APP_VERSION = "v0.2 Dev Build";
const wxString APP_NAME = "CardMate";

// Event IDs
enum {
    ID_OPEN_FILE = 1,
    ID_SELECT_DISK,
    ID_START_WRITE,
    ID_SELECT_DISK_BACKUP,
    ID_SAVE_IMAGE,
    ID_START_BACKUP,
    ID_SELECT_FORMAT_DISK,
    ID_START_FORMAT
};

#endif