# Source import policy

[← KairoMSX](../README.md)

This repository is independent. Its only Git remote is `origin`, pointing to this project's repository. The first-party Kairo frontend is pinned at `shared/` as a submodule. Do not add an `upstream` remote, fork relationship, other submodule, subtree sync, or automated upstream merge. Third-party source is copied in as pinned snapshots under `third_party/`.

## openMSX snapshot

`third_party/openmsx/` holds [openMSX](https://openmsx.org/) 21.0.

| | |
| --- | --- |
| Release | `RELEASE_21_0`, tag commit `cb61db762aba16752ff649990bf85e40627777af` (2025-09-26) |
| Archive | <https://codeload.github.com/openMSX/openMSX/tar.gz/refs/tags/RELEASE_21_0> |
| Archive SHA-256 | `678b520172ad0f1f75350dfecf827d742dba2edf57fba545f3d2bcac7d4818fc` |
| Imported | 2026-10-21 |

The archive was copied whole except for:

- `Contrib/cbios-old/`: compressed builds of earlier C-BIOS versions. The current C-BIOS 0.29 in `Contrib/cbios/` is kept.
- `Contrib/codec/Win32/zmbv.dll`: a prebuilt Windows binary.
- `.github/` and the nested `.gitignore` files, removed so every copied file is tracked.

The local changes are below.

## Libraries openMSX needs

openMSX links zlib, libpng, Tcl and SDL2. Each is a pinned release copied into `third_party/`, built by `core/deps.cmake`.

| Library | Release | Archive | Archive SHA-256 | Released |
| --- | --- | --- | --- | --- |
| zlib (`zlib/`) | 1.3.2 | GitHub release `v1.3.2` of `madler/zlib`, `zlib-1.3.2.tar.gz` | `bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16` | 2026-02-17 |
| libpng (`libpng/`) | 1.6.59 | <https://codeload.github.com/pnggroup/libpng/tar.gz/refs/tags/v1.6.59> | `2540302a1844ad2b2b501977abecfa850f265f97b78f065a712ab4074a89f5b5` | 2026-09-28 |
| Tcl (`tcl/`) | 8.6.18 | GitHub release `core-8-6-18` of `tcltk/tcl`, `tcl8.6.18-src.tar.gz` | `14f9af32b1767ff718477a8f974ad03c34341097e6b43f4ce54644ee974e268e` | 2026-05-13 |
| SDL2 (`sdl2/`) | 2.32.10 | GitHub release `release-2.32.10` of `libsdl-org/SDL`, `SDL2-2.32.10.tar.gz` | `5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165` | 2025-09-01 |

All were imported on 2026-10-22. The zlib and SDL2 archives were checked against their release signatures: zlib's by Mark Adler (key `5ED4 6A67 21D3 6558 7791 E2AA 783F CD8E 58BC AFBA`), SDL2's by Sam Lantinga (key `1528 635D 8053 A57F 77D1 E086 30A5 9377 A776 3BE6`).

Each archive was copied whole except for files no KairoMSX build uses:

- zlib: `amiga/`, `contrib/`, `doc/`, `msdos/`, `os400/`, `qnx/`, `test/`, `watcom/` and `win32/`.
- libpng: `ci/`.
- Tcl: `doc/`, `macosx/`, `pkgs/`, `tests/`, `tools/`, `compat/zlib/` (KairoMSX's zlib is used instead) and the prebuilt `win/nmakehlp.exe`.
- SDL2: the IDE projects (`VisualC*/`, `Xcode*/`), `acinclude/`, `android-project*/`, `docs/`, `mingw/`, `test/`, `visualtest/` and `wayland-protocols/`.
- Nested Git files (`.gitignore`, `.gitattributes` and similar), so every copied file is tracked.

## Local changes

`tools/local_patches.py` makes these changes to the snapshots and checks they are in place (`--check`). Each replaces text that occurs exactly once.

| Change | File | What and why |
| --- | --- | --- |
| `sdl-cpufeatures` | `sdl2/CMakeLists.txt` | Adds the NDK's `cpufeatures` (Apache-2.0) only to 32-bit ARM builds, the only ones whose code calls it, so KairoMSX's arm64 build links no Apache-2.0 code into GPL-2.0-only openMSX. |
| `openmsx-imgui-freetype` | `openmsx/src/3rdparty/imgui/imconfig.h` | Builds Dear ImGui without FreeType fonts. KairoMSX compiles ImGui's core only for the key names of openMSX's settings. |
| `openmsx-postprocessor` | `openmsx/src/video/PostProcessor.hh` | Includes KairoMSX's `PostProcessor` (`core/kairo/KairoPostProcessor.hh`), which hands frames to the host instead of OpenGL. The rasterizers include this header from their own folder, so a stand-in header elsewhere cannot replace it. |
| `openmsx-sound-driver` | `openmsx/src/sound/Mixer.cc` | Creates KairoMSX's sound driver, which queues the mixed sound for the host, where openMSX creates its SDL one. |

The rest of KairoMSX's replacements need no change to openMSX: `core/CMakeLists.txt` leaves openMSX's OpenGL renderer, ImGui windows, on-screen display, SDL video and sound drivers and real-time pacing out of the build, and compiles classes of the same names from `core/kairo/` in their place.
