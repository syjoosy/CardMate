#include <wx/wx.h>
#include <wx/process.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/txtstrm.h>
#include <wx/sstream.h>
#include <wx/regex.h>


#define VERSION "v0.1 Dev Build"

// Класс приложения
class MyApp : public wxApp
{
public:
    virtual bool OnInit();
};

// Главное окно
class MyFrame : public wxFrame
{
public:
    MyFrame(const wxString& title);

private:
    void OnHello(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnOpenFile(wxCommandEvent& event);
    void OnSelectDisk(wxCommandEvent& event);
    void OnStartDD(wxCommandEvent& event);
    void OnDDOutput(wxProcessEvent& event);

    wxTextCtrl* m_filePathText;   // путь к файлу
    wxTextCtrl* m_diskPathText;   // путь к диску
    wxTextCtrl* m_fileSizeText;   // размер файла
    wxTextCtrl* m_diskSizeText;   // размер диска
    wxGauge* m_progressBar;       // прогресс бар
    wxTextCtrl* m_logWindow;      // лог сообщений

    wxProcess* m_process = nullptr; 
    unsigned long long m_fileSizeBytes = 0; // размер файла
};

// Реализация приложения
wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    MyFrame* frame = new MyFrame("CardMate");
    wxLog::SetActiveTarget(new wxLogStderr());

    frame->Show(true);
    return true;
}

