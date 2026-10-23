# Development

## Building

Build the app, and optionally install it, with the shared build script (see [Android build](../shared/docs/android-build.md)):

```powershell
shared\tools\build.ps1 -Install <adb serial>
```

## Host tests

The machine also builds for the host, without Android: build `core/` with CMake for the host, stage openMSX's data folder with `tools/stage_openmsx_data.py <folder>`, then run, under `shared\tools\run-limited.ps1`:

```
boot_test [--machine <name>] [--memory <KB>] [--disk <image>] [--hold <row>,<column>[;...][@<seconds>]] [--command <Tcl>] <data folder> <work folder> <seconds> [cartridge [joystick|none|key:<row>,<column>]]
```

It boots a cartridge (or the machine alone) and presses Space, the joystick's trigger, or a key of the keyboard matrix; checks that a save state loads back to the same picture and sound, and that the mouse joins port B only once it is used; and starts a second machine in the same process, as the app does for its next game. `--command` prints what an openMSX console command returns after the boot, such as debugger registers or a watchpoint script.

## MSX-DOS2 disks

`tools/msxdos2_disks.py` lists the disks that start from MSX-DOS2: those holding `MSXDOS2.SYS`, and those recorded by hand in `catalog/research/msxdos2-disks-v1.json`. The catalog turns their MSX-DOS2 setting on.

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
