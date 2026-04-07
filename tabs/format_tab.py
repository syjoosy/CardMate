import customtkinter as ctk

from utils.logging import log_message, DEBUG, INFO, WARNING, ERROR, SUCCESS, CRITICAL

class FormatTab:
    def __init__(self, parent):
        log_message(INFO, "Generate Format Tab!")
        label = ctk.CTkLabel(parent, text="Format (WIP)")
        label.pack(pady=50)