import logging
import sys
import os
import threading
import traceback
from datetime import datetime
import tkinter as tk

from ui.dialog import show_dialog, show_error_dialog

DEBUG = "DEBUG"
INFO = "INFO"
WARNING = "WARNING"
ERROR = "ERROR"
CRITICAL = "CRITICAL"
SUCCESS = "SUCCESS"

SUCCESS_LEVEL_NUM = 35
logging.addLevelName(SUCCESS_LEVEL_NUM, SUCCESS)

MAC_OS_LOG_PATH = "~/Library/Logs/CardMate/"
LINUX_LOG_PATH = "/var/log/CardMate/"
WINDOWS_LOG_PATH = "~/Library/Logs/CardMate/"

if sys.platform == "darwin":
    LOG_DIR = os.path.expanduser(MAC_OS_LOG_PATH)
elif sys.platform.startswith("win"):
    LOG_DIR = os.path.expanduser(WINDOWS_LOG_PATH)
elif sys.platform.startswith("linux"):
    LOG_DIR = os.path.expanduser(LINUX_LOG_PATH)
else:
    LOG_DIR = os.path.expanduser(LINUX_LOG_PATH)

if not os.path.exists(LOG_DIR):
    try:
        os.makedirs(LOG_DIR)
    except Exception as e:
        print(f"ERROR: {e}")

log_filename = f"cardMate_{datetime.now().strftime('%Y-%m-%d_%H-%M-%S')}.log"
log_filepath = os.path.join(LOG_DIR, log_filename)

# ---------- COLORS ----------
class Colors:
    RESET = '\033[0m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    GREEN = '\033[92m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'

class ColoredFormatter(logging.Formatter):
    LEVEL_COLORS = {
        logging.DEBUG: Colors.BLUE,
        logging.INFO: Colors.CYAN,
        logging.WARNING: Colors.YELLOW,
        logging.ERROR: Colors.RED,
        logging.CRITICAL: Colors.MAGENTA,
        SUCCESS_LEVEL_NUM: Colors.GREEN,
    }

    def format(self, record):
        original_levelname = record.levelname
        color = self.LEVEL_COLORS.get(record.levelno, Colors.RESET)
        record.levelname = f"{color}{record.levelname}{Colors.RESET}"
        formatted_message = super().format(record)
        record.levelname = original_levelname
        return formatted_message

# ---------- LOGGER ----------
def success(self, message, *args, **kws):
    if self.isEnabledFor(SUCCESS_LEVEL_NUM):
        self._log(SUCCESS_LEVEL_NUM, message, args, **kws)

logging.Logger.success = success

logger = logging.getLogger("CardMate")
logger.setLevel(logging.DEBUG)

console_handler = logging.StreamHandler(sys.stdout)
console_handler.setLevel(logging.DEBUG)
console_handler.setFormatter(
    ColoredFormatter('%(asctime)s [%(levelname)s] %(message)s',
                     datefmt='%Y-%m-%d %H:%M:%S')
)

file_handler = logging.FileHandler(log_filepath, mode='a', encoding='utf-8')
file_handler.setLevel(logging.DEBUG)
file_handler.setFormatter(
    logging.Formatter('%(asctime)s [%(levelname)s] %(message)s',
                      datefmt='%Y-%m-%d %H:%M:%S')
)

logger.addHandler(console_handler)
logger.addHandler(file_handler)
logger.propagate = False

# ---------- EXCEPTION HOOKS ----------

# 🔹 Главный поток
def handle_exception(exc_type, exc_value, exc_traceback):
    if issubclass(exc_type, KeyboardInterrupt):
        sys.__excepthook__(exc_type, exc_value, exc_traceback)
        return

    logger.critical(
        "Uncaught exception",
        exc_info=(exc_type, exc_value, exc_traceback)
    )

sys.excepthook = handle_exception


# 🔹 Потоки
def thread_exception_handler(args):
    logger.critical(
        f"Uncaught exception in thread: {args.thread.name}",
        exc_info=(args.exc_type, args.exc_value, args.exc_traceback)
    )

threading.excepthook = thread_exception_handler


# 🔹 Tkinter / CustomTkinter
def tk_exception_handler(self, exc, val, tb):
    logger.critical(
        "Tkinter callback exception",
        exc_info=(exc, val, tb)
    )

    show_error_dialog(self, "Critical error", val, tb)

tk.Tk.report_callback_exception = tk_exception_handler

# ---------- API ----------
def log_message(severity, message):
    if severity == DEBUG:
        logger.debug(message)
    elif severity == INFO:
        logger.info(message)
    elif severity == WARNING:
        logger.warning(message)
    elif severity == ERROR:
        logger.error(message)
    elif severity == CRITICAL:
        logger.critical(message)
    elif severity == SUCCESS:
        logger.success(message)

# ---------- START LOG ----------
if sys.platform == "darwin":
    logger.info("Detected OS: MacOS")
elif sys.platform.startswith("win"):
    logger.info("Detected OS: Windows")
elif sys.platform.startswith("linux"):
    logger.info("Detected OS: Linux")
else:
    logger.info("Detected OS: Unknown")

logger.info("Using log path: " + LOG_DIR)