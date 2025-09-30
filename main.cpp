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

#define VERSION "v0.1 Dev Build"

// --- Определение событий для взаимодействия с потоком ---
wxDECLARE_EVENT(wxEVT_THREAD_UPDATE, wxThreadEvent); // объявление кастомного события
wxDEFINE_EVENT(wxEVT_THREAD_UPDATE, wxThreadEvent);  // его определение

// --- Класс для фоновой записи файла на диск ---
class FlashThread : public wxThread {
 public:
  // Конструктор: принимает родительское окно, путь к файлу-источнику и путь к диску-приёмнику
  FlashThread(wxWindow* parent, const wxString& srcFile,
              const wxString& dstFile)
      : wxThread(wxTHREAD_DETACHED), // поток будет автоматически уничтожен по завершении
        m_parent(parent),
        m_srcFile(srcFile),
        m_dstFile(dstFile) {}

 protected:
// Основная функция потока (Entry) — выполняет запись
  virtual ExitCode Entry() override {
    const size_t buf_size = 4 * 1024 * 1024;  // размер буфера = 4 МБ
    char* buffer = new char[buf_size];        // выделяем буфер под данные

    // Открываем исходный файл для чтения
    std::ifstream src(m_srcFile.mb_str(), std::ios::binary);
    if (!src) {
      SendLog("Ошибка: не удалось открыть файл образа!");
      delete[] buffer;
      return (ExitCode)1;
    }

    // Открываем устройство/файл для записи
    std::ofstream dst(m_dstFile.mb_str(), std::ios::binary);
    if (!dst) {
      SendLog("Ошибка: не удалось открыть устройство для записи!");
      wxMessageBox("Не удалось открыть устройство для записи!", "Ошибка",
                 wxOK | wxICON_ERROR);
      delete[] buffer;
      return (ExitCode)1;
    }

    // Получаем размер исходного файла
    src.seekg(0, std::ios::end);
    size_t file_size = src.tellg();
    src.seekg(0, std::ios::beg);

    size_t total_bytes = 0; // счетчик записанных байтов
    while (src) {
      // читаем из файла кусками
      src.read(buffer, buf_size);
      std::streamsize read_bytes = src.gcount();
      if (read_bytes == 0) break; // если больше нечего читать — выходим

      // пишем прочитанное в целевой файл
      dst.write(buffer, read_bytes);
      total_bytes += read_bytes;

      // вычисляем процент прогресса
      int percent = int((double)total_bytes / file_size * 100);

      // формируем сообщение о прогрессе
      wxString msg = wxString::Format("[PROGRESS] %d%% (%zu/%zu  mbytes)\n",
                                      percent, total_bytes / 1000 / 1000, file_size / 1000 / 1000);

      SendUpdate(percent, msg); // отправляем в GUI
    }

    delete[] buffer;

    SendUpdate(100, "Запись завершена!\n"); // уведомляем о завершении

    return (ExitCode)0;
  }

 private:
  wxWindow* m_parent;  // указатель на родительское окно
  wxString m_srcFile;  // путь к исходному файлу
  wxString m_dstFile;  // путь к устройству назначения

  // Метод отправки прогресса в главный поток
  void SendUpdate(int percent, const wxString& msg) {
    wxThreadEvent event(wxEVT_THREAD_UPDATE);
    event.SetInt(percent);   // % выполнения
    event.SetString(msg);    // текстовое сообщение
    wxQueueEvent(m_parent, event.Clone()); // добавляем событие в очередь главного потока
  }

  // Метод отправки логов (без прогресса)
  void SendLog(const wxString& msg) {
    wxThreadEvent event(wxEVT_THREAD_UPDATE);
    event.SetInt(-1); // спец. значение: только лог
    event.SetString(msg + "\n");
    wxQueueEvent(m_parent, event.Clone());
  }
};

