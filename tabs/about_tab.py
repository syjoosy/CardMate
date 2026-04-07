import customtkinter as ctk
from const import VERSION
from utils.logging import log_message, DEBUG, INFO, WARNING, ERROR, SUCCESS, CRITICAL

class AboutTab:
    def __init__(self, parent):
        log_message(INFO, "Generate About Tab!")
        label = ctk.CTkLabel(
            parent,
            text="CardMate\n\nDisk utility tool\n" + str(VERSION),
            justify="center"
        )
        label.pack(expand=True)