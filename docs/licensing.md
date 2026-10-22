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

The shared RetroArch Slang shader runtime uses librashader under its GPL-3.0-only option and includes Rust crates licensed only under Apache-2.0 (`codespan-reporting`, `rspirv`, `spirv`). Apache-2.0 code cannot be combined with GPL-2.0-only code. A shader-enabled KairoMSX build therefore needs openMSX under GPL-2.0-or-later. The question is open with the openMSX developers in [openMSX#2248](https://github.com/openMSX/openMSX/issues/2248). Until they confirm "or later", KairoMSX builds leave out the shader runtime and are distributed under GPL-2.0 terms. Shaders are enabled only after that confirmation. The source snapshot itself may be redistributed under its own terms.

### KairoMSX's build of openMSX

KairoMSX builds openMSX's emulation code with its own replacements for the parts that draw, play and pace (`core/kairo/`, GPL-2.0-or-later). It links Dear ImGui's core (MIT, in `src/3rdparty/imgui/`) for the key names openMSX keeps in its settings, and none of openMSX's ImGui windows, OpenGL code or fonts.

## Libraries openMSX needs

| Library | Location | License |
| --- | --- | --- |
| zlib 1.3.2 | `third_party/zlib/` | zlib license (`LICENSE`) |
| libpng 1.6.59 | `third_party/libpng/` | PNG Reference Library License version 2 (`LICENSE`) |
| Tcl 8.6.18 | `third_party/tcl/` | Tcl/Tk license, BSD-style (`license.terms`) |
| SDL2 2.32.10 | `third_party/sdl2/` | zlib license (`LICENSE.txt`) |

All four permit combination with GPL-2.0-only code. KairoMSX's arm64 build does not link the NDK's Apache-2.0 `cpufeatures`, which SDL2 uses only on 32-bit ARM (see the `sdl-cpufeatures` change in [source import](source-import.md)).

## C-BIOS

`third_party/openmsx/Contrib/cbios/` holds [C-BIOS](https://cbios.sourceforge.net/) 0.29, a free MSX BIOS replacement for running cartridge images, under the BSD-2-Clause license in `third_party/openmsx/Contrib/README.cbios`. It contains no original MSX system ROM code.

## Firmware and games

Apart from C-BIOS, KairoMSX does not include MSX system ROMs, disk ROMs, operating systems, or games. Users supply any firmware and games they need.
