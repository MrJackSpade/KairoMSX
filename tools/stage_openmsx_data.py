#!/usr/bin/env python3
"""Assembles openMSX's data folder (its installed share/) for KairoMSX.

Usage: stage_openmsx_data.py DEST

DEST is replaced by third_party/openmsx/share with the C-BIOS machines and ROMs in machines/, as
openMSX's install (build/install.py) lays them out. The OpenGL shaders, the on-screen display
skins and the window icons are left out: KairoMSX draws none of them. The host tests and the
Android app's assets use this folder as OPENMSX_SYSTEM_DATA.

DEST/STAMP names the folder's contents (a SHA-256 over every file's path and bytes), so the app
can tell when the copy it made is out of date.
"""
import hashlib
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OPENMSX = ROOT / 'third_party/openmsx'
LEFT_OUT = {'shaders', 'skins', 'icons'}


def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    dest = Path(sys.argv[1]).resolve()
    if dest.exists():
        shutil.rmtree(dest)
    shutil.copytree(OPENMSX / 'share', dest, ignore=lambda folder, names: [
        name for name in names if Path(folder) == OPENMSX / 'share' and name in LEFT_OUT])
    for item in (OPENMSX / 'Contrib/cbios').iterdir():
        shutil.copy2(item, dest / 'machines' / item.name)
    stamp = hashlib.sha256()
    for path in sorted(p for p in dest.rglob('*') if p.is_file()):
        stamp.update(path.relative_to(dest).as_posix().encode() + b'\0')
        stamp.update(path.read_bytes())
    (dest / 'STAMP').write_text(stamp.hexdigest() + '\n', encoding='ascii')


if __name__ == '__main__':
    main()
