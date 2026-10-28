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

### GPL version and the shader runtime

The README's "GPL" without a version is read as permission under any GPL version, so openMSX is treated as GPL-2.0-or-later (Debian's record reads it as GPL-2 only). On that reading KairoMSX links the shared RetroArch Slang shader runtime, which uses librashader under its GPL-3.0-only option and includes Rust crates licensed only under Apache-2.0, and the combined app is distributed under GPLv3 terms, as the other Kairo apps are. The same reading covers the Apache-2.0 Java libraries every Kairo app carries (the Kotlin standard library, AndroidX and Apache Commons Compress).

The question is open with the openMSX developers in [openMSX#2248](https://github.com/openMSX/openMSX/issues/2248). If they state GPL-2.0-only, KairoMSX must leave out the shader runtime: the shared frontend's `kairoShaders=false` Gradle property builds an app without it. The Apache-2.0 Java libraries would then need review as well.

### KairoMSX's build of openMSX

KairoMSX builds openMSX's emulation code with its own replacements for the parts that draw, play and pace (`core/kairo/`, GPL-2.0-or-later). It links Dear ImGui's core (MIT, in `src/3rdparty/imgui/`) for the key names openMSX keeps in its settings, and none of openMSX's ImGui windows, OpenGL code or fonts.

## Notices in the app

`shared/tools/notices.py .` writes the app's `THIRD_PARTY_NOTICES.txt` (About → Licenses) from `tools/notices_sources.py`, which lists the license files and source notices below: openMSX's GPL text, the notices of the code its sources embed under other terms (Nuke.YKT's YM2413, SDL_image's PNG code, LZ4, xxHash, UTF8-CPP, fast_log, the Tiger tree hash, function_ref), C-BIOS, Dear ImGui and the four libraries. `tools/notices.py --check` fails when the file is out of date.

## Libraries openMSX needs

| Library | Location | License |
| --- | --- | --- |
| zlib 1.3.2 | `third_party/zlib/` | zlib license (`LICENSE`) |
| libpng 1.6.59 | `third_party/libpng/` | PNG Reference Library License version 2 (`LICENSE`) |
| Tcl 8.6.18 | `third_party/tcl/` | Tcl/Tk license, BSD-style (`license.terms`) |
| SDL2 2.32.10 | `third_party/sdl2/` | zlib license (`LICENSE.txt`) |

All four permit combination with GPL code. KairoMSX's arm64 build does not link the NDK's `cpufeatures`, which SDL2 uses only on 32-bit ARM (see the `sdl-android-sources` change in [source import](source-import.md)).

## C-BIOS

`third_party/openmsx/Contrib/cbios/` holds [C-BIOS](https://cbios.sourceforge.net/) 0.29, a free MSX BIOS replacement for running cartridge images, under the BSD-2-Clause license in `third_party/openmsx/Contrib/README.cbios`. It contains no original MSX system ROM code.

## Shared frontend

<!-- kairo:shared-frontend-license -->
The shared RetroArch Slang shader runtime uses librashader under its GPL-3.0-only option, so a shader-enabled KairoMSX build is distributed under GPLv3 terms using first-party "or later" permission. See [shared licensing](../shared/docs/licensing.md).
<!-- /kairo:shared-frontend-license -->

## Firmware and games

Apart from C-BIOS, KairoMSX does not include MSX system firmware (BIOS), operating systems, or games. Users supply any firmware and games they need. The app recognizes a player's system ROM dumps by the SHA-1 digests in openMSX's machine definitions (`tools/system_roms.py` writes the list) and copies them to its private storage.
