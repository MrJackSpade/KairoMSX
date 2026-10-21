# Licensing and credits

[← KairoMSX](../README.md)

KairoMSX first-party code is [GPL-2.0-or-later](../LICENSE.md). The pinned [Kairo frontend](https://github.com/MrJackSpade/Kairo) is also GPL-2.0-or-later. Third-party components keep their own notices.

## openMSX

The MSX emulation comes from [openMSX](https://openmsx.org/) 21.0. Its README states that source files without their own notice are licensed under the GPL, with the GPL version 2 text in `third_party/openmsx/doc/GPL.txt`; contributors are listed in `doc/authors.txt`. The README names no GPL version. Debian's copyright record for openMSX reads the project as GPL-2 only, with these files under other terms:

| Files | License |
| --- | --- |
| `src/sound/opll.*`, `src/sound/YM2413NukeYKT.*`, `Contrib/codec/*` | GPL-2.0-or-later |
| `src/utils/win32-dirent.cc` | LGPL-2.1-or-later |
| `src/utils/semiregular.hh`, `Contrib/catch2/` | Boost Software License 1.0 |
| `src/video/PNG.cc` | LGPL-2.0-or-later |
| `Contrib/openmsx-control-*.cc` | Public domain |
| `Contrib/tas/` | MIT |
| `src/cassette/TsxParser.cc` | GPL, any version |
| `src/serial/Midi_w32.*` | BSD-2-Clause |
| `src/3rdparty/imgui/`, `src/3rdparty/ImGuiFileDialog/` | MIT |
| `share/skins/DejaVuSansMono.ttf.gz` | Bitstream Vera Fonts license |

### Open question: GPL version and the shader runtime

The shared RetroArch Slang shader runtime uses librashader under its GPL-3.0-only option and includes Rust crates licensed only under Apache-2.0 (`codespan-reporting`, `rspirv`, `spirv`). Apache-2.0 code cannot be combined with GPL-2.0-only code. A shader-enabled KairoMSX build therefore needs openMSX under GPL-2.0-or-later (or another GPLv3-compatible reading) before it can be distributed. This is unresolved: confirm the GPL version with the openMSX developers before any release. The source snapshot itself may be redistributed under its own terms.

## C-BIOS

`third_party/openmsx/Contrib/cbios/` holds [C-BIOS](https://cbios.sourceforge.net/) 0.29, a free MSX BIOS replacement for running cartridge images, under the BSD-2-Clause license in `third_party/openmsx/Contrib/README.cbios`. It contains no original MSX system ROM code.

## Firmware and games

Apart from C-BIOS, KairoMSX does not include MSX system ROMs, disk ROMs, operating systems, or games. Users supply any firmware and games they need.
