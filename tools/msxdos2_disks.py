#!/usr/bin/env python3
"""Lists the scanned disks that start from MSX-DOS2, for catalog_config.MACHINE.

Usage: msxdos2_disks.py

Reads every disk of the scan indexes (catalog_config.SCANS) from its set's folder and writes the
content IDs of those whose root folder holds MSXDOS2.SYS to catalog/research/msxdos2-disks-v1.json,
beside the disks listed by hand ("manual": {content ID: why}), which are kept. Such a disk needs
the turbo R, the only machine here with the MSX-DOS2 kernel, and a DOS2-format boot sector, which
the app writes into the working copy (MsxDos2.kt). Without it the kernel starts Disk BASIC.
"""
import json
import struct
import zipfile
from pathlib import Path

from catalog_config import ROOT, SCANS

OUTPUT = ROOT / "catalog/research/msxdos2-disks-v1.json"


def root_names(image):
    """The 8.3 names in a FAT12 disk image's root folder, or None for a disk with no FAT file
    system (a game's own format), which cannot hold MSXDOS2.SYS."""
    bps, spc, res, nfat, nroot, total, media, spf = struct.unpack_from("<HBHBHHBH", image, 11)
    if bps != 512 or not 1 <= nfat <= 2 or not 0 < nroot <= 512 or spf == 0:
        return None
    start = (res + nfat * spf) * bps
    names = set()
    for index in range(nroot):
        entry = image[start + index * 32:start + index * 32 + 32]
        if len(entry) < 32 or entry[0] == 0:
            break
        if entry[0] == 0xE5 or entry[11] & 0x08:
            continue
        names.add(entry[:8].decode("latin-1").rstrip() + "." + entry[8:11].decode("latin-1").rstrip())
    return names


def main():
    previous = json.loads(OUTPUT.read_text(encoding="utf-8")) if OUTPUT.is_file() else {"manual": {}}
    found = {}
    unformatted = 0
    for set_name, (index_path, _) in SCANS.items():
        index = json.loads((ROOT / index_path).read_text(encoding="utf-8"))
        root = Path(index["root"])
        for archive in index["archives"]:
            disks = [disk for disk in archive["disks"] if disk["contentId"].startswith("sha256-fd-v1:")]
            if archive["status"] != "ready" or not disks:
                continue
            with zipfile.ZipFile(root / archive["path"]) as source:
                for disk in disks:
                    names = root_names(source.read(disk["entry"]))
                    if names is None:
                        unformatted += 1
                    elif "MSXDOS2.SYS" in names:
                        found[disk["contentId"]] = f"{set_name}:{archive['path']}"
    result = {"schemaVersion": 1, "manual": previous["manual"], "found": dict(sorted(found.items()))}
    OUTPUT.write_text(json.dumps(result, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"{len(found)} disks hold MSXDOS2.SYS; {len(previous['manual'])} listed by hand; "
          f"{unformatted} disks have no FAT file system")


if __name__ == "__main__":
    main()
