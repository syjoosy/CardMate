import customtkinter as ctk
from const import VERSION

class AboutTab:
    def __init__(self, parent):
        label = ctk.CTkLabel(
            parent,
            text="CardMate\n\nDisk utility tool\n" + str(VERSION),
            justify="center"
        )
        label.pack(expand=True)