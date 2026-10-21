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

There are no local changes to the openMSX sources.
