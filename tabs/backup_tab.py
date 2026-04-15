import customtkinter as ctk
import threading
import subprocess
import os
import sys

from utils.paths import get_default_paths
from utils.commands import create_backup, get_disk_list
from utils.logging import log_message, DEBUG, INFO, WARNING, ERROR, SUCCESS, CRITICAL
from ui.dialog import show_dialog

class BackupTab:
    def __init__(self, parent):
        self.parent = parent

        self.process = None
        self.is_processing = False
        self.device_map = {}

        self.default_paths = get_default_paths()

        log_message(INFO, "Generate Backup Tab!")

        

        self.build_ui()
        self.refresh_disks()


    def build_ui(self):
        self.title_label = ctk.CTkLabel(
            self.parent,
            text="Create disk image",
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
            text="⚠️ Select the entire disk (/dev/diskX), not the partition!",
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
            text="Start",
            command=self.start_process,
            width=200,
            height=35,
        )
        self.btn_copy.pack(side="left", padx=10)

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

        self.log_text.tag_config("INFO", foreground="white")
        self.log_text.tag_config("WARNING", foreground="orange")
        self.log_text.tag_config("ERROR", foreground="red")
        self.log_text.tag_config("SUCCESS", foreground="green")
        self.log_text.tag_config("CRITICAL", foreground="red")
        self.log_text.tag_config("DEBUG", foreground="gray")

    # =========================
    # LOGGING
    # =========================
    def log(self, severity, message):
        self.log_text.configure(state="normal")
        self.log_text.insert("end", message + "\n", severity)
        self.log_text.see("end")
        self.log_text.configure(state="disabled")

        log_message(severity, message)

    def log_status(self, message, color="white"):
        self.log_label.configure(text=message, text_color=color)

    # =========================
    # DISKS
    # =========================
    def refresh_disks(self):
        disks = get_disk_list()
        self.device_map = {name: path for name, path in disks}

        self.source_combo.configure(values=list(self.device_map.keys()))

        if disks:
            self.source_combo.set(disks[0][0])

        self.log(INFO, "The list of disks has been updated")

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
            self.log(ERROR, "Error: fill in the fields")
            return

        if not dest.endswith(".img"):
            dest += ".img"
            self.dest_entry.delete(0, "end")
            self.dest_entry.insert(0, dest)

        if hasattr(os, "geteuid") and os.geteuid() != 0:
            self.log(WARNING, "⚠️ Running without sudo may not work.")

        if "internal" in selected:
            self.log(WARNING, "⚠️ ATTENTION: the internal disk is selected!")

        cmd = create_backup(source, dest)

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
                    if "Permission denied" in line:
                        self.log(ERROR, line.strip())
                    else:
                        self.log(INFO, line.strip())

        except Exception as e:
            self.log(ERROR, f"Error: {e}")

        self.is_processing = False
        self.btn_copy.configure(state="normal")
        self.btn_stop.configure(state="disabled")

        show_dialog(self.parent, "Ready", "Backup finished!")
        self.log(INFO, "Backup finished")

    # =========================
    # CONTROL
    # =========================
    def start_process(self):
        if self.is_processing:
            return

        selected = self.source_combo.get().strip()
        source = self.device_map.get(selected, selected)
        dest = self.dest_entry.get().strip()

        self.log(INFO, f"Start backup!")
        self.log(INFO, f"Source: {source}")
        self.log(INFO, f"Destination: {dest}")

        self.is_processing = True
        self.btn_copy.configure(state="disabled")
        self.btn_stop.configure(state="normal")

        threading.Thread(target=self.run_dd).start()

    def stop_process(self):
        if self.process:
            self.process.terminate()
            self.log(WARNING, "Backup stopped!")

        self.is_processing = False
        self.btn_copy.configure(state="normal")
        self.btn_stop.configure(state="disabled")

        show_dialog(self.parent, "Warning", "Backup stopped!", type_='warning')