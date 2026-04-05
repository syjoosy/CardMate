import os


def get_default_paths():
    home = os.path.expanduser("~")

    return {
        "Home": home,
        "Documents": os.path.join(home, "Documents"),
        "Downloads": os.path.join(home, "Downloads"),
        "Desktop": os.path.join(home, "Desktop"),
    }