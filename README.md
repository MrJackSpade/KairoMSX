<!-- kairo:discord -->
<p align="center">
  <a href="https://discord.gg/ZkYQ3bPTTq"><img src="https://raw.githubusercontent.com/MrJackSpade/Kairo/main/docs/assets/discord-banner.svg" alt="Join the Kairo Discord" width="49%"></a>
  <a href="https://play.google.com/store/apps/details?id=com.loxifi.kairomsx"><img src="https://raw.githubusercontent.com/MrJackSpade/Kairo/main/docs/assets/donate-banner.svg" alt="Donate: buying KairoMSX on Google Play is the donation; it adds no features" width="49%"></a>
</p>
<!-- /kairo:discord -->

<!-- kairo:family -->
<h3 align="center">Other Kairo apps</h3>
<table align="center">
  <tr>
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/Kairo98"><img src="https://raw.githubusercontent.com/MrJackSpade/Kairo98/main/kairo98/src/main/res/drawable-nodpi/kairo98_icon_art.png" width="96" alt="Kairo98"><br><b>Kairo98</b></a><br>PC-98</td>
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/KairoDos"><img src="https://raw.githubusercontent.com/MrJackSpade/KairoDos/main/kairodos/src/main/res/drawable-nodpi/kairodos_icon_art.png" width="96" alt="KairoDos"><br><b>KairoDos</b></a><br>DOS</td>
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/Kairo88"><img src="https://raw.githubusercontent.com/MrJackSpade/Kairo88/main/kairo88/src/main/res/drawable-nodpi/product_icon_art.png" width="96" alt="Kairo88"><br><b>Kairo88</b></a><br>PC-88</td>
  </tr>
  <tr>
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/Kairo68"><img src="https://raw.githubusercontent.com/MrJackSpade/Kairo68/main/kairo68/src/main/res/drawable-nodpi/product_icon_art.png" width="96" alt="Kairo68"><br><b>Kairo68</b></a><br>X68000</td>
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/KairoTowns"><img src="https://raw.githubusercontent.com/MrJackSpade/KairoTowns/main/kairotowns/src/main/res/drawable-nodpi/product_icon_art.png" width="96" alt="KairoTowns"><br><b>KairoTowns</b></a><br>FM Towns</td>
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/KairoST"><img src="https://raw.githubusercontent.com/MrJackSpade/KairoST/main/kairost/src/main/res/drawable-nodpi/product_icon_art.png" width="96" alt="KairoST"><br><b>KairoST</b></a><br>Atari ST</td>
  </tr>
</table>
<p align="center">Every Kairo app is built on <a href="https://github.com/MrJackSpade/Kairo">Kairo</a>, the shared frontend for the game library, controls, and settings.</p>
<!-- /kairo:family -->

# KairoMSX

