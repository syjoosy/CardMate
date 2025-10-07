#include <wx/dir.h>
#include <wx/stdpaths.h>
#include <wx/notebook.h>
#include <wx/filename.h>
#include <wx/regex.h>
#include <wx/thread.h>
#include <wx/wx.h>
#include <wx/clipbrd.h>
#include <wx/artprov.h>
#include <fstream>
#include <iostream>
#include <unistd.h>

#define VERSION "v0.2 Dev Build"

// --- Определение событий для взаимодействия с потоком ---
wxDECLARE_EVENT(wxEVT_THREAD_UPDATE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_THREAD_UPDATE, wxThreadEvent);

// --- Класс для фонового копирования диска в файл ---
class BackupThread : public wxThread {
public:
    BackupThread(wxWindow* parent, const wxString& srcDisk,
                 const wxString& dstFile)
        : wxThread(wxTHREAD_DETACHED),
          m_parent(parent),
          m_srcDisk(srcDisk),
          m_dstFile(dstFile) {}

protected:
    virtual ExitCode Entry() override {
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

private:
    wxWindow* m_parent;
    wxString m_srcDisk;
    wxString m_dstFile;

    void SendUpdate(int percent, const wxString& msg) {
        wxThreadEvent event(wxEVT_THREAD_UPDATE);
        event.SetInt(percent);
        event.SetString(msg);
        wxQueueEvent(m_parent, event.Clone());
    }

    void SendLog(const wxString& msg) {
        wxThreadEvent event(wxEVT_THREAD_UPDATE);
        event.SetInt(-1);
        event.SetString(msg + "\n");
        wxQueueEvent(m_parent, event.Clone());
    }
};

// --- Класс для фоновой записи файла на диск ---
class FlashThread : public wxThread {
public:
    FlashThread(wxWindow* parent, const wxString& srcFile,
                const wxString& dstFile)
        : wxThread(wxTHREAD_DETACHED),
          m_parent(parent),
          m_srcFile(srcFile),
          m_dstFile(dstFile) {}

protected:
    virtual ExitCode Entry() override {
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
            wxMessageBox("Не удалось открыть устройство для записи!", "Ошибка",
                         wxOK | wxICON_ERROR);
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

private:
    wxWindow* m_parent;
    wxString m_srcFile;
    wxString m_dstFile;

    void SendUpdate(int percent, const wxString& msg) {
        wxThreadEvent event(wxEVT_THREAD_UPDATE);
        event.SetInt(percent);
        event.SetString(msg);
        wxQueueEvent(m_parent, event.Clone());
    }

    void SendLog(const wxString& msg) {
        wxThreadEvent event(wxEVT_THREAD_UPDATE);
        event.SetInt(-1);
        event.SetString(msg + "\n");
        wxQueueEvent(m_parent, event.Clone());
    }
};

// --- Диалог для проверки root прав ---
class RootDialog : public wxDialog {
public:
    RootDialog(wxWindow* parent, const wxString& cmd)
        : wxDialog(parent, wxID_ANY, "Error: Administrator rights are required", wxDefaultPosition, wxSize(500, 220))
    {
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
        wxBoxSizer* topSizer = new wxBoxSizer(wxHORIZONTAL);

        wxBitmap errorIcon = wxArtProvider::GetBitmap(wxART_ERROR, wxART_MESSAGE_BOX, wxSize(32, 32));
        wxStaticBitmap* icon = new wxStaticBitmap(this, wxID_ANY, errorIcon);
        topSizer->Add(icon, 0, wxALL | wxALIGN_TOP, 10);

        wxStaticText* message = new wxStaticText(this, wxID_ANY,
            "The program is running without elevated rights!\n\n"
            "To make the program work correctly, open Terminal and run the command:"
        );
        topSizer->Add(message, 1, wxALL | wxALIGN_CENTER_VERTICAL, 10);
        mainSizer->Add(topSizer, 0, wxEXPAND);

        wxTextCtrl* cmdField = new wxTextCtrl(this, wxID_ANY, cmd,
                                              wxDefaultPosition, wxDefaultSize,
                                              wxTE_READONLY);
        mainSizer->Add(cmdField, 0, wxALL | wxEXPAND, 10);

        wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
        wxButton* copyBtn = new wxButton(this, wxID_ANY, "Copy command");
        wxButton* closeBtn = new wxButton(this, wxID_OK, "Close");
        btnSizer->Add(copyBtn, 0, wxALL, 5);
        btnSizer->Add(closeBtn, 0, wxALL, 5);
        mainSizer->Add(btnSizer, 0, wxALIGN_CENTER);

        SetSizerAndFit(mainSizer);

        copyBtn->Bind(wxEVT_BUTTON, [cmd](wxCommandEvent&) {
            if (wxTheClipboard->Open()) {
                wxTheClipboard->SetData(new wxTextDataObject(cmd));
                wxTheClipboard->Close();
            }
        });
    }
};

// --- Класс приложения ---
class MyApp : public wxApp {
public:
    virtual bool OnInit();
};

// --- Главное окно программы ---
class MyFrame : public wxFrame {
public:
    MyFrame(const wxString& title);

private:
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnOpenFile(wxCommandEvent& event);
    void OnSelectDisk(wxCommandEvent& event);
    void OnStartWrite(wxCommandEvent& event);
    void OnUpdate(wxThreadEvent& event);

