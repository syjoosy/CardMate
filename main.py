import customtkinter as ctk

from tabs.flash_tab import FlashTab
from tabs.backup_tab import BackupTab
from tabs.format_tab import FormatTab
from tabs.about_tab import AboutTab

ctk.set_appearance_mode("Dark")
ctk.set_default_color_theme("blue")


class CardMateApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.title("CardMate")
        self.geometry("650x450")

        self.tabview = ctk.CTkTabview(self)
        self.tabview.pack(expand=True, fill="both", padx=10, pady=10)

        self.tabview.add("Flash")
        self.tabview.add("Backup")
        self.tabview.add("Format")
        self.tabview.add("About")

        FlashTab(self.tabview.tab("Flash"))
        BackupTab(self.tabview.tab("Backup"))
        FormatTab(self.tabview.tab("Format"))
        AboutTab(self.tabview.tab("About"))


if __name__ == "__main__":
    app = CardMateApp()
    app.mainloop()