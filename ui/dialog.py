import customtkinter as ctk
import traceback


import traceback
import sys


def show_error_dialog(parent, title="Error", exc=None, tb=None, modal=True):
    """
    Диалог ошибки с traceback + копирование + выход из приложения
    """

    dialog = ctk.CTkToplevel(parent)
    dialog.title(title)
    dialog.transient(parent)

    if modal:
        dialog.grab_set()

    dialog.minsize(450, 300)

    # ===== Контейнер =====
    frame = ctk.CTkFrame(dialog)
    frame.pack(fill="both", expand=True, padx=15, pady=15)

    # ===== Верх (иконка + сообщение) =====
    top_frame = ctk.CTkFrame(frame, fg_color="transparent")
    top_frame.pack(fill="x", pady=(0, 10))

    icon = ctk.CTkLabel(
        top_frame,
        text="❌",
        font=ctk.CTkFont(size=32)
    )
    icon.pack(side="left", padx=(0, 10))

    if exc:
        short_msg = f"{type(exc).__name__}: {exc}"
    else:
        short_msg = "Unknown error"

    msg_label = ctk.CTkLabel(
        top_frame,
        text=short_msg,
        justify="left",
        wraplength=380
    )
    msg_label.pack(side="left", fill="x", expand=True)

    # ===== Textbox =====
    textbox = ctk.CTkTextbox(frame, height=180)
    textbox.pack(fill="both", expand=True)

    if exc and tb:
        error_text = "".join(traceback.format_exception(type(exc), exc, tb))
    else:
        error_text = str(exc) if exc else ""

    textbox.insert("1.0", error_text)
    textbox.configure(state="disabled")

    # ===== Кнопки =====
    btn_frame = ctk.CTkFrame(frame, fg_color="transparent")
    btn_frame.pack(pady=(10, 0), fill="x")

    def copy_to_clipboard():
        dialog.clipboard_clear()
        dialog.clipboard_append(error_text)

    def close():
        dialog.destroy()

    def exit_app():
        dialog.destroy()
        parent.destroy()  # закрывает всё приложение
        sys.exit(1)

    copy_btn = ctk.CTkButton(
        btn_frame,
        text="Copy",
        width=100,
        command=copy_to_clipboard
    )
    copy_btn.pack(side="left", padx=5)

    close_btn = ctk.CTkButton(
        btn_frame,
        text="Close",
        width=100,
        command=close
    )
    close_btn.pack(side="left", padx=5)

    exit_btn = ctk.CTkButton(
        btn_frame,
        text="Exit",
        width=100,
        fg_color="#e74c3c",
        hover_color="#c0392b",
        command=exit_app
    )
    exit_btn.pack(side="right", padx=5)

    # ===== Авторазмер =====
    dialog.update_idletasks()
    dialog.geometry("650x420")

    return dialog

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