    void OnSelectDiskBackup(wxCommandEvent& event);
    void OnSaveImage(wxCommandEvent& event);
    void OnStartBackup(wxCommandEvent& event);

    // --- Flash UI ---
    wxTextCtrl* m_filePathText;
    wxTextCtrl* m_diskPathText;
    wxTextCtrl* m_fileSizeText;
    wxTextCtrl* m_diskSizeText;
    wxGauge*    m_progressBar;
    wxTextCtrl* m_logWindow;

    // --- Backup UI ---
    wxTextCtrl* m_backupDiskText;
    wxTextCtrl* m_backupFileText;
    wxGauge*    m_backupProgress;
    wxTextCtrl* m_backupLog;

    // --- Format UI ---
    wxTextCtrl* m_formatDiskText;
    wxTextCtrl* m_formatDiskSizeText;
    wxChoice*   m_fsChoice;
    wxTextCtrl* m_formatLog;

    unsigned long long m_fileSizeBytes = 0;
};

// --- Реализация приложения ---
wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit() {
    if (geteuid() != 0) {
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
        wxString cmd = "sudo \"" + exePath + "\"";
        RootDialog dlg(nullptr, cmd);
        dlg.ShowModal();
        return false;
    }

    MyFrame* frame = new MyFrame("CardMate");
    frame->Show(true);
    return true;
}

// --- Реализация главного окна ---
MyFrame::MyFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(600, 600)) {
    wxMenu* helpMenu = new wxMenu;
    helpMenu->Append(wxID_ABOUT, "&About\tF1");
    wxMenuBar* menuBar = new wxMenuBar;
    menuBar->Append(helpMenu, "&Help");
    SetMenuBar(menuBar);

    wxNotebook* notebook = new wxNotebook(this, wxID_ANY);

    // --- Вкладка Flash ---
    wxPanel* flashPanel = new wxPanel(notebook, wxID_ANY);
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    wxButton* btnOpenFile = new wxButton(flashPanel, 1, "Выбрать файл");
    m_filePathText = new wxTextCtrl(flashPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    m_fileSizeText = new wxTextCtrl(flashPanel, wxID_ANY, "Размер файла: -", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    wxButton* btnSelectDisk = new wxButton(flashPanel, 2, "Выбрать диск");
    m_diskPathText = new wxTextCtrl(flashPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    m_diskSizeText = new wxTextCtrl(flashPanel, wxID_ANY, "Размер диска: -", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    m_progressBar = new wxGauge(flashPanel, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 25));
    wxButton* btnStartWrite = new wxButton(flashPanel, 3, "Записать файл на диск");
    m_logWindow = new wxTextCtrl(flashPanel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 200), wxTE_MULTILINE | wxTE_READONLY);

    vbox->Add(btnOpenFile, 0, wxEXPAND | wxALL, 5);
    vbox->Add(m_filePathText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    vbox->Add(m_fileSizeText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    vbox->Add(btnSelectDisk, 0, wxEXPAND | wxALL, 5);
    vbox->Add(m_diskPathText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    vbox->Add(m_diskSizeText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    vbox->Add(m_progressBar, 0, wxEXPAND | wxALL, 5);
    vbox->Add(btnStartWrite, 0, wxEXPAND | wxALL, 5);
    vbox->Add(new wxStaticText(flashPanel, wxID_ANY, "Лог выполнения:"), 0, wxLEFT | wxTOP, 5);
    vbox->Add(m_logWindow, 1, wxEXPAND | wxALL, 5);
    flashPanel->SetSizer(vbox);

    // --- Вкладка Backup ---
    wxPanel* backupPanel = new wxPanel(notebook, wxID_ANY);
    wxBoxSizer* bSizer = new wxBoxSizer(wxVERTICAL);
    wxButton* btnSelectDiskBackup = new wxButton(backupPanel, 10, "Выбрать диск");
    m_backupDiskText = new wxTextCtrl(backupPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    wxButton* btnSaveImage = new wxButton(backupPanel, 11, "Выбрать файл образа");
    m_backupFileText = new wxTextCtrl(backupPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    m_backupProgress = new wxGauge(backupPanel, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 25));
    wxButton* btnStartBackup = new wxButton(backupPanel, 12, "Создать образ SD карты");
    m_backupLog = new wxTextCtrl(backupPanel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 200), wxTE_MULTILINE | wxTE_READONLY);

    bSizer->Add(btnSelectDiskBackup, 0, wxEXPAND | wxALL, 5);
    bSizer->Add(m_backupDiskText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    bSizer->Add(btnSaveImage, 0, wxEXPAND | wxALL, 5);
    bSizer->Add(m_backupFileText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    bSizer->Add(m_backupProgress, 0, wxEXPAND | wxALL, 5);
    bSizer->Add(btnStartBackup, 0, wxEXPAND | wxALL, 5);
    bSizer->Add(new wxStaticText(backupPanel, wxID_ANY, "Лог выполнения:"), 0, wxLEFT | wxTOP, 5);
    bSizer->Add(m_backupLog, 1, wxEXPAND | wxALL, 5);
    backupPanel->SetSizer(bSizer);

    // --- Вкладка Format ---
wxPanel* formatPanel = new wxPanel(notebook, wxID_ANY);
wxBoxSizer* fSizer = new wxBoxSizer(wxVERTICAL);

wxButton* btnSelectFormatDisk = new wxButton(formatPanel, 20, "Выбрать диск");
m_formatDiskText = new wxTextCtrl(formatPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);

m_formatDiskSizeText = new wxTextCtrl(formatPanel, wxID_ANY, "Размер диска: -", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);


wxStaticText* fsLabel = new wxStaticText(formatPanel, wxID_ANY, "Файловая система:");
wxArrayString fsChoices;
fsChoices.Add("FAT32");
fsChoices.Add("NTFS");
fsChoices.Add("ExFAT");
fsChoices.Add("Mac OS Extended (Journaled)");
fsChoices.Add("APFS");
m_fsChoice = new wxChoice(formatPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, fsChoices);
m_fsChoice->SetSelection(0);

// Новый элемент для имени диска
wxStaticText* nameLabel = new wxStaticText(formatPanel, wxID_ANY, "Имя диска:");
wxTextCtrl* diskNameText = new wxTextCtrl(formatPanel, wxID_ANY, "MYDISK");

wxButton* btnStartFormat = new wxButton(formatPanel, 21, "Форматировать диск");
m_formatLog = new wxTextCtrl(formatPanel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 200),
                             wxTE_MULTILINE | wxTE_READONLY);

fSizer->Add(btnSelectFormatDisk, 0, wxEXPAND | wxALL, 5);
fSizer->Add(m_formatDiskText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
fSizer->Add(m_formatDiskSizeText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
fSizer->Add(fsLabel, 0, wxALL, 5);
fSizer->Add(m_fsChoice, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
fSizer->Add(nameLabel, 0, wxALL, 5);
fSizer->Add(diskNameText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
fSizer->Add(btnStartFormat, 0, wxEXPAND | wxALL, 5);
fSizer->Add(new wxStaticText(formatPanel, wxID_ANY, "Лог выполнения:"), 0, wxLEFT | wxTOP, 5);
fSizer->Add(m_formatLog, 1, wxEXPAND | wxALL, 5);
formatPanel->SetSizer(fSizer);

    notebook->AddPage(flashPanel, "Flash", true);
    notebook->AddPage(backupPanel, "Backup");
    notebook->AddPage(formatPanel, "Format");

    // --- Привязка событий ---
    Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &MyFrame::OnExit, this, wxID_EXIT);
    Bind(wxEVT_BUTTON, &MyFrame::OnOpenFile, this, 1);
    Bind(wxEVT_BUTTON, &MyFrame::OnSelectDisk, this, 2);
    Bind(wxEVT_BUTTON, &MyFrame::OnStartWrite, this, 3);
    Bind(wxEVT_THREAD_UPDATE, &MyFrame::OnUpdate, this);

    Bind(wxEVT_BUTTON, &MyFrame::OnSelectDiskBackup, this, 10);
    Bind(wxEVT_BUTTON, &MyFrame::OnSaveImage, this, 11);
    Bind(wxEVT_BUTTON, &MyFrame::OnStartBackup, this, 12);

    // Format events
    Bind(wxEVT_BUTTON, [=](wxCommandEvent&) {
        wxArrayString output;
        long exitCode = wxExecute("diskutil list", output, wxEXEC_SYNC);
        if (exitCode != 0 || output.IsEmpty()) {
            wxMessageBox("Не удалось получить список дисков!", "Ошибка", wxOK | wxICON_ERROR);
            return;
        }
        wxArrayString disks;
        for (auto& line : output) {
            line.Trim(true).Trim(false);
            if (line.StartsWith("/dev/disk")) disks.Add(line);
        }
        wxSingleChoiceDialog dlg(this, "Выберите диск для форматирования:", "Доступные диски", disks);
        if (dlg.ShowModal() == wxID_OK) {
            wxString choice = dlg.GetStringSelection();
            wxString diskPath = choice.BeforeFirst(' ');
            m_formatDiskText->SetValue(diskPath);

            wxString cmd;
            cmd.Printf("sh -c \"diskutil info %s | awk -F': ' '/Disk Size/ {print $2}' | awk '{print $1}'\"", diskPath);
            wxArrayString info;
            long ret = wxExecute(cmd, info, wxEXEC_SYNC);
            if (ret == 0 && !info.IsEmpty()) m_formatDiskSizeText->SetValue("Disk size: " + info[0] + "GB");
            else m_formatDiskSizeText->SetValue("Не удалось получить размер диска");
        }
    }, 20);

    // --- Обработчик кнопки Format ---
    Bind(wxEVT_BUTTON, [=](wxCommandEvent&) {
        wxString diskPath = m_formatDiskText->GetValue();
        wxString fs = m_fsChoice->GetStringSelection();
        wxString diskName = diskNameText->GetValue().IsEmpty() ? "UNTITLED" : diskNameText->GetValue();

        if (diskPath.IsEmpty()) {
            wxMessageBox("Сначала выберите диск!", "Ошибка", wxOK | wxICON_ERROR);
            return;
        }

        wxString cmd;
        if (fs == "FAT32") cmd.Printf("diskutil eraseDisk FAT32 %s %s", diskName, diskPath);
        else if (fs == "NTFS") cmd.Printf("diskutil eraseDisk ExFAT %s %s", diskName, diskPath);
        else if (fs == "ExFAT") cmd.Printf("diskutil eraseDisk ExFAT %s %s", diskName, diskPath);
        else if (fs == "Mac OS Extended (Journaled)") cmd.Printf("diskutil eraseDisk JHFS+ %s %s", diskName, diskPath);
        else if (fs == "APFS") cmd.Printf("diskutil eraseDisk APFS %s %s", diskName, diskPath);
    

        m_formatLog->AppendText("Выполняется: " + cmd + "\n");

        wxArrayString output;
        long exitCode = wxExecute(cmd, output, wxEXEC_SYNC);
        for (auto& line : output) m_formatLog->AppendText(line + "\n");

        if (exitCode == 0) {
            m_formatLog->AppendText("Форматирование завершено!\n");
            wxMessageBox("Диск успешно отформатирован!", "Готово", wxOK | wxICON_INFORMATION);
        } else {
            m_formatLog->AppendText("Ошибка форматирования!\n");
            wxMessageBox("Произошла ошибка при форматировании диска!", "Ошибка", wxOK | wxICON_ERROR);
        }
    }, 21);
}

// --- Обработчики ---
void MyFrame::OnExit(wxCommandEvent&) { Close(true); }

void MyFrame::OnAbout(wxCommandEvent&) {
    wxMessageBox(wxString("Version: " + std::string(VERSION) + "\nAuthor: Vadim Nikolaev"),
                 "About", wxOK | wxICON_INFORMATION);
}

void MyFrame::OnOpenFile(wxCommandEvent&) {
    wxFileDialog openFileDialog(this, _("Выберите файл"), "", "", "Все файлы (*.*)|*.*",
                                wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_OK) {
        wxString path = openFileDialog.GetPath();
        m_filePathText->SetValue(path);
        wxFileName file(path);
        m_fileSizeBytes = file.GetSize().GetValue();
        m_fileSizeText->SetValue("Размер файла: " +
            wxString::Format("%llu MB", m_fileSizeBytes / 1000 / 1000));
    }
}

void MyFrame::OnSelectDisk(wxCommandEvent&) {
    wxArrayString output;
    long exitCode = wxExecute("diskutil list", output, wxEXEC_SYNC);
    if (exitCode != 0 || output.IsEmpty()) {
        wxMessageBox("Не удалось получить список дисков!", "Ошибка", wxOK | wxICON_ERROR);
        return;
    }
    wxArrayString disks;
    for (auto& line : output) {
        line.Trim(true).Trim(false);
        if (line.StartsWith("/dev/disk")) disks.Add(line);
    }
    wxSingleChoiceDialog dlg(this, "Выберите диск для записи:", "Доступные диски", disks);
    if (dlg.ShowModal() == wxID_OK) {
        wxString choice = dlg.GetStringSelection();
        wxString diskPath = choice.BeforeFirst(' ');
        m_diskPathText->SetValue(diskPath);

        wxString cmd;
        cmd.Printf("sh -c \"diskutil info %s | awk -F': ' '/Disk Size/ {print $2}' | awk '{print $1}'\"", diskPath);
        wxArrayString info;
        long ret = wxExecute(cmd, info, wxEXEC_SYNC);
        if (ret == 0 && !info.IsEmpty()) m_diskSizeText->SetValue("Disk size: " + info[0] + "GB");
        else m_diskSizeText->SetValue("Не удалось получить размер диска");
    }
}

void MyFrame::OnStartWrite(wxCommandEvent&) {
    wxString filePath = m_filePathText->GetValue();
    wxString diskPath = m_diskPathText->GetValue();
    if (filePath.IsEmpty() || diskPath.IsEmpty()) {
        wxMessageBox("Сначала выберите файл и диск!", "Ошибка", wxOK | wxICON_ERROR);
        return;
    }
    m_progressBar->SetValue(0);
    m_logWindow->AppendText("Начата запись файла " + filePath + " на " + diskPath + "\n");
    FlashThread* thread = new FlashThread(this, filePath, diskPath);
    if (thread->Run() != wxTHREAD_NO_ERROR) {
        wxMessageBox("Не удалось запустить поток записи!", "Ошибка", wxOK | wxICON_ERROR);
        delete thread;
    }
}

void MyFrame::OnUpdate(wxThreadEvent& event) {
    int value = event.GetInt();
    wxString log = event.GetString();
    if (!log.IsEmpty()) m_logWindow->AppendText(log);
    if (value >= 0) {
        m_progressBar->SetValue(value);
        if (value == 100) wxMessageBox("Запись успешно завершена!");
    }
}

void MyFrame::OnSelectDiskBackup(wxCommandEvent&) {
    wxArrayString output;
    long exitCode = wxExecute("diskutil list", output, wxEXEC_SYNC);
    if (exitCode != 0 || output.IsEmpty()) {
        wxMessageBox("Не удалось получить список дисков!", "Ошибка", wxOK | wxICON_ERROR);
        return;
    }
    wxArrayString disks;
    for (auto& line : output) {
        line.Trim(true).Trim(false);
        if (line.StartsWith("/dev/disk")) disks.Add(line);
    }
    wxSingleChoiceDialog dlg(this, "Выберите диск для бэкапа:", "Доступные диски", disks);
    if (dlg.ShowModal() == wxID_OK) {
        wxString choice = dlg.GetStringSelection();
        wxString diskPath = choice.BeforeFirst(' ');
        m_backupDiskText->SetValue(diskPath);
    }
}

void MyFrame::OnSaveImage(wxCommandEvent&) {
    wxFileDialog saveDialog(this, _("Сохранить образ"), "", "", "Образ (*.img)|*.img",
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (saveDialog.ShowModal() == wxID_OK) m_backupFileText->SetValue(saveDialog.GetPath());
}

void MyFrame::OnStartBackup(wxCommandEvent&) {
    wxString diskPath = m_backupDiskText->GetValue();
    wxString imgPath = m_backupFileText->GetValue();
    if (diskPath.IsEmpty() || imgPath.IsEmpty()) {
        wxMessageBox("Сначала выберите диск и файл образа!", "Ошибка", wxOK | wxICON_ERROR);
        return;
    }
    m_backupProgress->SetValue(0);
    m_backupLog->AppendText("Начат бэкап диска " + diskPath + " в файл " + imgPath + "\n");
    BackupThread* thread = new BackupThread(this, diskPath, imgPath);
    if (thread->Run() != wxTHREAD_NO_ERROR) {
        wxMessageBox("Не удалось запустить поток бэкапа!", "Ошибка", wxOK | wxICON_ERROR);
        delete thread;
    }
}
