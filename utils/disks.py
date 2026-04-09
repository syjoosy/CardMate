import subprocess
from utils.logging import log_message, DEBUG, INFO, WARNING, ERROR, SUCCESS, CRITICAL

def get_macos_disks():
    log_message(INFO, "Start disk refresh!")
    disks = []

    result = subprocess.run(
        ["diskutil", "list"],
        capture_output=True,
        text=True
    )

    for line in result.stdout.splitlines():
        line = line.strip()

        if line.startswith("/dev/disk"):
            disk_name = line.split()[0]

            info = subprocess.run(
                ["diskutil", "info", disk_name],
                capture_output=True,
                text=True
            ).stdout

            size = "Unknown"
            protocol = ""
            internal = ""

            for info_line in info.splitlines():
                if "Disk Size:" in info_line:
                    size = info_line.split(":")[1].split("(")[0].strip()
                elif "Protocol:" in info_line:
                    protocol = info_line.split(":")[1].strip()
                elif "Internal:" in info_line:
                    internal = info_line.split(":")[1].strip()

            disk_type = "internal" if internal == "Yes" else "external"

            display = f"{disk_name} ({size}, {disk_type} {protocol})"
            disks.append((display, disk_name))
    
    log_message(INFO, "Disks: ")
    log_message(INFO, disks)
    return disks

def get_linux_disks():
    log_message(INFO, "Start disk refresh!")
    disks = []

    result = subprocess.run(
        ["lsblk", "-d", "-o", "NAME,SIZE,TYPE,TRAN"],
        capture_output=True,
        text=True
    )

    lines = result.stdout.splitlines()

    # Пропускаем заголовок
    for line in lines[1:]:
        parts = line.split()

        if len(parts) < 3:
            continue

        name, size, dev_type = parts[:3]
        tran = parts[3] if len(parts) > 3 else ""

        # Берем только реальные диски (не разделы)
        if dev_type != "disk":
            continue

        disk_name = f"/dev/{name}"

        # Определяем тип (внутренний/внешний условно)
        if tran in ["usb"]:
            disk_type = "external"
        else:
            disk_type = "internal"

        display = f"{disk_name} ({size}, {disk_type} {tran})"
        disks.append((display, disk_name))

    log_message(INFO, "Disks:")
    log_message(INFO, disks)

    return disks