import customtkinter as ctk
import threading
import subprocess
import os
import sys
from tkinter import filedialog

from utils.commands import get_disk_list
from utils.logging import log_message, DEBUG, INFO, WARNING, ERROR, SUCCESS, CRITICAL


class FlashTab:
    def __init__(self, parent):
        self.parent = parent

        self.process = None
        self.is_processing = False
        self.device_map = {}

        log_message(INFO, "Generate Flash Tab!")

        self.build_ui()
        self.refresh_disks()

    def build_ui(self):
        self.title_label = ctk.CTkLabel(
            self.parent,
            text="Flash image to disk",
            font=ctk.CTkFont(size=18, weight="bold"),
        )
        self.title_label.pack(pady=20)

        # IMAGE FILE
        self.source_frame = ctk.CTkFrame(self.parent)
        self.source_frame.pack(padx=20, fill="x")

        ctk.CTkLabel(self.source_frame, text="Image file:").grid(
            row=0, column=0, padx=10, pady=5
        )

        self.source_entry = ctk.CTkEntry(
            self.source_frame,
            placeholder_text="/path/to/image.img",
            width=360,
        )
        self.source_entry.grid(row=0, column=1, padx=10, pady=5)

        self.browse_button = ctk.CTkButton(
            self.source_frame,
            text="Browse",
            command=self.select_file,
            width=100,
        )
        self.browse_button.grid(row=0, column=2, padx=5)

        # TARGET DISK
        self.dest_frame = ctk.CTkFrame(self.parent)
        self.dest_frame.pack(padx=20, fill="x")

        ctk.CTkLabel(self.dest_frame, text="Target disk:").grid(
            row=0, column=0, padx=10, pady=5
        )

        self.dest_combo = ctk.CTkComboBox(self.dest_frame, values=[], width=360)
        self.dest_combo.grid(row=0, column=1, padx=10, pady=5)

        self.refresh_button = ctk.CTkButton(
            self.dest_frame,
            text="Refresh",
            command=self.refresh_disks,
            width=100,
        )
        self.refresh_button.grid(row=0, column=2, padx=5)

        self.note = ctk.CTkLabel(
            self.dest_frame,
            text="⚠️ ALL DATA ON THE DISK WILL BE ERASED!",
            font=ctk.CTkFont(size=10),
            text_color="red",
        )
        self.note.grid(row=1, column=1, padx=10, pady=2, sticky="w")

        # CONTROLS
        self.control_frame = ctk.CTkFrame(self.parent)
        self.control_frame.pack(pady=20)

        self.btn_start = ctk.CTkButton(
            self.control_frame,
            text="Start Flash",
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

        for tag, color in {
            "INFO": "white",
            "WARNING": "orange",
            "ERROR": "red",
            "SUCCESS": "green",
            "CRITICAL": "red",
            "DEBUG": "gray",
        }.items():
            self.log_text.tag_config(tag, foreground=color)

    # =========================
    # FILE PICKER
    # =========================
    def select_file(self):
        file_path = filedialog.askopenfilename(
            filetypes=[("Image files", "*.img *.iso"), ("All files", "*.*")]
        )
        if file_path:
            self.source_entry.delete(0, "end")
            self.source_entry.insert(0, file_path)

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

        self.dest_combo.configure(values=list(self.device_map.keys()))

        if disks:
            self.dest_combo.set(disks[0][0])

        self.log(INFO, "Disk list updated")

    # =========================
    # FLASH PROCESS
    # =========================
    def run_dd(self):
        image = self.source_entry.get().strip()
        selected = self.dest_combo.get().strip()
        target = self.device_map.get(selected, selected)

        if not image or not target:
            self.log(ERROR, "Fill all fields!")
            return

        if not os.path.exists(image):
            self.log(ERROR, "Image file not found!")
            return

        if sys.platform == "darwin" and target.startswith("/dev/disk"):
            target = target.replace("/dev/disk", "/dev/rdisk")

        if hasattr(os, "geteuid") and os.geteuid() != 0:
            self.log(WARNING, "⚠️ Running without sudo may not work.")

        if "internal" in selected:
            self.log(CRITICAL, "🚨 YOU ARE FLASHING INTERNAL DISK!")

        cmd = ["dd", f"if={image}", f"of={target}", "bs=4m", "status=progress"]

        self.log(INFO, "Command: " + " ".join(cmd))

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

        self.log(INFO, "Flashing finished")

    # =========================
    # CONTROL
    # =========================
    def start_process(self):
        if self.is_processing:
            return

        self.is_processing = True
        self.btn_start.configure(state="disabled")
        self.btn_stop.configure(state="normal")

        threading.Thread(target=self.run_dd).start()

    def stop_process(self):
        if self.process:
            self.process.terminate()
            self.log(WARNING, "Flashing stopped!")

        self.is_processing = False
        self.btn_start.configure(state="normal")
        self.btn_stop.configure(state="disabled")