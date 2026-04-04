import customtkinter as ctk
import subprocess
import threading
import os
import sys
import shlex

# Настройка внешнего вида (Тема: Dark, Цвет: Blue)
ctk.set_appearance_mode("Dark")
ctk.set_default_color_theme("blue")


class DiskClonerApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        # Настройка окна
        self.title("CardMate v0.1 Dev Build")
        self.geometry("620x400")
        self.resizable(False, False)

        # Переменные
        self.process = None
        self.is_processing = False

        # === ЛЕЙАУТ ===

        # Заголовок
        self.title_label = ctk.CTkLabel(
            self, text="Создание образа диска", font=ctk.CTkFont(size=18, weight="bold")
        )
        self.title_label.pack(pady=20)

        # 1. Выбор диска (Источник)
        self.source_frame = ctk.CTkFrame(self)
        self.source_frame.pack(padx=20, fill="x")

        self.source_label = ctk.CTkLabel(
            self.source_frame, text="Устройство для копирования (SOURCE):"
        )
        self.source_label.grid(row=0, column=0, padx=10, pady=5, sticky="w")

        # Добавим список популярных устройств для удобства, но разрешим вводить свои
        self.device_vars = [
            "/dev/sda",
            "/dev/sdb",
            "/dev/sdc",
            "/dev/nvme0n1",
            "/dev/mmcblk0",
        ]
        self.source_combo = ctk.CTkComboBox(
            self.source_frame, values=self.device_vars, width=300
        )
        self.source_combo.grid(row=0, column=1, padx=10, pady=5, sticky="w")

        self.note_source = ctk.CTkLabel(
            self.source_frame,
            text="* Рекомендуется /dev/sdX (не разделы!)",
            font=ctk.CTkFont(size=10),
            text_color="gray",
        )
        self.note_source.grid(row=1, column=1, padx=10, pady=2, sticky="w")

        # 2. Выбор файла (Цель)
        self.dest_frame = ctk.CTkFrame(self)
        self.dest_frame.pack(padx=20, fill="x")

        self.dest_label = ctk.CTkLabel(self.dest_frame, text="Путь к файлу (DEST):")
        self.dest_label.grid(row=0, column=0, padx=10, pady=5, sticky="w")

        self.dest_entry = ctk.CTkEntry(
            self.dest_frame, placeholder_text="/home/user/image.img", width=300
        )
        self.dest_entry.grid(row=0, column=1, padx=10, pady=5, sticky="ew")

        # Кнопки управления
        self.control_frame = ctk.CTkFrame(self)
        self.control_frame.pack(pady=20)

        self.btn_copy = ctk.CTkButton(
            self.control_frame,
            text="НАЧАТЬ КОПИРОВАНИЕ",
            command=self.start_process,
            width=200,
            height=35,
        )
        self.btn_copy.pack(side="left", padx=10)

        self.btn_stop = ctk.CTkButton(
            self.control_frame,
            text="ОСТАНОВИТЬ",
            command=self.stop_process,
            width=200,
            height=35,
            fg_color="red",
            hover_color="darkred",
        )
        self.btn_stop.pack(side="left", padx=10)

        self.btn_stop.configure(state="disabled")

        # Лог / Прогресс
        self.log_label = ctk.CTkLabel(
            self, text="Статус: Готов к работе", text_color="green"
        )
        self.log_label.pack(pady=10)

        self.log_text = ctk.CTkTextbox(self, width=450, height=120)
        self.log_text.pack(padx=20, pady=10)
        self.log_text.configure(state="disabled")  # Только для чтения

        # Автопрокрутка лога
        self.log_text.bind("<Key>", lambda e: "break")

    def log(self, message, color="white"):
        """Функция для безопасной записи в лог из потока"""
        self.log_text.configure(state="normal")
        self.log_text.insert("end", message + "\n")
        self.log_text.see("end")
        self.log_text.configure(state="disabled")

    def log_status(self, message, color="white"):
        """Обновление статуса сверху"""
        self.log_label.configure(text=message, text_color=color)

    def run_dd(self):
        source = self.source_combo.get().strip()
        dest = self.dest_entry.get().strip()

        if not source or not dest:
            self.log("Ошибка: Заполните оба поля!")
            self.log_status("Ошибка: Пустые поля", "red")
            return

        # Проверяем, есть ли расширение у файла, добавляем .img если нет
        if not dest.endswith(".img"):
            dest += ".img"
            self.dest_entry.delete(0, "end")
            self.dest_entry.insert(0, dest)

        # Проверка на права (обычно dd требует root для чтения /dev/sdX)
        # Выводим предупреждение, но позволяем запустить
        if not os.geteuid() == 0:
            self.log(
                "Внимание: Вы запускаете без sudo. Это может не сработать при чтении дисков."
            )

        # Формируем команду
        # bs=4M - быстрый блок чтения
        # status=progress - вывод прогресса в stderr
        cmd = [
            "dd",
            f"if={source}",
            f"of={dest}",
            "status=progress",
            "conv=noerror,sync",  # Игнорировать ошибки чтения, чтобы не прерываться
        ]

        try:
            # Запускаем процесс
            self.process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,  # Объединяем вывод в один поток
                bufsize=1,
                text=True,
            )

            # Читаем вывод пока процесс идет
            while True:
                if self.process.poll() is not None:
                    # Процесс завершен
                    status, _ = self.process.communicate()
                    self.log_status("Копирование завершено!", "green")
                    self.is_processing = False
                    self.btn_copy.configure(state="normal")
                    self.btn_stop.configure(state="disabled")
                    self.log("Процесс завершен.")
                    break

                # Читаем строку
                line = self.process.stdout.readline()
                if line:
                    # Очищаем возможные символы возврата carriage return
                    line = line.replace("\r", "")
                    if line.strip():
                        self.log(f"[LOG] {line}")

                # Периодически обновляем статус UI (чтобы не повис)
                self.update()

        except PermissionError:
            self.log_status("ОШИБКА: Нет прав доступа к диску.", "red")
            self.log(
                "Ошибка: Убедитесь, что вы запустили скрипт через sudo (sudo python3 script.py)"
            )
        except FileNotFoundError:
            self.log_status("ОШИБКА: Утилита 'dd' не найдена.", "red")

    def start_process(self):
        if self.is_processing:
            return

        self.is_processing = True
        self.btn_copy.configure(state="disabled")
        self.btn_stop.configure(state="normal")
        self.log_status("Инициализация...", "orange")
        self.log(
            f"Запуск команды: dd if={self.source_combo.get()} of={self.dest_entry.get()}"
        )

        # Запускаем процесс в отдельном потоке, чтобы GUI не завис
        thread = threading.Thread(target=self.run_dd)
        thread.start()

    def stop_process(self):
        if self.process and self.is_processing:
            self.log("Остановка процесса...")
            try:
                # Отправляем SIGTERM процессу
                self.process.terminate()
                # Ждем немного завершения
                self.process.wait(timeout=5)
                self.log_status("Прервано пользователем", "orange")
            except:
                # Если не завершилось, кидаем SIGKILL
                self.process.kill()
                self.log_status("Принудительно остановлено", "red")

            self.is_processing = False
            self.process = None
            self.btn_copy.configure(state="normal")
            self.btn_stop.configure(state="disabled")


if __name__ == "__main__":
    app = DiskClonerApp()
    app.mainloop()
