import customtkinter as ctk

# Импорт классов вкладок (каждая отвечает за свою UI-логику)
from tabs.flash_tab import FlashTab
from tabs.backup_tab import BackupTab
from tabs.format_tab import FormatTab
from tabs.about_tab import AboutTab

# Импорт системы логирования
from utils.logging import log_message, DEBUG, INFO, WARNING, ERROR, CRITICAL, SUCCESS

# Глобальные настройки внешнего вида приложения
ctk.set_appearance_mode("Dark")        # Тёмная тема
ctk.set_default_color_theme("blue")   # Синий акцентный цвет

class CardMateApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        # Заголовок и размер окна
        self.title("CardMate")
        self.geometry("650x550")

        # Создаём контейнер с вкладками
        self.tabview = ctk.CTkTabview(self)
        self.tabview.pack(expand=True, fill="both", padx=10, pady=10)

        # Добавляем вкладки
        self.tabview.add("Flash")
        self.tabview.add("Backup")
        self.tabview.add("Format")
        self.tabview.add("About")

        # Инициализируем содержимое каждой вкладки
        # В каждую вкладку передаётся её frame (контейнер)
        FlashTab(self.tabview.tab("Flash"))
        BackupTab(self.tabview.tab("Backup"))
        FormatTab(self.tabview.tab("Format"))
        AboutTab(self.tabview.tab("About"))

        # Получаем оригинальный обработчик кнопки вкладок
        # (он отвечает за реальное переключение UI)
        original_command = self.tabview._segmented_button.cget("command")

        # Создаём обёртку над стандартным поведением
        def wrapped_command(tab_name):
            # 1. Сначала вызываем стандартную логику переключения вкладки
            if original_command:
                original_command(tab_name)

            # 2. Затем выполняем свою логику (логирование, обновления и т.д.)
            self.on_tab_change(tab_name)

        # Подменяем обработчик на наш (но с сохранением оригинального поведения)
        self.tabview._segmented_button.configure(command=wrapped_command)

        # Лог об успешной инициализации
        log_message(SUCCESS, "App init success!")

    # Обработчик смены вкладки
    def on_tab_change(self, tab_name):
        # Вызывается каждый раз при переключении вкладки
        log_message(INFO, "Switched to: " + tab_name)

if __name__ == "__main__":
    # Лог при запуске приложения
    log_message(INFO, "App started!")

    # Создаём и запускаем приложение
    app = CardMateApp()
    app.mainloop()