#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/regex.h>
#include <wx/thread.h>
#include <wx/wx.h>

#include <fstream>
#include <iostream>

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
  MyFrame* frame = new MyFrame("CardMate"); // создаем главное окно
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
  menuBar->Append(menuFile, "&File");
  menuBar->Append(helpMenu, "&About");
  SetMenuBar(menuBar);

  // --- Основная панель и разметка ---
  wxPanel* panel = new wxPanel(this, wxID_ANY);
  wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL); // вертикальный контейнер

  // --- Элементы для выбора файла ---
  wxButton* btnOpenFile = new wxButton(panel, 1, "Выбрать файл");
  m_filePathText = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition,
                                  wxDefaultSize, wxTE_READONLY);
  m_fileSizeText =
      new wxTextCtrl(panel, wxID_ANY, "Размер файла: -", wxDefaultPosition,
                     wxDefaultSize, wxTE_READONLY);

  // --- Элементы для выбора диска ---
  wxButton* btnSelectDisk = new wxButton(panel, 2, "Выбрать диск");
  m_diskPathText = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition,
                                  wxDefaultSize, wxTE_READONLY);
  m_diskSizeText =
      new wxTextCtrl(panel, wxID_ANY, "Размер диска: -", wxDefaultPosition,
                     wxDefaultSize, wxTE_READONLY);

  // --- Индикатор прогресса ---
  m_progressBar =
      new wxGauge(panel, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 25));
  wxButton* btnStartWrite = new wxButton(panel, 3, "Записать файл на диск");

  // --- Окно логов ---
  m_logWindow = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition,
                               wxSize(-1, 200), wxTE_MULTILINE | wxTE_READONLY);

  // --- Добавление элементов в layout ---
  vbox->Add(btnOpenFile, 0, wxEXPAND | wxALL, 5);
  vbox->Add(m_filePathText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
  vbox->Add(m_fileSizeText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

  vbox->Add(btnSelectDisk, 0, wxEXPAND | wxALL, 5);
  vbox->Add(m_diskPathText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
  vbox->Add(m_diskSizeText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

  vbox->Add(m_progressBar, 0, wxEXPAND | wxALL, 5);
  vbox->Add(btnStartWrite, 0, wxEXPAND | wxALL, 5);
  vbox->Add(new wxStaticText(panel, wxID_ANY, "Лог выполнения:"), 0,
            wxLEFT | wxTOP, 5);
  vbox->Add(m_logWindow, 1, wxEXPAND | wxALL, 5);

  panel->SetSizer(vbox);

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

