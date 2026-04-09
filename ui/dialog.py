import customtkinter as ctk


def show_dialog(
    parent,
    title="Message",
    message="",
    type_="info",
    buttons=None,  # список: [("OK", callback), ("Cancel", callback)]
    modal=True
):
    """
    Универсальное диалоговое окно
    
    :param parent: родитель
    :param title: заголовок
    :param message: текст
    :param type_: info | warning | error
    :param buttons: список кнопок [(text, callback)]
    :param modal: блокировать ли основное окно
    """

    styles = {
        "info": {"color": "#2b7cff", "icon": "ℹ️"},
        "warning": {"color": "#f1c40f", "icon": "⚠️"},
        "error": {"color": "#e74c3c", "icon": "❌"},
    }

    style = styles.get(type_, styles["info"])

    dialog = ctk.CTkToplevel(parent)
    dialog.title(title)
    dialog.geometry("300x140")
    dialog.resizable(False, False)

    dialog.transient(parent)  # поверх родителя

    if modal:
        dialog.grab_set()  # делает окно модальным

    # Контейнер
    frame = ctk.CTkFrame(dialog)
    frame.pack(fill="both", expand=True, padx=15, pady=15)

    # Иконка
    icon_label = ctk.CTkLabel(
        frame,
        text=style["icon"],
        font=ctk.CTkFont(size=32)
    )
    icon_label.grid(row=0, column=0, rowspan=2, padx=10, pady=10)

    # Заголовок
    # title_label = ctk.CTkLabel(
    #     frame,
    #     text=title,
    #     font=ctk.CTkFont(size=16, weight="bold"),
    #     text_color=style["color"]
    # )
    # title_label.grid(row=0, column=1, sticky="w", pady=(10, 0))

    # Сообщение
    msg_label = ctk.CTkLabel(
        frame,
        text=message,
        wraplength=260,
        justify="left"
    )
    msg_label.grid(row=1, column=1, sticky="w")

    # Кнопки
    btn_frame = ctk.CTkFrame(dialog, fg_color="transparent")
    btn_frame.pack(pady=10)

    if not buttons:
        buttons = [("OK", None)]

    def make_cmd(callback):
        def cmd():
            if callback:
                callback()
            dialog.destroy()
        return cmd

    for text, callback in buttons:
        btn = ctk.CTkButton(
            btn_frame,
            text=text,
            width=100,
            command=make_cmd(callback)
        )
        btn.pack(side="left", padx=5)

    return dialog

# Examples:
# show_dialog(self, "Готово", "Приложение успешно запущено")

# def on_yes():
#     print("YES")

# def on_no():
#     print("NO")

# show_dialog(
#     self,
#     "Подтверждение",
#     "Удалить файл?",
#     type_="warning",
#     buttons=[
#         ("Да", on_yes),
#         ("Нет", on_no)
#     ]
# )