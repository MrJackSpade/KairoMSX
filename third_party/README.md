# Third-party source snapshots

These directories contain copied, pinned source snapshots. They are not Git submodules or forks, and this repository has no upstream remote.

- `openmsx/`: openMSX 21.0, including C-BIOS 0.29, with its license notices preserved. Earlier C-BIOS builds and a prebuilt Windows codec DLL were omitted.
- `zlib/`, `libpng/`, `tcl/`, `sdl2/`: zlib 1.3.2, libpng 1.6.59, Tcl 8.6.18 and SDL2 2.32.10, the libraries openMSX links, without their documentation, tests and other platforms' build files.

`tools/local_patches.py` applies KairoMSX's few local changes to these snapshots; the source import lists them.

See [source import](../docs/source-import.md) for URLs, hashes, dates, and what was included. Do not replace these snapshots through a Git remote or automated sync.
