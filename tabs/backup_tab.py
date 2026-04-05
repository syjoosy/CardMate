import customtkinter as ctk
import threading
import subprocess
import os
import sys

from utils.paths import get_default_paths
from utils.disks import get_macos_disks


class BackupTab:
    def __init__(self, parent):
        self.parent = parent

        self.process = None
        self.is_processing = False
        self.device_map = {}

        self.default_paths = get_default_paths()

        self.build_ui()
        self.refresh_disks()

    def build_ui(self):
        self.title_label = ctk.CTkLabel(
            self.parent,
            text="Создание образа диска",
            font=ctk.CTkFont(size=18, weight="bold"),
        )
        self.title_label.pack(pady=20)

        # SOURCE
        self.source_frame = ctk.CTkFrame(self.parent)
        self.source_frame.pack(padx=20, fill="x")

        ctk.CTkLabel(self.source_frame, text="Source:").grid(
            row=0, column=0, padx=10, pady=5, sticky="w"
        )

        self.source_combo = ctk.CTkComboBox(self.source_frame, values=[], width=360)
        self.source_combo.grid(row=0, column=1, padx=10, pady=5)

        self.refresh_button = ctk.CTkButton(
            self.source_frame, text="Refresh", command=self.refresh_disks, width=100
        )
        self.refresh_button.grid(row=0, column=2, padx=5)

        self.note_source = ctk.CTkLabel(
            self.source_frame,
            text="⚠️ Выбирайте диск целиком (/dev/diskX), не раздел!",
            font=ctk.CTkFont(size=10),
            text_color="gray",
        )
        self.note_source.grid(row=1, column=1, padx=10, pady=2, sticky="w")

        # DESTINATION
        self.dest_frame = ctk.CTkFrame(self.parent)
        self.dest_frame.pack(padx=20, fill="x")

        ctk.CTkLabel(self.dest_frame, text="Destination:").grid(
            row=0, column=0, padx=10, pady=5
        )

        self.dest_combo = ctk.CTkComboBox(
            self.dest_frame,
            values=list(self.default_paths.keys()),
            width=180,
            command=self.on_dest_select,
        )
        self.dest_combo.grid(row=0, column=1, padx=10, pady=5)

        self.dest_entry = ctk.CTkEntry(
            self.dest_frame,
            placeholder_text="/Users/user/image.img",
            width=260,
        )
        self.dest_entry.grid(row=0, column=2, padx=10, pady=5)

        self.dest_combo.set("Downloads")
        self.dest_entry.insert(
            0, os.path.join(self.default_paths["Downloads"], "image.img")
        )

        # CONTROLS
        self.control_frame = ctk.CTkFrame(self.parent)
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

        # STATUS
        self.log_label = ctk.CTkLabel(
            self.parent, text="Готов", text_color="green"
        )
        self.log_label.pack(pady=10)

        self.log_text = ctk.CTkTextbox(self.parent, width=500, height=150)
        self.log_text.pack(padx=20, pady=10)
        self.log_text.configure(state="disabled")

    # =========================
    # LOGGING
    # =========================
    def log(self, message):
        self.log_text.configure(state="normal")
        self.log_text.insert("end", message + "\n")
        self.log_text.see("end")
        self.log_text.configure(state="disabled")

    def log_status(self, message, color="white"):
        self.log_label.configure(text=message, text_color=color)

    # =========================
    # DISKS
    # =========================
    def refresh_disks(self):
        if sys.platform == "darwin":
            disks = get_macos_disks()
            self.device_map = {name: path for name, path in disks}

            self.source_combo.configure(values=list(self.device_map.keys()))

            if disks:
                self.source_combo.set(disks[0][0])

            self.log("Список дисков обновлён")

    # =========================
    # DEST PATH
    # =========================
    def on_dest_select(self, choice):
        base_path = self.default_paths.get(choice, "")

        current = self.dest_entry.get().strip()
        filename = os.path.basename(current) if current else "image.img"

        new_path = os.path.join(base_path, filename)

        self.dest_entry.delete(0, "end")
        self.dest_entry.insert(0, new_path)

    # =========================
    # DD PROCESS
    # =========================
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

        if hasattr(os, "geteuid") and os.geteuid() != 0:
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

        except Exception as e:
            self.log(f"Ошибка: {e}")

        self.is_processing = False
        self.btn_copy.configure(state="normal")
        self.btn_stop.configure(state="disabled")

    # =========================
    # CONTROL
    # =========================
    def start_process(self):
        if self.is_processing:
            return

        self.is_processing = True
        self.btn_copy.configure(state="disabled")
        self.btn_stop.configure(state="normal")

        threading.Thread(target=self.run_dd).start()

    def stop_process(self):
        if self.process:
            self.process.terminate()
            self.log("Остановлено")

        self.is_processing = False
        self.btn_copy.configure(state="normal")
        self.btn_stop.configure(state="disabled")