// Конструктор
MyFrame::MyFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(600, 500))
{
    wxMenu* menuFile = new wxMenu;
    menuFile->Append(1, "&Hello..\tCtrl-H", "Показать сообщение");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);

    wxMenu* helpMenu = new wxMenu;
    helpMenu->Append(2, "&About\tF1");

    wxMenuBar* menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(helpMenu, "&About");
    SetMenuBar(menuBar);

    wxPanel* panel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);

    wxButton* btnOpenFile = new wxButton(panel, 3, "Выбрать файл");
    m_filePathText = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    m_fileSizeText = new wxTextCtrl(panel, wxID_ANY, "Размер файла: -", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);

    wxButton* btnSelectDisk = new wxButton(panel, 4, "Выбрать диск");
    m_diskPathText = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    m_diskSizeText = new wxTextCtrl(panel, wxID_ANY, "Размер диска: -", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);

    m_progressBar = new wxGauge(panel, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 25));
    wxButton* btnStartDD = new wxButton(panel, 5, "Записать файл на диск (dd)");

    m_logWindow = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 200),
                                 wxTE_MULTILINE | wxTE_READONLY);

    vbox->Add(btnOpenFile, 0, wxEXPAND | wxALL, 5);
    vbox->Add(m_filePathText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    vbox->Add(m_fileSizeText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

    vbox->Add(btnSelectDisk, 0, wxEXPAND | wxALL, 5);
    vbox->Add(m_diskPathText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    vbox->Add(m_diskSizeText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

    vbox->Add(m_progressBar, 0, wxEXPAND | wxALL, 5);
    vbox->Add(btnStartDD, 0, wxEXPAND | wxALL, 5);
    vbox->Add(new wxStaticText(panel, wxID_ANY, "Лог выполнения:"), 0, wxLEFT | wxTOP, 5);
    vbox->Add(m_logWindow, 1, wxEXPAND | wxALL, 5);

    panel->SetSizer(vbox);

    Bind(wxEVT_MENU, &MyFrame::OnHello, this, 1);
    Bind(wxEVT_MENU, &MyFrame::OnAbout, this, 2);
    Bind(wxEVT_MENU, &MyFrame::OnExit, this, wxID_EXIT);
    Bind(wxEVT_BUTTON, &MyFrame::OnOpenFile, this, 3);
    Bind(wxEVT_BUTTON, &MyFrame::OnSelectDisk, this, 4);
    Bind(wxEVT_BUTTON, &MyFrame::OnStartDD, this, 5);
    Bind(wxEVT_END_PROCESS, &MyFrame::OnDDOutput, this);
}

void MyFrame::OnExit(wxCommandEvent& event) { Close(true); }

void MyFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageBox(wxString("Version: " + std::string(VERSION) + "\nAuthor: Vadim Nikolaev"),
                 "About", wxOK | wxICON_INFORMATION);
}

void MyFrame::OnHello(wxCommandEvent& event)
{
    wxMessageBox("Привет из wxWidgets!", "Hello", wxOK | wxICON_INFORMATION);
}

void MyFrame::OnOpenFile(wxCommandEvent& event)
{
    wxFileDialog openFileDialog(this, _("Выберите файл"), "", "",
                                "Все файлы (*.*)|*.*",
                                wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_OK)
    {
        wxString path = openFileDialog.GetPath();
        m_filePathText->SetValue(path);

        wxFileName file(path);
        m_fileSizeBytes = file.GetSize().GetValue();
        m_fileSizeText->SetValue("Размер файла: " + wxString::Format("%llu Mbyte", m_fileSizeBytes / 1000 / 1000));
    }
}

void MyFrame::OnSelectDisk(wxCommandEvent& event)
{
    wxLogMessage("Start disk select!");

    wxArrayString output;
    long exitCode = wxExecute("diskutil list", output, wxEXEC_SYNC);

    if (exitCode != 0 || output.IsEmpty())
    {
        wxMessageBox("Не удалось получить список дисков!", "Ошибка", wxOK | wxICON_ERROR);
        return;
    }

    // Парсим вывод diskutil
    wxArrayString disks;
    for (auto& line : output)
    {
        line.Trim(true).Trim(false);
        if (line.StartsWith("/dev/disk"))
        {
            disks.Add(line);
        }
    }

    if (disks.IsEmpty())
    {
        wxMessageBox("Диски не найдены!", "Ошибка", wxOK | wxICON_ERROR);
        return;
    }

    // Показываем выбор
    wxSingleChoiceDialog dlg(this,
                             "Выберите диск для записи:",
                             "Доступные диски",
                             disks);

                        

    if (dlg.ShowModal() == wxID_OK)
    {
        wxString choice = dlg.GetStringSelection();
        // Из строки "/dev/diskX ..." берём только путь
        wxString diskPath = choice.BeforeFirst(' ');
        m_diskPathText->SetValue(diskPath);


        // Формируем команду с подстановкой выбранного диска
        wxString cmd;
        cmd.Printf("sh -c \"diskutil info %s | awk -F': ' '/Disk Size/ {print $2}' | awk '{print $1}'\"",
                diskPath);



        wxLogMessage(cmd);

        // Выполняем команду и читаем вывод
        wxArrayString output;
        long exitCode = wxExecute(cmd, output, wxEXEC_SYNC);

        if (exitCode == 0 && !output.IsEmpty())
        {
            wxString sizeBytes = output[0];
            m_diskSizeText->SetValue("Disk size: " + sizeBytes + "GB");
            wxLogMessage("OK");
        }
        else
        {
            m_diskSizeText->SetValue("Не удалось получить размер диска");
            wxLogMessage("ERROR");
        }
    }
}


void MyFrame::OnStartDD(wxCommandEvent& event)
{
    wxString filePath = m_filePathText->GetValue();
    wxString diskPath = m_diskPathText->GetValue();

    if (filePath.IsEmpty() || diskPath.IsEmpty())
    {
        wxMessageBox("Сначала выберите файл и диск!", "Ошибка", wxOK | wxICON_ERROR);
        return;
    }

    wxString command2 = "diskutil unmountDisk " + diskPath;

    m_process = new wxProcess(this);
    m_process->Redirect();

    long pid2 = wxExecute(command2, wxEXEC_ASYNC, m_process);

    wxString command = "dd if=" + filePath + " of=" + diskPath + " bs=4M status=progress";

    m_logWindow->AppendText("Запуск: " + command + "\n");

    m_process = new wxProcess(this);
    m_process->Redirect();

    wxMessageBox("Команда для записи:\n" + command,
                         "DD Command", wxOK | wxICON_INFORMATION);

    long pid = wxExecute(command, wxEXEC_ASYNC, m_process);


    if (pid == 0)
    {
        m_logWindow->AppendText("Ошибка запуска dd\n");
        delete m_process;
        m_process = nullptr;
    }
}

void MyFrame::OnDDOutput(wxProcessEvent& event)
{
    if (!m_process) return;

    wxInputStream* err = m_process->GetErrorStream();
    if (err && err->CanRead())
    {
        wxString msg;
        wxStringOutputStream sstream(&msg);
        err->Read(sstream);

        m_logWindow->AppendText(msg);

        // Парсим прогресс
        wxRegEx re("([0-9]+) bytes");
        if (re.Matches(msg))
        {
            wxString bytesStr = re.GetMatch(msg, 1);
            unsigned long long copied = 0;
            bytesStr.ToULongLong(&copied);

            if (m_fileSizeBytes > 0)
            {
                int percent = (int)((copied * 100) / m_fileSizeBytes);
                if (percent > 100) percent = 100;
                m_progressBar->SetValue(percent);
            }
        }
    }

    if (event.GetExitCode() != -1)
    {
        m_logWindow->AppendText("\nЗавершено.\n");
        m_progressBar->SetValue(100);
        delete m_process;
        m_process = nullptr;
    }
}
