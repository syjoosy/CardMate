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
    return [
        "APFS",
        "JHFS+",      # Mac OS Extended (Journaled)
        "ExFAT",
        "FAT32",
    ]

def _fs_linux():
    return [
        "ext4",
        "ext3",
        "ext2",
        "vfat",
        "ntfs",
        "exfat",
    ]

def _fs_windows():
    return [
        "NTFS",
        "exFAT",
        "FAT32",
    ]

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
        raise NotImplementedError("Windows get disk not implemented yet")

    else:
        raise Exception("Unknown OS! Cant get disk list!")

# =========================
# UNMOUNT DISK
# =========================
def unmount_disk(disk):
    platform = get_platform()

    if platform == "macos":
        return ["diskutil", "unmountDisk", disk]

    elif platform == "linux":
        # размонтировать все разделы
        return ["bash", "-c", f"umount {disk}*"]

    elif platform == "windows":
        raise NotImplementedError("Unmount not implemented for Windows")

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
        raise NotImplementedError("Flash not implemented for Windows")

    else:
        raise Exception("Unknown OS! Cant flash disk!")


def _flash_macos(image, target):
    return [
        "dd",
        f"if={image}",
        f"of={target}",
        "bs=4m",
        "status=progress",
    ]


def _flash_linux(image, target):
    return [
        "dd",
        f"if={image}",
        f"of={target}",
        "bs=4M",
        "status=progress",
    ]

# =========================
# FORMAT DISK
# =========================
def format_disk(disk, filesystem, volume_name):
    platform = get_platform()

    if platform == "macos":
        return _format_macos(disk, filesystem, volume_name)

    elif platform == "linux":
        raise NotImplementedError("Linux format not implemented yet")

    elif platform == "windows":
        raise NotImplementedError("Windows format not implemented yet")

    else:
        raise Exception("Unknown OS! Cant format disk!")


def _format_macos(disk, filesystem, volume_name):
    return [
        "diskutil",
        "eraseDisk",
        filesystem,
        volume_name,
        disk,
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
        raise NotImplementedError("Windows backup not implemented yet")

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