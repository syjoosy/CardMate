import sys

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
    return "unknown"

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
        return []

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
        raise Exception("Unsupported OS")


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
        raise Exception("Unsupported OS")


def _backup_macos(source, destination):
    if source.startswith("/dev/disk"):
        source = source.replace("/dev/disk", "/dev/rdisk")

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