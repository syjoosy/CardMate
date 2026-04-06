import logging
import sys
import os
from datetime import datetime

DEBUG = "DEBUG"
INFO = "INFO"
WARNING = "WARNING"
ERROR = "ERROR"
CRITICAL = "CRITICAL"
SUCCESS = "SUCCESS"  # Новый уровень

# Числовое значение для SUCCESS (между WARNING=30 и ERROR=40)
SUCCESS_LEVEL_NUM = 35

# Регистрируем новый уровень в logging
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

# Создаем папку logs, если её нет
if not os.path.exists(LOG_DIR):
    try:
        os.makedirs(LOG_DIR)
    except Exception as e:
        print(f"ERROR: {e}")

# Формируем имя файла с текущей датой и временем
# Формат: cardMate_2024-01-15_14-30-45.log
log_filename = f"cardMate_{datetime.now().strftime('%Y-%m-%d_%H-%M-%S')}.log"
log_filepath = os.path.join(LOG_DIR, log_filename)

# ANSI color codes
class Colors:
    RESET = '\033[0m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    GREEN = '\033[92m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'

# Custom formatter for colored console output
class ColoredFormatter(logging.Formatter):
    # Map log level to color
    LEVEL_COLORS = {
        logging.DEBUG: Colors.BLUE,
        logging.INFO: Colors.CYAN,
        logging.WARNING: Colors.YELLOW,
        logging.ERROR: Colors.RED,
        logging.CRITICAL: Colors.MAGENTA,
        SUCCESS_LEVEL_NUM: Colors.GREEN,  # SUCCESS зеленый
    }

    def format(self, record):
        # Save original levelname
        original_levelname = record.levelname
        
        # Add color to levelname
        color = self.LEVEL_COLORS.get(record.levelno, Colors.RESET)
        record.levelname = f"{color}{record.levelname}{Colors.RESET}"
        
        # Format the message
        formatted_message = super().format(record)
        
        # Restore original levelname
        record.levelname = original_levelname
        
        return formatted_message

# Создаем метод success для логгера
def success(self, message, *args, **kws):
    if self.isEnabledFor(SUCCESS_LEVEL_NUM):
        self._log(SUCCESS_LEVEL_NUM, message, args, **kws)

# Добавляем метод success к классу Logger
logging.Logger.success = success

# Create logger
logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

# Console handler (with colors)
console_handler = logging.StreamHandler(sys.stdout)
console_handler.setLevel(logging.DEBUG)

# Console formatter with colors
console_formatter = ColoredFormatter('%(asctime)s [%(levelname)s] %(message)s', 
                                      datefmt='%Y-%m-%d %H:%M:%S')
console_handler.setFormatter(console_formatter)

# File handler (without colors) - теперь с динамическим именем
file_handler = logging.FileHandler(log_filepath, mode='a', encoding='utf-8')
file_handler.setLevel(logging.DEBUG)

# File formatter (plain text)
file_formatter = logging.Formatter('%(asctime)s [%(levelname)s] %(message)s',
                                   datefmt='%Y-%m-%d %H:%M:%S')
file_handler.setFormatter(file_formatter)

# Add both handlers to logger
logger.addHandler(console_handler)
logger.addHandler(file_handler)

# Optional: prevent log messages from propagating to root logger
logger.propagate = False

# Your original function (обновленная с поддержкой SUCCESS)
def log_message(severity, message):
    if severity == "DEBUG":
        logger.debug(message)
    elif severity == "INFO":
        logger.info(message)
    elif severity == "WARNING":
        logger.warning(message)
    elif severity == "ERROR":
        logger.error(message)
    elif severity == "CRITICAL":
        logger.critical(message)
    elif severity == "SUCCESS":  # Добавляем обработку SUCCESS
        logger.success(message)

# Создаем папку logs, если её нет
if sys.platform == "darwin":
    logger.info("Detected OS: MacOS")
elif sys.platform.startswith("win"):
    logger.info("Detected OS: Windows")
elif sys.platform.startswith("linux"):
    logger.info("Detected OS: Linux")
else:
    logger.info("Detected OS: Unknown")

logger.info("Using log path: " + LOG_DIR)