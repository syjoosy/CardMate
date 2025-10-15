#include "MainFrame.h"
#include "../threads/FlashThread.h"
#include "../threads/BackupThread.h"
#include "../utils/DiskUtils.h"
#include "../utils/Constants.h"

#include <wx/menu.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/dir.h>
#include <wx/stdpaths.h>
#include <wx/notebook.h>
#include <wx/filename.h>
#include <wx/regex.h>
#include <wx/thread.h>
#include <wx/wx.h>
#include <wx/clipbrd.h>
#include <wx/artprov.h>
#include <wx/filename.h>

MainFrame::MainFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(600, 600)) {
    
    // Create menu
    wxMenu* helpMenu = new wxMenu;
    helpMenu->Append(wxID_ABOUT, "&About\tF1");
    wxMenuBar* menuBar = new wxMenuBar;
    menuBar->Append(helpMenu, "&Help");
    SetMenuBar(menuBar);

    // Create notebook
    m_notebook = new wxNotebook(this, wxID_ANY);

    // Create tabs
    wxPanel* flashPanel = new wxPanel(m_notebook, wxID_ANY);
    wxPanel* backupPanel = new wxPanel(m_notebook, wxID_ANY);
    wxPanel* formatPanel = new wxPanel(m_notebook, wxID_ANY);

    CreateFlashTab(flashPanel);
    CreateBackupTab(backupPanel);
    CreateFormatTab(formatPanel);

    m_notebook->AddPage(flashPanel, "Flash", true);
    m_notebook->AddPage(backupPanel, "Backup");
    m_notebook->AddPage(formatPanel, "Format");

    // Bind events
    Bind(wxEVT_MENU, &MainFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &MainFrame::OnExit, this, wxID_EXIT);
    Bind(wxEVT_THREAD_UPDATE, &MainFrame::OnUpdate, this);
}

