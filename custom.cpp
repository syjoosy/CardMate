#include <wx/wx.h>
#include <wx/thread.h>
#include <fstream>
#include <iostream>

// Событие для обновления прогресса
wxDECLARE_EVENT(wxEVT_THREAD_UPDATE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_THREAD_UPDATE, wxThreadEvent);

class FlashThread : public wxThread
{
public:
    FlashThread(wxWindow* parent, const wxString& srcFile, const wxString& dstFile)
        : wxThread(wxTHREAD_DETACHED), m_parent(parent), m_srcFile(srcFile), m_dstFile(dstFile) {}

protected:
    virtual wxThread::ExitCode Entry() override
    {
        const size_t buf_size = 4 * 1024 * 1024; // 4 MB
        char* buffer = new char[buf_size];

        std::cout << "[INFO] Открываем файл образа: " << m_srcFile << std::endl;
        std::ifstream src(m_srcFile.mb_str(), std::ios::binary);
        if (!src)
        {
            std::cerr << "[ERROR] Не удалось открыть файл образа!" << std::endl;
            delete[] buffer;
            return (wxThread::ExitCode)1;
        }

        std::cout << "[INFO] Открываем устройство для записи: " << m_dstFile << std::endl;
        std::ofstream dst(m_dstFile.mb_str(), std::ios::binary);
        if (!dst)
        {
            std::cerr << "[ERROR] Не удалось открыть устройство. Запустите с sudo и укажите /dev/rdiskN!" << std::endl;
            delete[] buffer;
            return (wxThread::ExitCode)1;
        }

        src.seekg(0, std::ios::end);
        size_t file_size = src.tellg();
        src.seekg(0, std::ios::beg);
        std::cout << "[INFO] Размер образа: " << file_size << " байт" << std::endl;

        size_t total_bytes = 0;

        while (src)
        {
            src.read(buffer, buf_size);
            std::streamsize read_bytes = src.gcount();
            if (read_bytes == 0) break;

            dst.write(buffer, read_bytes);
            total_bytes += read_bytes;

            int percent = int((double)total_bytes / file_size * 100);

            // Отправляем событие в GUI-поток
            wxThreadEvent event(wxEVT_THREAD_UPDATE);
            event.SetInt(percent);
            wxQueueEvent(m_parent, event.Clone());

            // Вывод прогресса в терминал
            std::cout << "\r[PROGRESS] " << percent << "% (" << total_bytes << "/" << file_size << " байт)" << std::flush;
        }

        std::cout << std::endl << "[INFO] Запись завершена!" << std::endl;
        delete[] buffer;

        // Уведомление о завершении
        wxThreadEvent done(wxEVT_THREAD_UPDATE);
        done.SetInt(100);
        wxQueueEvent(m_parent, done.Clone());

        wxQueueEvent(m_parent, new wxThreadEvent(wxEVT_THREAD, wxID_ANY));

        return (wxThread::ExitCode)0;
    }

private:
    wxWindow* m_parent;
    wxString m_srcFile;
    wxString m_dstFile;
};

class MyFrame : public wxFrame
{
public:
    MyFrame() : wxFrame(nullptr, wxID_ANY, "SD Flash Tool", wxDefaultPosition, wxSize(400, 150))
    {
        wxPanel* panel = new wxPanel(this);
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

        m_gauge = new wxGauge(panel, wxID_ANY, 100);
        sizer->Add(m_gauge, 0, wxEXPAND | wxALL, 10);

        wxButton* startBtn = new wxButton(panel, wxID_ANY, "Start Flashing");
        sizer->Add(startBtn, 0, wxALIGN_CENTER | wxALL, 10);

        panel->SetSizer(sizer);

        Bind(wxEVT_BUTTON, &MyFrame::OnStart, this, startBtn->GetId());
        Bind(wxEVT_THREAD_UPDATE, &MyFrame::OnUpdate, this);
    }

private:
    void OnStart(wxCommandEvent&)
    {
        std::cout << "[INFO] Нажата кнопка Start Flashing" << std::endl;

        wxString src = "/Users/alex/Downloads/licheervnano_sd.img";   // путь к образу
        wxString dst = "/dev/rdisk6";    // SD-карта

        FlashThread* thread = new FlashThread(this, src, dst);
        if (thread->Run() != wxTHREAD_NO_ERROR)
        {
            std::cerr << "[ERROR] Не удалось запустить поток!" << std::endl;
        }
    }

    void OnUpdate(wxThreadEvent& event)
    {
        int value = event.GetInt();
        m_gauge->SetValue(value);

        if (value == 100)
        {
            wxMessageBox("Запись завершена!");
        }
    }

    wxGauge* m_gauge;
};

class MyApp : public wxApp
{
public:
    virtual bool OnInit() override
    {
        MyFrame* frame = new MyFrame();
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);
