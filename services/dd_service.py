import subprocess
import os
import sys


class DDService:
    def __init__(self):
        self.process = None

    def build_command(self, source, dest):
        if sys.platform == "darwin" and source.startswith("/dev/disk"):
            source = source.replace("/dev/disk", "/dev/rdisk")

        return [
            "dd",
            f"if={source}",
            f"of={dest}",
            "status=progress",
            "conv=noerror,sync",
        ]

    def run(self, source, dest, log_callback, done_callback):
        try:
            cmd = self.build_command(source, dest)

            self.process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )

            while True:
                if self.process.poll() is not None:
                    done_callback()
                    break

                line = self.process.stdout.readline()
                if line:
                    log_callback(line.strip())

        except Exception as e:
            log_callback(f"Ошибка: {e}")

    def stop(self):
        if self.process:
            self.process.terminate()