"""Exercise the display readiness gate without display hardware or root access."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "files" / "wait-for-display.sh"


class DisplayTest(unittest.TestCase):
    def check_display(self, status, modes, device, expected):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            connector = root / "sys" / "card0-HDMI-A-1"
            connector.mkdir(parents=True)
            (connector / "status").write_text(status + "\n")
            (connector / "modes").write_text(modes)
            devices = root / "dev"
            devices.mkdir()
            if device:
                (devices / "card0").symlink_to("/dev/null")
            env = dict(os.environ, DRM_SYSFS_ROOT=str(root / "sys"),
                       DRM_DEVICE_ROOT=str(devices), DISPLAY_WAIT_SECONDS="0")
            result = subprocess.run(["sh", str(SCRIPT)], env=env,
                                    capture_output=True, text=True, timeout=5)
            self.assertEqual(result.returncode, expected, result.stderr)

    def test_connected_with_mode_and_device(self):
        self.check_display("connected", "1920x1080\n", True, 0)

    def test_disconnected(self):
        self.check_display("disconnected", "1920x1080\n", True, 1)

    def test_empty_modes(self):
        self.check_display("connected", "", True, 1)

    def test_missing_device(self):
        self.check_display("connected", "1920x1080\n", False, 1)


if __name__ == "__main__":
    unittest.main()
