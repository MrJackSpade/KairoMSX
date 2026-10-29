#!/usr/bin/env python3
"""Writes kairomsx/src/main/assets/system_roms.json: the system ROMs of the real MSX machines
KairoMSX offers and of the extensions it plugs in, read from openMSX's definitions
(third_party/openmsx/share/machines and extensions).

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
SHARE = ROOT / 'third_party/openmsx/share'
OUT = ROOT / 'kairomsx/src/main/assets/system_roms.json'
# The machines offered for disk and tape games (MsxCatalog), and the extensions the app plugs in,
# by openMSX's folder and name, with their labels and a note on what each is used for.
OFFERED = [
    ('machines', 'Panasonic_FS-A1WSX', 'Panasonic FS-A1WSX (MSX2+)',
     'Plays disk and tape games. Cartridge games need no BIOS: they run on the built-in C-BIOS.'),
    ('machines', 'Panasonic_FS-A1GT', 'Panasonic FS-A1GT (turbo R)', 'Plays turbo R games.'),
    ('machines', 'Philips_NMS_8250', 'Philips NMS 8250 (MSX2)', 'Not needed: used only when a game\'s Machine setting picks it.'),
    # A hard disk goes on this IDE interface (msx_machine.cpp kHardDiskExtension).
    ('extensions', 'SunriseIDE_Nextor', 'Sunrise IDE (Nextor)', 'Plays hard disk images.'),
]
# Each system ROM file's name in the app and what it does. Every offered file must have one.
DESCRIPTIONS = {
    'fs-a1wsx_IC18.rom': ('Whole system chip', 'one dump of the whole system chip; stands in for the BIOS, music, sub and disk files'),
    'fs-a1wsx_basic-bios2p.rom': ('BIOS', 'the MSX2+ BIOS and BASIC that start the machine'),
    'fs-a1wsx_fmbasic.rom': ('Music BASIC', 'MSX-MUSIC, the FM sound BASIC'),
    'fs-a1wsx_msx2psub.rom': ('Sub-BIOS', 'the MSX2+ sub-BIOS for graphics and setup'),
    'fs-a1wsx_kanjibasic.rom': ('Kanji BASIC', 'Kanji BASIC for Japanese text'),
    'fs-a1wsx_disk.rom': ('Disk', 'runs the floppy drive'),
    'fs-a1wsx_firmware.rom': ('Firmware', 'the built-in firmware and its menu'),
    'fs-a1wsx_kanjifont.rom': ('Kanji font', 'the Japanese character font'),
    'fs-a1gt_firmware.rom': ('System', 'the whole turbo R system: BIOS, BASIC, disk and music'),
    'fs-a1gt_kanjifont.rom': ('Kanji font', 'the Japanese character font'),
    'nms8250_basic-bios2.rom': ('BIOS', 'the MSX2 BIOS and BASIC that start the machine'),
    'nms8250_msx2sub.rom': ('Sub-BIOS', 'the MSX2 sub-BIOS for graphics'),
    'nms8250_disk.rom': ('Disk', 'runs the floppy drive'),
    'Nextor-2.1.1.SunriseIDE.ROM': ('IDE ROM', 'Nextor, which runs the IDE interface and starts hard disks'),
}


def machine(folder, name, label, note):
    """The machine's or extension's ROM files, each once with its digests, and its requirements: for
    each device, the files any one of which it loads."""
    files, requirements = {}, []
    for device in ElementTree.parse(SHARE / folder / f'{name}.xml').iter():
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
    missing = sorted(set(files.values()) - DESCRIPTIONS.keys())
    if missing:
        raise SystemExit(f'{name}: no description for {missing}')
    return {'machine': name, 'label': label, 'note': note,
            'roms': [{'file': file, 'sha1': list(digests), 'label': DESCRIPTIONS[file][0], 'description': DESCRIPTIONS[file][1]}
                     for digests, file in files.items()],
            'requirements': requirements}


def content():
    return json.dumps({'machines': [machine(*offered) for offered in OFFERED]}, indent=1) + '\n'


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
