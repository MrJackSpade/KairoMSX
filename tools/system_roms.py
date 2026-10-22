#!/usr/bin/env python3
"""Writes kairomsx/src/main/assets/system_roms.json: the system ROMs of the real MSX machines
KairoMSX offers, read from openMSX's machine definitions (third_party/openmsx/share/machines).

Usage: system_roms.py [--check]   (--check fails when the file is out of date)

Each machine lists its ROM files by the file name openMSX gives them and the SHA-1 digests of good
dumps; the app imports a player's dumps by digest into openMSX's systemroms folder, where openMSX
finds them by digest whatever their name. A device's <rom> entries are alternatives, of which
openMSX loads the first it finds (such as one whole-EPROM dump or the separate ROMs it holds), so
each device is one requirement, met by any of its files.
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


def machine(name, label):
    """The machine's ROM files, each once with its digests, and its requirements: for each device,
    the files any one of which it loads."""
    files, requirements = {}, []
    for device in ElementTree.parse(MACHINES / f'{name}.xml').iter():
        options = []
        for rom in device.findall('rom'):
            digests = tuple(sorted(d.text.strip().lower() for d in rom.findall('sha1')))
            if not digests:
                continue  # a part of another ROM file, as a window into it
            names = [f.text.strip() for f in rom.findall('filename')]
            if not names:
                raise SystemExit(f'{name}: a ROM with digests but no file name')
            options.append(files.setdefault(digests, names[0]))
        options = sorted(set(options))
        if options and options not in requirements:
            requirements.append(options)
    return {'machine': name, 'label': label,
            'roms': [{'file': file, 'sha1': list(digests)} for digests, file in files.items()],
            'requirements': requirements}


def content():
    return json.dumps({'machines': [machine(name, label) for name, label in OFFERED]}, indent=1) + '\n'


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