class RootDialog : public wxDialog {
public:
    RootDialog(wxWindow* parent, const wxString& cmd)
        : wxDialog(parent, wxID_ANY, "Error: Administrator rights are required", wxDefaultPosition, wxSize(500, 220))
    {
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        // Горизонтальный блок для иконки и текста
        wxBoxSizer* topSizer = new wxBoxSizer(wxHORIZONTAL);

        // Стандартная иконка ошибки
        wxBitmap errorIcon = wxArtProvider::GetBitmap(wxART_ERROR, wxART_MESSAGE_BOX, wxSize(32, 32));
        wxStaticBitmap* icon = new wxStaticBitmap(this, wxID_ANY, errorIcon);
        topSizer->Add(icon, 0, wxALL | wxALIGN_TOP, 10);

        wxStaticText* message = new wxStaticText(this, wxID_ANY,
            "The program is running without elevated rights!\n\n"
            "To make the program work correctly, open Terminal and run the command:"
        );
        topSizer->Add(message, 1, wxALL | wxALIGN_CENTER_VERTICAL, 10);

        mainSizer->Add(topSizer, 0, wxEXPAND);

        // Текстовое поле с командой (только для чтения)
        wxTextCtrl* cmdField = new wxTextCtrl(this, wxID_ANY, cmd,
                                              wxDefaultPosition, wxDefaultSize,
                                              wxTE_READONLY);
        mainSizer->Add(cmdField, 0, wxALL | wxEXPAND, 10);

        // Кнопки
        wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);

        wxButton* copyBtn = new wxButton(this, wxID_ANY, "Copy command");
        wxButton* closeBtn = new wxButton(this, wxID_OK, "Close");

        btnSizer->Add(copyBtn, 0, wxALL, 5);
        btnSizer->Add(closeBtn, 0, wxALL, 5);

        mainSizer->Add(btnSizer, 0, wxALIGN_CENTER);

        SetSizerAndFit(mainSizer);

        // Обработчик кнопки "Скопировать"
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
  virtual bool OnInit(); // точка входа в приложение
};

// --- Главное окно программы ---
class MyFrame : public wxFrame {
 public:
  MyFrame(const wxString& title);

 private:
  // Обработчики событий
  void OnExit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);
  void OnOpenFile(wxCommandEvent& event);
  void OnSelectDisk(wxCommandEvent& event);
  void OnStartWrite(wxCommandEvent& event);
  void OnUpdate(wxThreadEvent& event);

  // Элементы интерфейса
  wxTextCtrl* m_filePathText;   // путь к выбранному файлу
  wxTextCtrl* m_diskPathText;   // путь к выбранному диску
  wxTextCtrl* m_fileSizeText;   // размер выбранного файла
  wxTextCtrl* m_diskSizeText;   // размер выбранного диска
  wxGauge* m_progressBar;       // индикатор прогресса
  wxTextCtrl* m_logWindow;      // окно логов

  unsigned long long m_fileSizeBytes = 0; // размер файла в байтах
};

// --- Реализация приложения ---
wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit() {
    if (geteuid() != 0) {
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();

         // Формируем команду, которую пользователь должен выполнить в терминале
        wxString cmd = "sudo \"" + exePath + "\"";

        // // Показываем сообщение
        // wxMessageBox(
        //     "Программа запущена без повышенных прав!\n\n"
        //     "Чтобы программа работала корректно, откройте Terminal и выполните команду:\n\n" +
        //     cmd,
        //     "Ошибка: Требуются права администратора",
        //     wxOK | wxICON_ERROR
        // );

        // Создаём и показываем диалог
        RootDialog dlg(nullptr, cmd);
        dlg.ShowModal();

        // // Преобразуем wxString в const char* через UTF-8
        // const char* exeCStr = exePath.utf8_str();

        // // Формируем аргументы для exec
        // std::vector<char*> args;
        // args.push_back(const_cast<char*>("sudo"));    // команда sudo
        // args.push_back(const_cast<char*>(exeCStr));   // путь к текущему исполняемому файлу

        // // Добавляем все аргументы, переданные программе
        // int argc = wxApp::argc;
        // char** argv = wxApp::argv;
        // for (int i = 1; i < argc; i++) {
        //     args.push_back(argv[i]);
        // }
        // args.push_back(nullptr); // завершающий nullptr

        // // Перезапуск через sudo
        // execvp("sudo", args.data());

        // // Если execvp вернулся — ошибка
        // wxMessageBox("Не удалось перезапустить программу с правами администратора!",
        //              "Ошибка", wxOK | wxICON_ERROR);
        return false;
    }

    // Если программа запущена с root — создаём главное окно
    MyFrame* frame = new MyFrame("CardMate");
    frame->Show(true);

    return true;
}

