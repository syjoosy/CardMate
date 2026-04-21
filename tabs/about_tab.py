import customtkinter as ctk
import webbrowser
from const import VERSION
from utils.logging import log_message, INFO

GITHUB_URL = "https://github.com/syjoosy/CardMate"

class AboutTab:
    def __init__(self, parent):
        log_message(INFO, "Generate About Tab!")

        container = ctk.CTkFrame(parent)
        container.pack(expand=True, fill="both", padx=20, pady=20)

        # Title
        title = ctk.CTkLabel(
            container,
            text="CardMate",
            font=ctk.CTkFont(size=20, weight="bold")
        )
        title.pack(pady=(10, 5))

        # Extended description
        description = ctk.CTkLabel(
            container,
            text="A utility program for flashing, formatting, and backing up SD cards.",
            wraplength=300,
            justify="center"
        )
        description.pack(pady=5)

        # Version
        version_label = ctk.CTkLabel(
            container,
            text=f"Version: {VERSION}"
        )
        version_label.pack(pady=2)

        # Spacer
        spacer = ctk.CTkFrame(container, fg_color="transparent")
        spacer.pack(expand=True, fill="both", pady=20)

        # GitHub button
        github_button = ctk.CTkButton(
            container,
            text="View on GitHub",
            command=self.open_github
        )
        github_button.pack(pady=2)

        # License
        license_label = ctk.CTkLabel(
            container,
            text="License: MIT"
        )
        license_label.pack(pady=1)

        # Author
        author_label = ctk.CTkLabel(
            container,
            text="Made by Vadim 'syjoosy' Nikolaev in 2026"
        )
        author_label.pack(pady=1)

    def open_github(self):
        webbrowser.open(GITHUB_URL)