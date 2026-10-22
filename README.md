# KairoMSX

**Play your own MSX games on Android phones and handhelds.** KairoMSX is an Android emulator for the MSX, MSX2, MSX2+, and MSX turbo R, built on the [Kairo shared frontend](https://github.com/MrJackSpade/Kairo): the same game library, controller and touch controls, on-screen keyboard, and per-game settings as [Kairo98](https://github.com/MrJackSpade/Kairo98) and [KairoDos](https://github.com/MrJackSpade/KairoDos). It runs a pinned copy of [openMSX](https://openmsx.org/). It is an independent project, not an official openMSX release.

## Status

Early development. There is no release yet. Cartridge games start on C-BIOS, which comes with the app. Disk and tape games start on a real MSX (Panasonic FS-A1WSX, Panasonic FS-A1GT or Philips NMS 8250) with system ROMs dumped from your own machine, imported in Menu > Machine.

## Games and firmware

Games and system ROMs are not supplied. The pinned core includes [C-BIOS](https://cbios.sourceforge.net/), a free replacement BIOS that runs many cartridge games without any firmware from you. Disk games and the turbo R need system ROMs from your own machine.

## Documentation

- [Licensing and credits](docs/licensing.md): code license and third-party components
- [Source provenance](docs/source-import.md): where the emulator source came from and what was copied

## Help and community

- **Chat** with other players on [Discord](https://discord.gg/ZkYQ3bPTTq).

### Related projects

- [Kairo98](https://github.com/MrJackSpade/Kairo98): PC-98 emulator for Android
- [KairoDos](https://github.com/MrJackSpade/KairoDos): DOS emulator for Android
- [Kairo88](https://github.com/MrJackSpade/Kairo88): PC-88 emulator for Android
- [Kairo68](https://github.com/MrJackSpade/Kairo68): X68000 emulator for Android
- [KairoTowns](https://github.com/MrJackSpade/KairoTowns): FM Towns emulator for Android
- [Kairo](https://github.com/MrJackSpade/Kairo): shared frontend used by every Kairo app

## Building from source

Clone with submodules to get the pinned shared frontend:

```sh
git clone --recurse-submodules https://github.com/MrJackSpade/KairoMSX.git
```

Build the app, and optionally install it, with the shared build script:

```powershell
shared\tools\build.ps1 -Install <adb serial>
```

The host boot test runs the machine without Android: build `core/` with CMake for the host, stage openMSX's data folder with `tools/stage_openmsx_data.py <folder>`, then run `boot_test <data folder> <work folder> <seconds> [cartridge [joystick|none]]`.

## License

First-party code is [GPL-2.0-or-later](LICENSE.md). Third-party components keep their own licenses; see [licensing and credits](docs/licensing.md).
