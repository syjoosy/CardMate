import subprocess
from utils.logging import log_message, DEBUG, INFO, WARNING, ERROR, SUCCESS, CRITICAL

def _get_macos_disks():
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

def _get_linux_disks():
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

def _get_windows_disks():
    log_message(INFO, "Start disk refresh!")
    disks = []

    # Используем wmic для получения информации о дисках
    result = subprocess.run(
        ["wmic", "diskdrive", "get", "DeviceID,Model,Size,InterfaceType,MediaType", "/format:csv"],
        capture_output=True,
        text=True
    )

    lines = result.stdout.splitlines()

    for line in lines[1:]:  # Пропускаем заголовок CSV
        if not line.strip():
            continue

        parts = line.split(',')
        if len(parts) < 5:
            continue

        # Формат CSV от wmic: Node,DeviceID,InterfaceType,MediaType,Model,Size
        # Индексы могут варьироваться, ищем DeviceID
        node = parts[0]
        device_id = parts[1] if len(parts) > 1 else ""
        interface_type = parts[2] if len(parts) > 2 else ""
        media_type = parts[3] if len(parts) > 3 else ""
        model = parts[4] if len(parts) > 4 else ""
        size_bytes = parts[5] if len(parts) > 5 else ""

        if not device_id:
            continue

        # Преобразуем размер из байт в человекочитаемый формат
        size = "Unknown"
        if size_bytes and size_bytes.isdigit():
            size_bytes_int = int(size_bytes)
            for unit in ['B', 'KB', 'MB', 'GB', 'TB']:
                if size_bytes_int < 1024.0:
                    size = f"{size_bytes_int:.1f} {unit}"
                    break
                size_bytes_int /= 1024.0

        # Определяем тип диска (внутренний/внешний)
        disk_type = "internal"
        if interface_type.lower() == "usb" or "usb" in media_type.lower():
            disk_type = "external"
        elif "removable" in media_type.lower():
            disk_type = "external"

        display = f"{device_id} ({model}, {size}, {disk_type} {interface_type})"
        disks.append((display, device_id))

    # Альтернативный метод через PowerShell (более детальная информация)
    if not disks:
        log_message(INFO, "Trying PowerShell method...")
        ps_script = """
        Get-PhysicalDisk | Select-Object FriendlyName, DeviceId, Size, MediaType, BusType | ConvertTo-Csv -NoTypeInformation
        """
        result = subprocess.run(
            ["powershell", "-Command", ps_script],
            capture_output=True,
            text=True
        )

        lines = result.stdout.splitlines()
        for line in lines[1:]:  # Пропускаем заголовок
            if not line.strip():
                continue

            parts = line.split(',')
            if len(parts) < 5:
                continue

            friendly_name = parts[0].strip('"')
            device_id = parts[1].strip('"')
            size_bytes = parts[2].strip('"')
            media_type = parts[3].strip('"')
            bus_type = parts[4].strip('"')

            size = "Unknown"
            if size_bytes and size_bytes.isdigit():
                size_bytes_int = int(size_bytes)
                for unit in ['B', 'KB', 'MB', 'GB', 'TB']:
                    if size_bytes_int < 1024.0:
                        size = f"{size_bytes_int:.1f} {unit}"
                        break
                    size_bytes_int /= 1024.0

            disk_type = "internal" if media_type != "USB" else "external"
            display = f"{device_id} ({friendly_name}, {size}, {disk_type} {bus_type})"
            disks.append((display, device_id))

    log_message(INFO, "Disks: ")
    log_message(INFO, disks)
    return disks