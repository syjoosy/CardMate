import customtkinter as ctk
import subprocess
import threading
import os
import sys

ctk.set_appearance_mode("Dark")
ctk.set_default_color_theme("blue")

def get_default_paths():
    home = os.path.expanduser("~")

    return {
        "Home": home,
        "Documents": os.path.join(home, "Documents"),
        "Downloads": os.path.join(home, "Downloads"),
        "Desktop": os.path.join(home, "Desktop"),
    }

def get_macos_disks():
    try:
        result = subprocess.run(
            ["diskutil", "list"],
            capture_output=True,
            text=True
        )

        disks = []

        for line in result.stdout.splitlines():
            line = line.strip()

            if line.startswith("/dev/disk"):
                disk_name = line.split()[0]

                info = subprocess.run(
                    ["diskutil", "info", disk_name],
                    capture_output=True,
                    text=True
                ).stdout

                size = "Unknown"
                protocol = ""
                internal = ""

                for info_line in info.splitlines():
                    info_line = info_line.strip()

                    if "Disk Size:" in info_line:
                        size = info_line.split(":")[1].split("(")[0].strip()

                    elif "Protocol:" in info_line:
                        protocol = info_line.split(":")[1].strip()

                    elif "Internal:" in info_line:
                        internal = info_line.split(":")[1].strip()

                disk_type = "internal" if internal == "Yes" else "external"

                display_name = f"{disk_name} ({size}, {disk_type} {protocol})"
                disks.append((display_name, disk_name))

        return disks if disks else [("/dev/disk0 (unknown)", "/dev/disk0")]

    except Exception as e:
        print(f"Ошибка получения списка дисков: {e}")
        return [("/dev/disk0 (unknown)", "/dev/disk0")]


class DiskClonerApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.title("CardMate v0.1 Dev Build")
        self.geometry("650x450")
        self.resizable(False, False)

        self.process = None
        self.is_processing = False

        self.device_map = {}

        self.title_label = ctk.CTkLabel(
            self, text="Создание образа диска", font=ctk.CTkFont(size=18, weight="bold")
        )
        self.title_label.pack(pady=20)

        self.source_frame = ctk.CTkFrame(self)
        self.source_frame.pack(padx=20, fill="x")

        self.source_label = ctk.CTkLabel(
            self.source_frame, text="Source:"
        )
        self.source_label.grid(row=0, column=0, padx=10, pady=5, sticky="w")

        self.source_combo = ctk.CTkComboBox(
            self.source_frame, values=[], width=360
        )
        self.source_combo.grid(row=0, column=1, padx=10, pady=5)

        self.refresh_button = ctk.CTkButton(
            self.source_frame,
            text="Refresh",
            command=self.refresh_disks,
            width=100
        )
        self.refresh_button.grid(row=0, column=2, padx=5)

        self.note_source = ctk.CTkLabel(
            self.source_frame,
            text="⚠️ Выбирайте диск целиком (/dev/diskX), не раздел!",
            font=ctk.CTkFont(size=10),
            text_color="gray",
        )
        self.note_source.grid(row=1, column=1, padx=10, pady=2, sticky="w")

        self.dest_frame = ctk.CTkFrame(self)
        self.dest_frame.pack(padx=20, fill="x")

        self.dest_label = ctk.CTkLabel(self.dest_frame, text="Destination:")
        self.dest_label.grid(row=0, column=0, padx=10, pady=5)

        # Получаем стандартные пути
        self.default_paths = get_default_paths()

        # ComboBox с папками
        self.dest_combo = ctk.CTkComboBox(
            self.dest_frame,
            values=list(self.default_paths.keys()),
            width=180,
            command=self.on_dest_select
        )
        self.dest_combo.grid(row=0, column=1, padx=10, pady=5)

        # Поле ввода пути
        self.dest_entry = ctk.CTkEntry(
            self.dest_frame,
            placeholder_text="/Users/user/image.img",
            width=260
        )
        self.dest_entry.grid(row=0, column=2, padx=10, pady=5)

        # Устанавливаем дефолт (Downloads)
        self.dest_combo.set("Downloads")
        self.dest_entry.insert(0, os.path.join(self.default_paths["Downloads"], "image.img"))

        self.control_frame = ctk.CTkFrame(self)
        self.control_frame.pack(pady=20)

        self.btn_copy = ctk.CTkButton(
            self.control_frame,
            text="НАЧАТЬ",
            command=self.start_process,
            width=200,
            height=35,
        )
        self.btn_copy.pack(side="left", padx=10)

        self.btn_stop = ctk.CTkButton(
            self.control_frame,
            text="СТОП",
            command=self.stop_process,
            width=200,
            height=35,
            fg_color="red",
        )
        self.btn_stop.pack(side="left", padx=10)
        self.btn_stop.configure(state="disabled")

        self.log_label = ctk.CTkLabel(
            self, text="Готов", text_color="green"
        )
        self.log_label.pack(pady=10)

        self.log_text = ctk.CTkTextbox(self, width=500, height=150)
        self.log_text.pack(padx=20, pady=10)
        self.log_text.configure(state="disabled")

        self.refresh_disks()

    def log(self, message):
        self.log_text.configure(state="normal")
        self.log_text.insert("end", message + "\n")
        self.log_text.see("end")
        self.log_text.configure(state="disabled")

    def log_status(self, message, color="white"):
        self.log_label.configure(text=message, text_color=color)

    def refresh_disks(self):
        if sys.platform == "darwin":
            disks = get_macos_disks()
            self.device_map = {name: path for name, path in disks}

            self.source_combo.configure(values=list(self.device_map.keys()))

            if disks:
                self.source_combo.set(disks[0][0])

            self.log("Список дисков обновлён")

    def on_dest_select(self, choice):
        base_path = self.default_paths.get(choice, "")

        # Если уже есть имя файла — сохраняем его
        current = self.dest_entry.get().strip()
        filename = "image.img"

        if current:
            filename = os.path.basename(current)

        new_path = os.path.join(base_path, filename)

        self.dest_entry.delete(0, "end")
        self.dest_entry.insert(0, new_path)

    def run_dd(self):
        selected = self.source_combo.get().strip()
        source = self.device_map.get(selected, selected)
        dest = self.dest_entry.get().strip()

        if not source or not dest:
            self.log("Ошибка: заполните поля")
            return

        if not dest.endswith(".img"):
            dest += ".img"
            self.dest_entry.delete(0, "end")
            self.dest_entry.insert(0, dest)

        if sys.platform == "darwin" and source.startswith("/dev/disk"):
            source = source.replace("/dev/disk", "/dev/rdisk")

        if not os.geteuid() == 0:
            self.log("⚠️ Запуск без sudo может не сработать")

        if "internal" in selected:
            self.log("⚠️ ВНИМАНИЕ: выбран внутренний диск!")

        cmd = [
            "dd",
            f"if={source}",
            f"of={dest}",
            "status=progress",
            "conv=noerror,sync",
        ]

        try:
            self.process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )

            while True:
                if self.process.poll() is not None:
                    self.log_status("Готово", "green")
                    break

                line = self.process.stdout.readline()
                if line:
                    self.log(line.strip())

                self.update()

        except Exception as e:
            self.log(f"Ошибка: {e}")

        self.is_processing = False
        self.btn_copy.configure(state="normal")
        self.btn_stop.configure(state="disabled")

    def start_process(self):
        if self.is_processing:
            return

        self.is_processing = True
        self.btn_copy.configure(state="disabled")
        self.btn_stop.configure(state="normal")

        thread = threading.Thread(target=self.run_dd)
        thread.start()

    def stop_process(self):
        if self.process:
            self.process.terminate()
            self.log("Остановлено")

        self.is_processing = False
        self.btn_copy.configure(state="normal")
        self.btn_stop.configure(state="disabled")


if __name__ == "__main__":
    app = DiskClonerApp()
    app.mainloop()