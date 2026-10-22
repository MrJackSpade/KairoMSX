"""Lists the scanned cartridges that hold an MSX-BASIC program, for catalog/research/basic-cartridges-v1.json.

A cartridge's header is "AB", then its INIT, STATEMENT, DEVICE and TEXT addresses. A BASIC
cartridge has no INIT routine and a TEXT address: the MSX-BASIC interpreter runs the program it
points at. C-BIOS has no BASIC, so these games need a machine with real MSX firmware (catalog_config.MACHINE).

    python tools/basic_cartridges.py
"""
import json
import os
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCANS = ["msx", "msx2", "turbor"]


def basic(data):
    if len(data) < 16:
        return False
    # The header sits at the start of the first 16 KiB page that has one.
    for page in range(0, min(len(data), 0x10000), 0x4000):
        header = data[page:page + 10]
        if header[:2] == b"AB":
            init = header[2] | header[3] << 8
            text = header[8] | header[9] << 8
            return init == 0 and text != 0
    return False


def main():
    found = {}
    for name in SCANS:
        index = json.loads((ROOT / f".downloads/scan/{name}.json").read_text(encoding="utf-8"))
        for archive in index["archives"]:
            if archive["status"] != "ready":
                continue
            roms = [disk for disk in archive["disks"] if disk["contentId"].startswith("sha256-rom-v1:")]
            if not roms:
                continue
            path = os.path.join(index["root"], archive["path"])
            with zipfile.ZipFile(path) as source:
                for disk in roms:
                    if basic(source.read(disk["entry"])):
                        found[disk["contentId"]] = f"{name}:{archive['path']}"
    output = ROOT / "catalog/research/basic-cartridges-v1.json"
    output.write_text(json.dumps({"schemaVersion": 1, "cartridges": dict(sorted(found.items()))},
                                 ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    print(f"{len(found)} BASIC cartridges")


if __name__ == "__main__":
    main()