// --- Реализация главного окна ---
MyFrame::MyFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(600, 500)) {
  // --- Меню ---
  wxMenu* menuFile = new wxMenu;
  menuFile->Append(wxID_EXIT); // пункт выхода

  wxMenu* helpMenu = new wxMenu;
  helpMenu->Append(wxID_ABOUT); // пункт "О программе"

  wxMenuBar* menuBar = new wxMenuBar;
  // menuBar->Append(menuFile, "&File");
  
  menuBar->Append(helpMenu, "&Help");
  helpMenu->Append(wxID_ABOUT, "&About\tF1");
  SetMenuBar(menuBar);

  // --- Notebook с вкладками ---
  wxNotebook* notebook = new wxNotebook(this, wxID_ANY);

  // --- Вкладка Flash ---
  wxPanel* flashPanel = new wxPanel(notebook, wxID_ANY);
  wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL); // вертикальный контейнер

  // --- Элементы для выбора файла ---
  wxButton* btnOpenFile = new wxButton(flashPanel, 1, "Выбрать файл");
  m_filePathText = new wxTextCtrl(flashPanel, wxID_ANY, "", wxDefaultPosition,
                                  wxDefaultSize, wxTE_READONLY);
  m_fileSizeText =
      new wxTextCtrl(flashPanel, wxID_ANY, "Размер файла: -", wxDefaultPosition,
                     wxDefaultSize, wxTE_READONLY);

  // --- Элементы для выбора диска ---
  wxButton* btnSelectDisk = new wxButton(flashPanel, 2, "Выбрать диск");
  m_diskPathText = new wxTextCtrl(flashPanel, wxID_ANY, "", wxDefaultPosition,
                                  wxDefaultSize, wxTE_READONLY);
  m_diskSizeText =
      new wxTextCtrl(flashPanel, wxID_ANY, "Размер диска: -", wxDefaultPosition,
                     wxDefaultSize, wxTE_READONLY);

  // --- Индикатор прогресса ---
  m_progressBar =
      new wxGauge(flashPanel, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 25));
  wxButton* btnStartWrite = new wxButton(flashPanel, 3, "Записать файл на диск");

  // --- Окно логов ---
  m_logWindow = new wxTextCtrl(flashPanel, wxID_ANY, "", wxDefaultPosition,
                               wxSize(-1, 200), wxTE_MULTILINE | wxTE_READONLY);

  // --- Добавление элементов во вкладку Flash ---
  vbox->Add(btnOpenFile, 0, wxEXPAND | wxALL, 5);
  vbox->Add(m_filePathText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
  vbox->Add(m_fileSizeText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

  vbox->Add(btnSelectDisk, 0, wxEXPAND | wxALL, 5);
  vbox->Add(m_diskPathText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
  vbox->Add(m_diskSizeText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

  vbox->Add(m_progressBar, 0, wxEXPAND | wxALL, 5);
  vbox->Add(btnStartWrite, 0, wxEXPAND | wxALL, 5);
  vbox->Add(new wxStaticText(flashPanel, wxID_ANY, "Лог выполнения:"), 0,
            wxLEFT | wxTOP, 5);
  vbox->Add(m_logWindow, 1, wxEXPAND | wxALL, 5);

  flashPanel->SetSizer(vbox);

  // --- Вкладка Backup ---
  wxPanel* backupPanel = new wxPanel(notebook, wxID_ANY);
  wxBoxSizer* bSizer = new wxBoxSizer(wxVERTICAL);
  bSizer->Add(new wxStaticText(backupPanel, wxID_ANY, "Soon"),
              1, wxALIGN_CENTER | wxALL, 20);
  backupPanel->SetSizer(bSizer);

  // --- Вкладка Format ---
  wxPanel* formatPanel = new wxPanel(notebook, wxID_ANY);
  wxBoxSizer* fSizer = new wxBoxSizer(wxVERTICAL);
  fSizer->Add(new wxStaticText(formatPanel, wxID_ANY, "Soon"),
              1, wxALIGN_CENTER | wxALL, 20);
  formatPanel->SetSizer(fSizer);

  // --- Добавление вкладок в notebook ---
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
}


// --- Обработчики событий ---
void MyFrame::OnExit(wxCommandEvent&) { Close(true); }

void MyFrame::OnAbout(wxCommandEvent&) {
  wxMessageBox(
      wxString("Version: " + std::string(VERSION) + "\nAuthor: Vadim Nikolaev"),
      "About", wxOK | wxICON_INFORMATION);
}

// Выбор файла для записи
void MyFrame::OnOpenFile(wxCommandEvent&) {
  wxFileDialog openFileDialog(this, _("Выберите файл"), "", "",
                              "Все файлы (*.*)|*.*",
                              wxFD_OPEN | wxFD_FILE_MUST_EXIST);

  if (openFileDialog.ShowModal() == wxID_OK) {
    wxString path = openFileDialog.GetPath();
    m_filePathText->SetValue(path);

    wxFileName file(path);
    m_fileSizeBytes = file.GetSize().GetValue();
    m_fileSizeText->SetValue(
        "Размер файла: " +
        wxString::Format("%llu MB", m_fileSizeBytes / 1000 / 1000));
  }
}

// Выбор диска для записи (через diskutil, только macOS)
void MyFrame::OnSelectDisk(wxCommandEvent&) {
  wxArrayString output;
  long exitCode = wxExecute("diskutil list", output, wxEXEC_SYNC);

  if (exitCode != 0 || output.IsEmpty()) {
    wxMessageBox("Не удалось получить список дисков!", "Ошибка",
                 wxOK | wxICON_ERROR);
    return;
  }

  wxArrayString disks;
  for (auto& line : output) {
    line.Trim(true).Trim(false);
    if (line.StartsWith("/dev/disk")) {
      disks.Add(line);
    }
  }

  if (disks.IsEmpty()) {
    wxMessageBox("Диски не найдены!", "Ошибка", wxOK | wxICON_ERROR);
    return;
  }

  wxSingleChoiceDialog dlg(this, "Выберите диск для записи:", "Доступные диски",
                           disks);

  if (dlg.ShowModal() == wxID_OK) {
    wxString choice = dlg.GetStringSelection();
    wxString diskPath = choice.BeforeFirst(' ');
    m_diskPathText->SetValue(diskPath);

    // получаем размер выбранного диска через diskutil info
    wxString cmd;
    cmd.Printf(
        "sh -c \"diskutil info %s | awk -F': ' '/Disk Size/ {print $2}' | awk "
        "'{print $1}'\"",
        diskPath);

    wxArrayString output;
    long exitCode = wxExecute(cmd, output, wxEXEC_SYNC);

    if (exitCode == 0 && !output.IsEmpty()) {
      wxString sizeBytes = output[0];
      m_diskSizeText->SetValue("Disk size: " + sizeBytes + "GB");
    } else {
      m_diskSizeText->SetValue("Не удалось получить размер диска");
    }
  }
}

// Запуск процесса записи
void MyFrame::OnStartWrite(wxCommandEvent&) {
  wxString filePath = m_filePathText->GetValue();
  wxString diskPath = m_diskPathText->GetValue();

  if (filePath.IsEmpty() || diskPath.IsEmpty()) {
    wxMessageBox("Сначала выберите файл и диск!", "Ошибка",
                 wxOK | wxICON_ERROR);
    return;
  }

  m_progressBar->SetValue(0);
  m_logWindow->AppendText("Начата запись файла " + filePath + " на " +
                          diskPath + "\n");

  // создаём и запускаем поток записи
  FlashThread* thread = new FlashThread(this, filePath, diskPath);
  if (thread->Run() != wxTHREAD_NO_ERROR) {
    wxMessageBox("Не удалось запустить поток записи!", "Ошибка",
                 wxOK | wxICON_ERROR);
    delete thread;
  }
}

// Обновление прогресса и логов из фонового потока
void MyFrame::OnUpdate(wxThreadEvent& event) {
  int value = event.GetInt();
  wxString log = event.GetString();

  if (!log.IsEmpty()) {
    m_logWindow->AppendText(log);
  }

  if (value >= 0) {  // -1 значит только лог
    m_progressBar->SetValue(value);

    if (value == 100) {
      wxMessageBox("Запись успешно завершена!");
    }
  }
}

