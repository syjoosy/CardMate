#include "FlashThread.h"
#include <wx/window.h>
#include <fstream>


FlashThread::FlashThread(wxWindow* parent, const wxString& srcFile, const wxString& dstFile)
    : wxThread(wxTHREAD_DETACHED),
      m_parent(parent),
      m_srcFile(srcFile),
      m_dstFile(dstFile) {}

wxThread::ExitCode FlashThread::Entry() {
    const size_t buf_size = 4 * 1024 * 1024;
    char* buffer = new char[buf_size];

    std::ifstream src(m_srcFile.mb_str(), std::ios::binary);
    if (!src) {
        SendLog("Ошибка: не удалось открыть файл образа!");
        delete[] buffer;
        return (ExitCode)1;
    }

    std::ofstream dst(m_dstFile.mb_str(), std::ios::binary);
    if (!dst) {
        SendLog("Ошибка: не удалось открыть устройство для записи!");
        delete[] buffer;
        return (ExitCode)1;
    }

    src.seekg(0, std::ios::end);
    size_t file_size = src.tellg();
    src.seekg(0, std::ios::beg);

    size_t total_bytes = 0;
    while (src) {
        src.read(buffer, buf_size);
        std::streamsize read_bytes = src.gcount();
        if (read_bytes == 0) break;

        dst.write(buffer, read_bytes);
        total_bytes += read_bytes;

        int percent = int((double)total_bytes / file_size * 100);

        wxString msg = wxString::Format("[PROGRESS] %d%% (%zu/%zu MB)\n",
                                       percent, total_bytes / 1000 / 1000, file_size / 1000 / 1000);
        SendUpdate(percent, msg);
    }

    delete[] buffer;
    SendUpdate(100, "Запись завершена!\n");
    return (ExitCode)0;
}

void FlashThread::SendUpdate(int percent, const wxString& msg) {
    wxThreadEvent* event = new wxThreadEvent(wxEVT_THREAD_UPDATE);
    event->SetInt(percent);
    event->SetString(msg);
    wxQueueEvent(m_parent, event);
}

void FlashThread::SendLog(const wxString& msg) {
    wxThreadEvent* event = new wxThreadEvent(wxEVT_THREAD_UPDATE);
    event->SetInt(-1);
    event->SetString(msg + "\n");
    wxQueueEvent(m_parent, event);
}