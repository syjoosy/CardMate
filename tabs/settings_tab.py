import customtkinter as ctk
from utils.logging import log_message, DEBUG, INFO, WARNING, ERROR, SUCCESS, CRITICAL

class SettingsTab:
    def __init__(self, parent):
        log_message(INFO, "Generate Settings Tab!")
        label = ctk.CTkLabel(
            parent,
            text="This is settings tab!",
            justify="center"
        )
        label.pack(expand=True)