**Play your own MSX games on Android phones and handhelds.** KairoMSX is an Android emulator for the MSX, MSX2, MSX2+, and MSX turbo R, built on the [Kairo shared frontend](https://github.com/MrJackSpade/Kairo): the same game library, controller and touch controls, on-screen keyboard, and per-game settings as [Kairo98](https://github.com/MrJackSpade/Kairo98) and [KairoDos](https://github.com/MrJackSpade/KairoDos). It runs a pinned copy of [openMSX](https://openmsx.org/). It is an independent project, not an official openMSX release.

## Status

Early development. There is no release yet. Cartridge games start on C-BIOS, which comes with the app. Disk, tape and turbo R games and MSX-BASIC cartridges start on a real MSX (Panasonic FS-A1WSX, Panasonic FS-A1GT or Philips NMS 8250) with its BIOS from your own machine, imported in Menu › Machine.

## Games and firmware

Games and the MSX machines' BIOS are not supplied. The pinned core includes [C-BIOS](https://cbios.sourceforge.net/), a free replacement BIOS that runs most cartridge games without any BIOS from you. Disk games, turbo R games and cartridges written in MSX-BASIC need the BIOS of a real machine from your own. Menu › Machine lists every BIOS file, each marked Required or Optional. Pick each file, or use **Import all from a folder…** to import every file the app recognizes in a folder you choose. Each game starts on the machine it needs; you can change it in the game's Machine settings. MSX2 and MSX2+ machines start with 512 KB of RAM, which the Memory setting changes for all games or one; MSX1 machines and the turbo R keep their own.

Some disks start from MSX-DOS2, ASCII's disk operating system, which only the turbo R has built in. Their MSX-DOS2 setting is on: they start on the FS-A1GT, and the first start readies the game's working copy of the disk. It gains a DOS2-format boot sector, and `MSXDOS2.SYS` and `COMMAND2.COM` if it lacks them. Those two files are ASCII's software, which the app cannot supply: Menu › Machine › MSX-DOS2 imports your own, like the BIOS files. The setting can be turned on for any other disk in its Machine settings. `tools/msxdos2_disks.py` lists the disks that need it, by `MSXDOS2.SYS` on the disk or by hand in `catalog/research/msxdos2-disks-v1.json`.

## Documentation

- [Licensing and credits](docs/licensing.md): code license and third-party components
- [Source provenance](docs/source-import.md): where the emulator source came from and what was copied

## Help and community

<!-- kairo:community -->
- **Report a game problem** in [Issues](https://github.com/MrJackSpade/KairoMSX/issues). Include the title, file format, and what happened.
- **Chat** with other players on [Discord](https://discord.gg/ZkYQ3bPTTq).
<!-- /kairo:community -->

## Building from source

<!-- kairo:clone -->
Clone with submodules to get the pinned shared frontend:

```sh
git clone --recurse-submodules https://github.com/MrJackSpade/KairoMSX.git
```
<!-- /kairo:clone -->

Build the app, and optionally install it, with the shared build script:

```powershell
shared\tools\build.ps1 -Install <adb serial>
```

The host boot test runs the machine without Android: build `core/` with CMake for the host, stage openMSX's data folder with `tools/stage_openmsx_data.py <folder>`, then run `boot_test [--machine <name>] [--memory <KB>] [--disk <image>] [--hold <row>,<column>[;...][@<seconds>]] [--command <Tcl>] <data folder> <work folder> <seconds> [cartridge [joystick|none]]`.

<!-- kairo:ui-tests -->
## UI tests

KairoMSX has device tests in the style of Selenium tests, on the shared Kairo test framework: they
name the app's elements, act on them as a player would, and assert what the player would see
([how they work and how to run them](https://github.com/MrJackSpade/Kairo/blob/main/docs/ui-tests.md)). Run them from this
repository with a device connected over adb (Android 10 or later) and the `LOCAL_BUILD_TOKEN` key set:

```
shared\tools\uitest.ps1 -Device <serial>                                              every test
shared\tools\uitest.ps1 -Device <serial> -Filter 'MenuTests|ControllerTests'          by test name
shared\tools\uitest.ps1 -Device <serial> -Class com.mrjackspade.kairomsx.uitest.<Class>   one class
shared\tools\uitest-all.ps1 -Device <serial>                                          every Kairo app
```

The tests are in `kairomsx/src/androidTest/java/com/mrjackspade/kairomsx/uitest/`. They run in a
build of their own, `com.loxifi.kairomsx.uitest`, installed beside the app without touching its data, and
each run's logs, screenshots and summary land in `.build/uitest/<run id>/`.
<!-- /kairo:ui-tests -->

## License

<!-- kairo:license -->
First-party code is [GPL-2.0-or-later](LICENSE.md). The app as distributed is GPLv3 because it bundles GPLv3 components. Third-party components keep their own licenses; see [licensing and credits](docs/licensing.md) and [source provenance](docs/source-import.md).
<!-- /kairo:license -->
