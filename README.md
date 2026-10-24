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
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/Kairo98"><img src="https://raw.githubusercontent.com/MrJackSpade/Kairo98/main/kairo98/src/main/res/drawable-nodpi/product_icon_art.png" width="96" alt="Kairo98"><br><b>Kairo98</b></a><br>PC-98</td>
    <td align="center" width="33%"><a href="https://github.com/MrJackSpade/KairoDos"><img src="https://raw.githubusercontent.com/MrJackSpade/KairoDos/main/kairodos/src/main/res/drawable-nodpi/product_icon_art.png" width="96" alt="KairoDos"><br><b>KairoDos</b></a><br>DOS</td>
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

**Play your own MSX games on Android phones and handhelds.** KairoMSX is an Android emulator for the MSX, MSX2, MSX2+, and MSX turbo R, built on the [Kairo shared frontend](https://github.com/MrJackSpade/Kairo): the same game library with artwork, controller and touch controls, on-screen keyboard, save states, and per-game settings as [Kairo98](https://github.com/MrJackSpade/Kairo98) and [KairoDos](https://github.com/MrJackSpade/KairoDos). It runs a pinned copy of [openMSX](https://openmsx.org/). It is an independent project, not an official openMSX release.

## Download

<!-- kairo:download -->
- **[GitHub Releases](https://github.com/MrJackSpade/KairoMSX/releases)** (free): install the APK yourself. The larger `-with-images` APK also bundles catalog artwork.

Requires an ARM64 device running Android 8.0 or newer.
<!-- /kairo:download -->

## Quick start

1. Install KairoMSX from [GitHub Releases](https://github.com/MrJackSpade/KairoMSX/releases).
2. Open KairoMSX and choose the folder that holds your MSX games. You can change it later in **Menu › Manage library**.
3. Select a game and tap **Play**. Cartridge games run straight away; disk, tape and turbo R games need a BIOS from your own machine (see [Firmware](#firmware)).

Games and the MSX machines' BIOS are not supplied.

## Supported formats

| Type | Formats |
| --- | --- |
| Cartridges | ROM, MX1, MX2, RI |
| Disks | DSK, DI1, DI2, DMK |
| Tapes | CAS, TSX |
| Archives | Any of the above inside ZIP or 7z archives |

A game's disks show as one library entry. **Menu › Media** changes disks and tapes during play.

## Firmware

KairoMSX comes with [C-BIOS](https://cbios.sourceforge.net/), a free replacement BIOS that runs most cartridge games without any BIOS from you. Disk games, tape games, turbo R games and cartridges written in MSX-BASIC need the BIOS of a real machine from your own: the Panasonic FS-A1WSX (MSX2+), the Panasonic FS-A1GT (turbo R) or the Philips NMS 8250 (MSX2). **Menu › Machine** lists every BIOS file, each marked Required or Optional. Pick each file, or use **Import all from a folder…** to import every file the app recognizes in a folder you choose.

Each game starts on the machine it needs; you can change it in the game's **Game settings › Machine**.

### MSX-DOS2

Some disks start from MSX-DOS2, ASCII's disk operating system, which only the turbo R has built in. The catalog turns their **MSX-DOS2** setting on: they start on the FS-A1GT, and the first start readies the game's working copy of the disk with a DOS2 boot sector and, if the disk lacks them, `MSXDOS2.SYS` and `COMMAND2.COM`. Those two files are ASCII's software, which the app cannot supply: put your own copies in a folder and import them in **Menu › Machine**, like the BIOS files. You can turn the setting on for any other disk in its **Game settings**.

## Machine settings

**Menu › Machine** sets these for all games; each game can override them in its **Game settings**.

- **Machine:** Automatic picks the right machine for each game; or choose one of the real machines or a C-BIOS MSX, MSX2 or MSX2+.
- **Memory:** the main RAM of MSX2 and MSX2+ machines, 64 KB to 4 MB (512 KB by default). MSX1 machines and the turbo R keep their own.
- **MSX-DOS2:** see [above](#msx-dos2).

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

Host tests, catalog tools and UI tests: see [development](docs/development.md).

## License

<!-- kairo:license -->
First-party code is [GPL-2.0-or-later](LICENSE.md). The app as distributed is GPLv3 because it bundles GPLv3 components. Third-party components keep their own licenses; see [licensing and credits](docs/licensing.md) and [source provenance](docs/source-import.md).
<!-- /kairo:license -->
