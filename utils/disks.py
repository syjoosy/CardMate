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