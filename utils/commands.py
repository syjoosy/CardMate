import sys
from utils.disks import _get_macos_disks, _get_linux_disks

# =========================
# OS DETECTION
# =========================
def get_platform():
    if sys.platform == "darwin":
        return "macos"
    elif sys.platform.startswith("linux"):
        return "linux"
    elif sys.platform.startswith("win"):
        return "windows"
    raise Exception("Unknown OS! Cant get platform!")

# =========================
# FILESYSTEMS
# =========================
def get_filesystems():
    platform = get_platform()

    if platform == "macos":
        return _fs_macos()
    elif platform == "linux":
        return _fs_linux()
    elif platform == "windows":
        return _fs_windows()
    else:
        raise Exception("Unknown OS! Cant get filesystems list!")

def _fs_macos():
    return ["APFS", "JHFS+", "ExFAT", "FAT32"]

def _fs_linux():
    return ["ext4", "ext3", "ext2", "vfat", "ntfs", "exfat"]

def _fs_windows():
    return ["NTFS", "exFAT", "FAT32"]

# =========================
# GET DISK LIST
# =========================
def get_disk_list():
    platform = get_platform()

    if platform == "macos":
        return _get_macos_disks()
    elif platform == "linux":
        return _get_linux_disks()
    elif platform == "windows":
        return _get_windows_disks()
    else:
        raise Exception("Unknown OS! Cant get disk list!")


def _get_windows_disks():
    # Используем PowerShell
    return [
        "powershell",
        "-Command",
        "Get-Disk | Select-Object Number, FriendlyName, Size"
    ]

# =========================
# UNMOUNT DISK
# =========================
def unmount_disk(disk):
    platform = get_platform()

    if platform == "macos":
        return ["diskutil", "unmountDisk", disk]

    elif platform == "linux":
        return ["bash", "-c", f"umount {disk}*"]

    elif platform == "windows":
        return [
            "powershell",
            "-Command",
            f"Get-Disk {disk} | Set-Disk -IsOffline $true"
        ]

    else:
        raise Exception("Unknown OS! Cant unmount disk!")

# =========================
# FLASH (DD WRITE)
# =========================
def flash_disk(image, target):
    platform = get_platform()

    if platform == "macos":
        return _flash_macos(image, target)
    elif platform == "linux":
        return _flash_linux(image, target)
    elif platform == "windows":
        return _flash_windows(image, target)
    else:
        raise Exception("Unknown OS! Cant flash disk!")


def _flash_macos(image, target):
    return ["dd", f"if={image}", f"of={target}", "bs=4m", "status=progress"]

def _flash_linux(image, target):
    return ["dd", f"if={image}", f"of={target}", "bs=4M", "status=progress"]

def _flash_windows(image, target):
    # Windows через WSL или dd (если установлен)
    return [
        "powershell",
        "-Command",
        f"wsl dd if={image} of={target} bs=4M status=progress"
    ]

# =========================
# FORMAT DISK
# =========================
def format_disk(disk, filesystem, volume_name):
    platform = get_platform()

    if platform == "macos":
        return _format_macos(disk, filesystem, volume_name)
    elif platform == "linux":
        return _format_linux(disk, filesystem, volume_name)
    elif platform == "windows":
        return _format_windows(disk, filesystem, volume_name)
    else:
        raise Exception("Unknown OS! Cant format disk!")


def _format_macos(disk, filesystem, volume_name):
    return ["diskutil", "eraseDisk", filesystem, volume_name, disk]


def _format_linux(disk, filesystem, volume_name):
    if filesystem.startswith("ext"):
        return ["mkfs", f"-t{filesystem}", disk]
    elif filesystem == "vfat" or filesystem == "fat32":
        return ["mkfs.vfat", disk]
    elif filesystem == "ntfs":
        return ["mkfs.ntfs", disk]
    elif filesystem == "exfat":
        return ["mkfs.exfat", disk]
    else:
        raise Exception(f"Unsupported FS: {filesystem}")


def _format_windows(disk, filesystem, volume_name):
    # diskpart script inline
    script = f"""
    select disk {disk}
    clean
    create partition primary
    format fs={filesystem} quick label={volume_name}
    assign
    """

    return [
        "powershell",
        "-Command",
        f"$script = @\"\n{script}\n\"@; $script | diskpart"
    ]

# =========================
# BACKUP (DD)
# =========================
def create_backup(source, destination):
    platform = get_platform()

    if platform == "macos":
        return _backup_macos(source, destination)
    elif platform == "linux":
        return _backup_linux(source, destination)
    elif platform == "windows":
        return _backup_windows(source, destination)
    else:
        raise Exception("Unknown OS! Cant create backup!")


def _backup_macos(source, destination):
    return [
        "dd",
        f"if={source}",
        f"of={destination}",
        "status=progress",
        "conv=noerror,sync",
    ]


def _backup_linux(source, destination):
    return [
        "dd",
        f"if={source}",
        f"of={destination}",
        "status=progress",
    ]


def _backup_windows(source, destination):
    return [
        "powershell",
        "-Command",
        f"wsl dd if={source} of={destination} bs=4M status=progress"
    ]