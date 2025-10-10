#include "BackupThread.h"
#include <wx/window.h>
#include <fstream>

BackupThread::BackupThread(wxWindow* parent, const wxString& srcDisk, const wxString& dstFile)
    : wxThread(wxTHREAD_DETACHED),
      m_parent(parent),
      m_srcDisk(srcDisk),
      m_dstFile(dstFile) {}

wxThread::ExitCode BackupThread::Entry() {
    const size_t buf_size = 4 * 1024 * 1024;
    char* buffer = new char[buf_size];

    std::ifstream src(m_srcDisk.mb_str(), std::ios::binary);
    if (!src) {
        SendLog("Ошибка: не удалось открыть диск для чтения!");
        delete[] buffer;
        return (ExitCode)1;
    }

    std::ofstream dst(m_dstFile.mb_str(), std::ios::binary);
    if (!dst) {
        SendLog("Ошибка: не удалось создать файл образа!");
        delete[] buffer;
        return (ExitCode)1;
    }

    src.seekg(0, std::ios::end);
    size_t disk_size = src.tellg();
    src.seekg(0, std::ios::beg);

    size_t total_bytes = 0;
    while (src) {
        src.read(buffer, buf_size);
        std::streamsize read_bytes = src.gcount();
        if (read_bytes == 0) break;

        dst.write(buffer, read_bytes);
        total_bytes += read_bytes;

        int percent = int((double)total_bytes / disk_size * 100);

        wxString msg = wxString::Format("[PROGRESS] %d%% (%zu/%zu MB)\n",
                                       percent,
                                       total_bytes / 1000 / 1000,
                                       disk_size / 1000 / 1000);
        SendUpdate(percent, msg);
    }

    delete[] buffer;
    SendUpdate(100, "Бэкап завершён!\n");
    return (ExitCode)0;
}

void BackupThread::SendUpdate(int percent, const wxString& msg) {
    wxThreadEvent* event = new wxThreadEvent(wxEVT_THREAD_UPDATE);
    event->SetInt(percent);
    event->SetString(msg);
    wxQueueEvent(m_parent, event);
}

void BackupThread::SendLog(const wxString& msg) {
    wxThreadEvent* event = new wxThreadEvent(wxEVT_THREAD_UPDATE);
    event->SetInt(-1);
    event->SetString(msg + "\n");
    wxQueueEvent(m_parent, event);
}