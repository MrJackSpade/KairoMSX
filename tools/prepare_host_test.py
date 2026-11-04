#!/usr/bin/env python3
"""What KairoMSX's core reads that the app's build makes, for the shared PC boot test (Kairo#115):
openMSX's data folder, as the build stages it into the app's assets (stage_openmsx_data.py), at
<host data>/openmsx, which core/test/boot-tests.json installs as the core's home "system"; and the
hardDisk check's empty hard disk, <host data>/hdtest.dsk and hdtest2.dsk (an MBR with one FAT16 partition, the
shared fat_disk.py), which Nextor on the Sunrise IDE mounts.

    prepare_host_test.py <host data>

shared/tools/test-core.ps1 runs it before the test; the folder is staged afresh each time.
"""
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "shared/tools"))
from fat_disk import mbr_fat16


def main(argv):
    if len(argv) != 2:
        raise SystemExit("usage: prepare_host_test.py <host data>")
    destination = Path(argv[1]) / "openmsx"
    subprocess.run([sys.executable, "-I", str(ROOT / "tools/stage_openmsx_data.py"), str(destination)], check=True)
    # A second, for the stateDisks check to swap to: a running MSX's hard disk cannot be taken out.
    for name in ("hdtest.dsk", "hdtest2.dsk"):
        (Path(argv[1]) / name).write_bytes(mbr_fat16(16))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