void MainFrame::CreateFlashTab(wxWindow* parent) {
    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    
    wxButton* btnOpenFile = new wxButton(parent, ID_OPEN_FILE, "Выбрать файл");
    m_filePathText = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    m_fileSizeText = new wxTextCtrl(parent, wxID_ANY, "Размер файла: -", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    
    wxButton* btnSelectDisk = new wxButton(parent, ID_SELECT_DISK, "Выбрать диск");
    m_diskPathText = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    m_diskSizeText = new wxTextCtrl(parent, wxID_ANY, "Размер диска: -", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    
    m_progressBar = new wxGauge(parent, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 25));
    wxButton* btnStartWrite = new wxButton(parent, ID_START_WRITE, "Записать файл на диск");
    m_logWindow = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 200), wxTE_MULTILINE | wxTE_READONLY);

    vbox->Add(btnOpenFile, 0, wxEXPAND | wxALL, 5);
    vbox->Add(m_filePathText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    vbox->Add(m_fileSizeText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    vbox->Add(btnSelectDisk, 0, wxEXPAND | wxALL, 5);
    vbox->Add(m_diskPathText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    vbox->Add(m_diskSizeText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    vbox->Add(m_progressBar, 0, wxEXPAND | wxALL, 5);
    vbox->Add(btnStartWrite, 0, wxEXPAND | wxALL, 5);
    vbox->Add(new wxStaticText(parent, wxID_ANY, "Лог выполнения:"), 0, wxLEFT | wxTOP, 5);
    vbox->Add(m_logWindow, 1, wxEXPAND | wxALL, 5);
    
    parent->SetSizer(vbox);

    // Bind tab-specific events
    btnOpenFile->Bind(wxEVT_BUTTON, &MainFrame::OnOpenFile, this);
    btnSelectDisk->Bind(wxEVT_BUTTON, &MainFrame::OnSelectDisk, this);
    btnStartWrite->Bind(wxEVT_BUTTON, &MainFrame::OnStartWrite, this);
}

void MainFrame::CreateBackupTab(wxWindow* parent) {
    wxBoxSizer* bSizer = new wxBoxSizer(wxVERTICAL);
    
    wxButton* btnSelectDiskBackup = new wxButton(parent, ID_SELECT_DISK_BACKUP, "Выбрать диск");
    m_backupDiskText = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    
    wxButton* btnSaveImage = new wxButton(parent, ID_SAVE_IMAGE, "Выбрать файл образа");
    m_backupFileText = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    
    m_backupProgress = new wxGauge(parent, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 25));
    wxButton* btnStartBackup = new wxButton(parent, ID_START_BACKUP, "Создать образ SD карты");
    m_backupLog = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 200), wxTE_MULTILINE | wxTE_READONLY);

    bSizer->Add(btnSelectDiskBackup, 0, wxEXPAND | wxALL, 5);
    bSizer->Add(m_backupDiskText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    bSizer->Add(btnSaveImage, 0, wxEXPAND | wxALL, 5);
    bSizer->Add(m_backupFileText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    bSizer->Add(m_backupProgress, 0, wxEXPAND | wxALL, 5);
    bSizer->Add(btnStartBackup, 0, wxEXPAND | wxALL, 5);
    bSizer->Add(new wxStaticText(parent, wxID_ANY, "Лог выполнения:"), 0, wxLEFT | wxTOP, 5);
    bSizer->Add(m_backupLog, 1, wxEXPAND | wxALL, 5);
    
    parent->SetSizer(bSizer);

    // Bind tab-specific events
    btnSelectDiskBackup->Bind(wxEVT_BUTTON, &MainFrame::OnSelectDiskBackup, this);
    btnSaveImage->Bind(wxEVT_BUTTON, &MainFrame::OnSaveImage, this);
    btnStartBackup->Bind(wxEVT_BUTTON, &MainFrame::OnStartBackup, this);
}

void MainFrame::CreateFormatTab(wxWindow* parent) {
    wxBoxSizer* fSizer = new wxBoxSizer(wxVERTICAL);

    wxButton* btnSelectFormatDisk = new wxButton(parent, ID_SELECT_FORMAT_DISK, "Выбрать диск");
    m_formatDiskText = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    m_formatDiskSizeText = new wxTextCtrl(parent, wxID_ANY, "Размер диска: -", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);

    wxStaticText* fsLabel = new wxStaticText(parent, wxID_ANY, "Файловая система:");
    wxArrayString fsChoices;
    fsChoices.Add("FAT32");
    fsChoices.Add("NTFS");
    fsChoices.Add("ExFAT");
    fsChoices.Add("Mac OS Extended (Journaled)");
    fsChoices.Add("APFS");
    m_fsChoice = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, fsChoices);
    m_fsChoice->SetSelection(0);

    wxStaticText* nameLabel = new wxStaticText(parent, wxID_ANY, "Имя диска:");
    diskNameText = new wxTextCtrl(parent, wxID_ANY, "MYDISK");

    wxButton* btnStartFormat = new wxButton(parent, ID_START_FORMAT, "Форматировать диск");
    m_formatLog = new wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 200),
                                 wxTE_MULTILINE | wxTE_READONLY);

    fSizer->Add(btnSelectFormatDisk, 0, wxEXPAND | wxALL, 5);
    fSizer->Add(m_formatDiskText, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
    fSizer->Add(m_formatDiskSizeText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    fSizer->Add(fsLabel, 0, wxALL, 5);
    fSizer->Add(m_fsChoice, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    fSizer->Add(nameLabel, 0, wxALL, 5);
    fSizer->Add(diskNameText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    fSizer->Add(btnStartFormat, 0, wxEXPAND | wxALL, 5);
    fSizer->Add(new wxStaticText(parent, wxID_ANY, "Лог выполнения:"), 0, wxLEFT | wxTOP, 5);
    fSizer->Add(m_formatLog, 1, wxEXPAND | wxALL, 5);
    
    parent->SetSizer(fSizer);

    // Bind tab-specific events
    btnSelectFormatDisk->Bind(wxEVT_BUTTON, &MainFrame::OnSelectFormatDisk, this);
    btnStartFormat->Bind(wxEVT_BUTTON, &MainFrame::OnStartFormat, this);
}

// Event handlers implementation
void MainFrame::OnExit(wxCommandEvent&) { Close(true); }

void MainFrame::OnAbout(wxCommandEvent&) {
    wxMessageBox(wxString("Version: " + APP_VERSION + "\nAuthor: Vadim Nikolaev"),
                 "About", wxOK | wxICON_INFORMATION);
}

void MainFrame::OnOpenFile(wxCommandEvent&) {
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

void MainFrame::OnSelectDisk(wxCommandEvent&) {
    ShowDiskSelectionDialog(m_diskPathText, m_diskSizeText, "Выберите диск для записи:");
}

void MainFrame::OnSelectDiskBackup(wxCommandEvent&) {
    ShowDiskSelectionDialog(m_backupDiskText, nullptr, "Выберите диск для бэкапа:");
}

void MainFrame::OnSelectFormatDisk(wxCommandEvent&) {
    ShowDiskSelectionDialog(m_formatDiskText, m_formatDiskSizeText, "Выберите диск для форматирования:");
}

void MainFrame::ShowDiskSelectionDialog(wxTextCtrl* diskText, wxTextCtrl* sizeText, const wxString& title) {
    wxArrayString disks = DiskUtils::GetAvailableDisks();
    if (disks.IsEmpty()) {
        wxMessageBox("Не удалось получить список дисков!", "Ошибка", wxOK | wxICON_ERROR);
        return;
    }
    
    wxSingleChoiceDialog dlg(this, title, "Доступные диски", disks);
    if (dlg.ShowModal() == wxID_OK) {
        wxString choice = dlg.GetStringSelection();
        wxString diskPath = choice.BeforeFirst(' ');
        diskText->SetValue(diskPath);
        
        if (sizeText) {
            wxString diskSize = DiskUtils::GetDiskSize(diskPath);
            sizeText->SetValue(diskSize);
        }
    }
}

void MainFrame::OnStartWrite(wxCommandEvent&) {
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

void MainFrame::OnSaveImage(wxCommandEvent&) {
    wxFileDialog saveDialog(this, _("Сохранить образ"), "", "", "Образ (*.img)|*.img",
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (saveDialog.ShowModal() == wxID_OK) {
        m_backupFileText->SetValue(saveDialog.GetPath());
    }
}

void MainFrame::OnStartBackup(wxCommandEvent&) {
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

void MainFrame::OnStartFormat(wxCommandEvent&) {
    wxString diskPath = m_formatDiskText->GetValue();
    wxString fs = m_fsChoice->GetStringSelection();
    
    // Get disk name from UI (simplified - in real implementation you'd need to add this control)
    wxString diskName = diskNameText->GetValue();

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
    bool success = DiskUtils::ExecuteCommand(cmd, output);
    for (auto& line : output) {
        m_formatLog->AppendText(line + "\n");
    }

    if (success) {
        m_formatLog->AppendText("Форматирование завершено!\n");
        wxMessageBox("Диск успешно отформатирован!", "Готово", wxOK | wxICON_INFORMATION);
    } else {
        m_formatLog->AppendText("Ошибка форматирования!\n");
        wxMessageBox("Произошла ошибка при форматировании диска!", "Ошибка", wxOK | wxICON_ERROR);
    }
}

void MainFrame::OnUpdate(wxThreadEvent& event) {
    int value = event.GetInt();
    wxString log = event.GetString();
    
    // Determine which log to update based on which progress bar is active
    if (m_progressBar->GetValue() < 100 && value <= 100) {
        if (!log.IsEmpty()) m_logWindow->AppendText(log);
        if (value >= 0) {
            m_progressBar->SetValue(value);
            if (value == 100) wxMessageBox("Запись успешно завершена!");
        }
    } else if (m_backupProgress->GetValue() < 100 && value <= 100) {
        if (!log.IsEmpty()) m_backupLog->AppendText(log);
        if (value >= 0) {
            m_backupProgress->SetValue(value);
            if (value == 100) wxMessageBox("Бэкап успешно завершён!");
        }
    }
}