import customtkinter as ctk


class AboutTab:
    def __init__(self, parent):
        label = ctk.CTkLabel(
            parent,
            text="CardMate\n\nDisk utility tool\nv0.2",
            justify="center"
        )
        label.pack(expand=True)