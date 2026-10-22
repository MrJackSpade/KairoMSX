#!/usr/bin/env python3
"""Writes kairomsx/src/main/assets/system_roms.json: the system ROMs of the real MSX machines
KairoMSX offers, read from openMSX's machine definitions (third_party/openmsx/share/machines).

Usage: system_roms.py [--check]   (--check fails when the file is out of date)

Each machine lists its ROM files by the file name openMSX gives them and the SHA-1 digests of good
dumps; the app imports a player's dumps by digest into openMSX's systemroms folder, where openMSX
finds them by digest whatever their name.
"""
import json
import sys
import xml.etree.ElementTree as ElementTree
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MACHINES = ROOT / 'third_party/openmsx/share/machines'
OUT = ROOT / 'kairomsx/src/main/assets/system_roms.json'

# The machines offered for disk and tape games (MsxCatalog), by openMSX name, with their labels.
OFFERED = [
    ('Panasonic_FS-A1WSX', 'Panasonic FS-A1WSX (MSX2+)'),
    ('Panasonic_FS-A1GT', 'Panasonic FS-A1GT (turbo R)'),
    ('Philips_NMS_8250', 'Philips NMS 8250 (MSX2)'),
]


def roms(name):
    """The machine's ROM files: (file name, sorted digests), each once."""
    found = {}
    for rom in ElementTree.parse(MACHINES / f'{name}.xml').iter('rom'):
        digests = tuple(sorted(d.text.strip().lower() for d in rom.findall('sha1')))
        if not digests:
            continue  # a part of another ROM file, as a window into it
        files = [f.text.strip() for f in rom.findall('filename')]
        if not files:
            raise SystemExit(f'{name}: a ROM with digests but no file name')
        found.setdefault(digests, files[0])
    return [{'file': file, 'sha1': list(digests)} for digests, file in found.items()]


def content():
    machines = [{'machine': name, 'label': label, 'roms': roms(name)} for name, label in OFFERED]
    return json.dumps({'machines': machines}, indent=1) + '\n'


def main():
    text = content()
    if sys.argv[1:] == ['--check']:
        if OUT.read_text(encoding='utf-8').replace('\r\n', '\n') != text:
            raise SystemExit(f'{OUT.relative_to(ROOT)} is out of date: run tools/system_roms.py')
        return
    OUT.write_text(text, encoding='utf-8', newline='\n')
    print(f'wrote {OUT.relative_to(ROOT)}')


if __name__ == '__main__':
    main()
