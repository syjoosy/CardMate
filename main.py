import customtkinter as ctk

# Импорт вкладок (каждая отвечает за свой UI)
from tabs.flash_tab import FlashTab
from tabs.backup_tab import BackupTab
from tabs.format_tab import FormatTab
from tabs.settings_tab import SettingsTab
from tabs.about_tab import AboutTab

# Импорт логирования
from utils.logging import log_message, INFO, SUCCESS

# Глобальные настройки внешнего вида
ctk.set_appearance_mode("Dark")       # Тема (Dark / Light / System)
ctk.set_default_color_theme("blue")  # Цвет акцентов


class CardMateApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        # --- НАСТРОЙКА ОКНА ---
        self.title("CardMate")
        self.geometry("650x550")

        # --- ОСНОВНОЙ КОНТЕЙНЕР С ВКЛАДКАМИ ---
        # Пока создаём только структуру, без наполнения
        self.tabview = ctk.CTkTabview(self)
        self.tabview.pack(expand=True, fill="both", padx=10, pady=10)

        # Добавляем вкладки (пустые контейнеры)
        self.tabview.add("Flash")
        self.tabview.add("Backup")
        self.tabview.add("Format")
        self.tabview.add("Settings")
        self.tabview.add("About")

        # --- OVERLAY С ЗАГРУЗКОЙ (ПОВЕРХ ВСЕГО UI) ---
        # Цвет адаптивный: светлый для Light темы, тёмный для Dark
        self.loading_frame = ctk.CTkFrame(self, fg_color=("gray90", "gray10"))
        self.loading_frame.place(relx=0, rely=0, relwidth=1, relheight=1)

        # Центральная "карточка" загрузки
        self.loading_card = ctk.CTkFrame(
            self.loading_frame,
            corner_radius=16
        )
        # Центрируем по экрану
        self.loading_card.place(relx=0.5, rely=0.5, anchor="center")

        # Текст состояния загрузки
        self.loading_label = ctk.CTkLabel(
            self.loading_card,
            text="Initializing CardMate...",
            font=ctk.CTkFont(size=18, weight="bold")
        )
        self.loading_label.pack(padx=20, pady=(20, 40))

        # Прогрессбар
        self.progress = ctk.CTkProgressBar(self.loading_card, width=220)
        self.progress.pack(padx=30, pady=(0, 20))
        self.progress.set(0)  # начальное значение

        # Запускаем инициализацию вкладок после старта UI
        # (чтобы окно успело отрисоваться)
        self.after(100, self.init_tabs_step_by_step)

        # --- ОБЁРТКА ДЛЯ ПЕРЕКЛЮЧЕНИЯ ВКЛАДОК ---
        # Сохраняем оригинальный обработчик
        original_command = self.tabview._segmented_button.cget("command")

        def wrapped_command(tab_name):
            # 1. Сначала выполняем стандартное переключение вкладки
            if original_command:
                original_command(tab_name)

            # 2. Затем добавляем свою логику (логирование)
            self.on_tab_change(tab_name)

        # Подменяем обработчик
        self.tabview._segmented_button.configure(command=wrapped_command)

    # --- ПОШАГОВАЯ ЗАГРУЗКА ВКЛАДОК ---
    def init_tabs_step_by_step(self):
        # Список шагов загрузки (имя вкладки + её класс)
        steps = [
            ("Flash", FlashTab),
            ("Backup", BackupTab),
            ("Format", FormatTab),
            ("Settings", SettingsTab),
            ("About", AboutTab),
        ]

        self.current_step = 0
        self.total_steps = len(steps)

        def load_next():
            # Пока есть незагруженные вкладки
            if self.current_step < self.total_steps:
                name, TabClass = steps[self.current_step]

                # Логируем текущий шаг
                log_message(INFO, f"Loading tab: {name}")

                # Обновляем текст на экране загрузки
                self.loading_label.configure(text=f"Loading {name}...")

                # Создаём UI вкладки (внутри её контейнера)
                TabClass(self.tabview.tab(name))

                # Обновляем прогресс
                self.current_step += 1
                self.progress.set(self.current_step / self.total_steps)

                # Планируем загрузку следующей вкладки
                # через небольшой delay, чтобы UI не зависал
                self.after(200, load_next)
            else:
                # Когда всё загружено
                self.on_loading_complete()

        # Запускаем первый шаг
        load_next()

    # --- ЗАВЕРШЕНИЕ ЗАГРУЗКИ ---
    def on_loading_complete(self):
        log_message(SUCCESS, "App init success!")

        # Удаляем overlay загрузки
        # После этого становится доступен основной UI
        self.loading_frame.destroy()

    # --- ОБРАБОТКА СМЕНЫ ВКЛАДКИ ---
    def on_tab_change(self, tab_name):
        log_message(INFO, "Switched to: " + tab_name)


if __name__ == "__main__":
    # Лог при старте приложения
    log_message(INFO, "App started!")

    # Создание и запуск приложения
    app = CardMateApp()
    app.mainloop()