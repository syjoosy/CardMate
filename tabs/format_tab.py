import customtkinter as ctk
import threading
import subprocess
import sys

from utils.disks import get_macos_disks
from utils.logging import log_message, INFO, WARNING, ERROR
from utils.commands import format_disk, get_filesystems

class FormatTab:
    def __init__(self, parent):
        self.parent = parent

        self.process = None
        self.is_processing = False
        self.device_map = {}

        log_message(INFO, "Generate Format Tab!")

        self.build_ui()
        self.refresh_disks()

    # =========================
    # UI
    # =========================
    def build_ui(self):
        self.title_label = ctk.CTkLabel(
            self.parent,
            text="Format disk",
            font=ctk.CTkFont(size=18, weight="bold"),
        )
        self.title_label.pack(pady=20)

        # DISK SELECT
        self.disk_frame = ctk.CTkFrame(self.parent)
        self.disk_frame.pack(padx=20, fill="x")

        ctk.CTkLabel(self.disk_frame, text="Disk:").grid(
            row=0, column=0, padx=10, pady=5, sticky="w"
        )

        self.disk_combo = ctk.CTkComboBox(self.disk_frame, values=[], width=360)
        self.disk_combo.grid(row=0, column=1, padx=10, pady=5)

        self.refresh_button = ctk.CTkButton(
            self.disk_frame, text="Refresh", command=self.refresh_disks, width=100
        )
        self.refresh_button.grid(row=0, column=2, padx=5)

        # FILESYSTEM SELECT
        self.fs_frame = ctk.CTkFrame(self.parent)
        self.fs_frame.pack(padx=20, fill="x", pady=10)

        ctk.CTkLabel(self.fs_frame, text="Filesystem:").grid(
            row=0, column=0, padx=10, pady=5, sticky="w"
        )

        filesystems = get_filesystems()

        self.fs_combo = ctk.CTkComboBox(
            self.fs_frame,
            values=filesystems,
            width=360,
        )

        if filesystems:
            self.fs_combo.set(filesystems[0])

        self.fs_combo.grid(row=0, column=1, padx=10, pady=5)
        self.fs_combo.set("APFS")

        # VOLUME NAME
        self.name_frame = ctk.CTkFrame(self.parent)
        self.name_frame.pack(padx=20, fill="x", pady=10)

        ctk.CTkLabel(self.name_frame, text="Volume Name:").grid(
            row=0, column=0, padx=10, pady=5, sticky="w"
        )

        self.name_combo = ctk.CTkComboBox(
            self.name_frame,
            values=[
                "MyDisk",
                "Backup",
                "Storage",
                "Data",
                "External",
                "USB",
                "Work",
                "Media",
            ],
            width=180,
            command=self.on_name_select,
        )
        self.name_combo.grid(row=0, column=1, padx=10, pady=5)

        self.name_entry = ctk.CTkEntry(
            self.name_frame,
            placeholder_text="Enter volume name",
            width=260,
        )
        self.name_entry.grid(row=0, column=2, padx=10, pady=5)

        self.name_combo.set("MyDisk")
        self.name_entry.insert(0, "MyDisk")

        # WARNING
        self.warning_label = ctk.CTkLabel(
            self.parent,
            text="⚠️ ALL DATA ON THE DISK WILL BE ERASED!",
            text_color="red",
        )
        self.warning_label.pack(pady=5)

        # CONTROLS
        self.control_frame = ctk.CTkFrame(self.parent)
        self.control_frame.pack(pady=20)

        self.btn_start = ctk.CTkButton(
            self.control_frame,
            text="Format",
            command=self.start_process,
            width=200,
            height=35,
        )
        self.btn_start.pack(side="left", padx=10)

        self.btn_stop = ctk.CTkButton(
            self.control_frame,
            text="Stop",
            command=self.stop_process,
            width=200,
            height=35,
            fg_color="red",
        )
        self.btn_stop.pack(side="left", padx=10)
        self.btn_stop.configure(state="disabled")

        # STATUS
        self.log_label = ctk.CTkLabel(
            self.parent, text="Ready", text_color="green"
        )
        self.log_label.pack(pady=5)

        self.log_text = ctk.CTkTextbox(self.parent, width=500, height=300)
        self.log_text.pack(padx=20, pady=5)
        self.log_text.configure(state="disabled")
    
    def on_name_select(self, choice):
        self.name_entry.delete(0, "end")
        self.name_entry.insert(0, choice)

    # =========================
    # LOG
    # =========================
    def log(self, severity, message):
        self.log_text.configure(state="normal")
        self.log_text.insert("end", message + "\n")
        self.log_text.see("end")
        self.log_text.configure(state="disabled")

        log_message(severity, message)

    def log_status(self, message, color="white"):
        self.log_label.configure(text=message, text_color=color)

    # =========================
    # DISKS
    # =========================
    def refresh_disks(self):
        if sys.platform == "darwin":
            disks = get_macos_disks()
            self.device_map = {name: path for name, path in disks}

            self.disk_combo.configure(values=list(self.device_map.keys()))

            if disks:
                self.disk_combo.set(disks[0][0])

            self.log(INFO, "Disk list updated")

    # =========================
    # FORMAT PROCESS
    # =========================
    def run_format(self):
        selected = self.disk_combo.get().strip()
        disk = self.device_map.get(selected, selected)
        fs = self.fs_combo.get()

        if not disk:
            self.log(ERROR, "Select a disk")
            return

        if "internal" in selected.lower():
            self.log(WARNING, "⚠️ INTERNAL DISK SELECTED!")

        volume_name = self.name_entry.get().strip()

        if not volume_name:
            volume_name = "MyDisk"

        cmd = format_disk(disk, fs, volume_name)

        self.log(INFO, "Command: " + str(cmd))

        try:
            self.process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )

            while True:
                if self.process.poll() is not None:
                    self.log_status("Done", "green")
                    break

                line = self.process.stdout.readline()
                if line:
                    self.log(INFO, line.strip())

        except Exception as e:
            self.log(ERROR, f"Error: {e}")

        self.is_processing = False
        self.btn_start.configure(state="normal")
        self.btn_stop.configure(state="disabled")

    # =========================
    # CONTROL
    # =========================
    def start_process(self):
        if self.is_processing:
            return

        self.log(INFO, "Start formatting...")

        self.is_processing = True
        self.btn_start.configure(state="disabled")
        self.btn_stop.configure(state="normal")

        threading.Thread(target=self.run_format).start()

    def stop_process(self):
        if self.process:
            self.process.terminate()
            self.log(WARNING, "Formatting stopped")

        self.is_processing = False
        self.btn_start.configure(state="normal")
        self.btn_stop.configure(state="disabled")