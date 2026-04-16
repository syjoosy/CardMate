import os
import sys
import subprocess
import customtkinter as ctk

from utils.logging import log_message, INFO
from utils.commands import get_platform
from utils.logging import MAC_OS_LOG_PATH, LINUX_LOG_PATH, WINDOWS_LOG_PATH

def get_logs_path():
    platform = get_platform()

    if platform == "windows":
        return os.path.expanduser(WINDOWS_LOG_PATH)
    elif platform == "macos":
        return os.path.expanduser(MAC_OS_LOG_PATH)
    elif platform == "linux":
        return os.path.expanduser(LINUX_LOG_PATH)
    else:
        raise Exception("Unsupported OS. Cant get log path!")

def get_logs_size():
    path = get_logs_path()
    total_size = 0

    if os.path.exists(path):
        for root, _, files in os.walk(path):
            for file in files:
                if file.startswith("cardMate"):
                    file_path = os.path.join(root, file)
                    try:
                        total_size += os.path.getsize(file_path)
                    except Exception:
                        raise Exception("Cant get log size!")

    return total_size


def format_size(size_bytes):
    units = ["B", "KB", "MB", "GB", "TB"]
    index = 0

    while size_bytes >= 1024 and index < len(units) - 1:
        size_bytes /= 1024
        index += 1

    return f"{size_bytes:.2f} {units[index]}"

def open_folder(path: str):
    path = os.path.expanduser(path)
    platform = get_platform()

    if platform == "windows":
        os.startfile(path)
    elif platform == "macos":
        subprocess.run(["open", path])
    elif platform == "linux":
        subprocess.run(["xdg-open", path])
    else:
        raise Exception("Unsupported OS. Cant open folder!")


class SettingsTab:
    def __init__(self, parent):
        log_message(INFO, "Generate Settings Tab!")

        row_frame = ctk.CTkFrame(parent)
        row_frame.pack(pady=10, fill="x")

        self.logs_size_label = ctk.CTkLabel(
            row_frame,
            text=self.get_logs_size_text()
        )
        self.logs_size_label.pack(side="left", padx=10)

        button = ctk.CTkButton(
            row_frame,
            text="Open logs folder",
            command=self.open_logs
        )
        button.pack(side="right", padx=10)

    def get_logs_size_text(self):
        size = get_logs_size()
        return f"Logs size: {format_size(size)}"

    def open_logs(self):
        log_message(INFO, "Start open logs folder!")
        path = get_logs_path()
        open_folder(path)
        self.logs_size_label.configure(text=self.get_logs_size_text())