package com.mrjackspade.kairomsx

import com.mrjackspade.kairo.frontend.CatalogGame
import com.mrjackspade.kairo.frontend.ControllerGuestSpec
import com.mrjackspade.kairo.frontend.ControllerLayout
import com.mrjackspade.kairo.frontend.DiskImageEntry
import com.mrjackspade.kairo.frontend.DiskImageLibrary
import com.mrjackspade.kairo.frontend.FirmwareSet
import com.mrjackspade.kairo.frontend.LayeredGameCatalog
import com.mrjackspade.kairo.frontend.LibraryStrings
import com.mrjackspade.kairo.frontend.MachineActivity
import com.mrjackspade.kairo.frontend.MachineFirstRun
import com.mrjackspade.kairo.frontend.NativeMachine

import org.json.JSONObject
import java.io.File
import java.util.concurrent.atomic.AtomicBoolean

/** KairoMSX: the MSX on openMSX, in the shared Kairo frontend. */
class MainActivity : MachineActivity<DiskImageEntry>() {
    override val identity = Identity("KAIROMSX", 0xff972AFB.toInt(), "KairoMSX", "MSX", "kairomsx",
        LibraryStrings("KAIROMSX", "KairoMSX"),
        "KairoMSX uses openMSX's MSX emulation and the C-BIOS system software. Source and provenance: " +
            "github.com/MrJackSpade/KairoMSX.",
        "Choose a games folder to find cartridge, disk and tape images and the ZIP and 7z archives holding them",
        "Folder access expired. Select the games folder again.",
        MachineFirstRun.Folder("Choose a games folder", "Select the folder containing your MSX games. KairoMSX " +
            "will scan its cartridge, disk and tape images and the ZIP and 7z archives holding them.", "Choose games folder…"),
        "SAVE STATES", "MrJackSpade/KairoMSX")
    override val machine = Machine(320, 240, 4.0 / 3.0, "state.oms")
    override val bridge by lazy { NativeMachine("kairomsx") }
    override val keyboard = MsxKeys.layout
    override val maxKeyCode = MsxKeys.MAX_CODE
    override fun guestKey(androidKey: Int) = MsxKeys.fromAndroid(androidKey)
    override val joystickControls get() = MsxKeys.joystick
    override val controllerSpec = ControllerGuestSpec("MSX", MsxKeys.bindableCodes, { MsxKeys.label(it) ?: "Key $it" },
        setOf(MsxKeys.SHIFT, MsxKeys.CTRL, MsxKeys.GRAPH, MsxKeys.CODE), MsxKeys.joystick.zip(MsxKeys.joystickLabels),
        "The joystick in port A.", ControllerGuestSpec.STANDARD_ACTIONS, MsxKeys.layout)
    override val bindingsCodec = MsxKeys.bindings
    override fun defaultBindings(layout: ControllerLayout) = MsxKeys.defaults(layout)
    override val artworkDownloadsEnabled: Boolean get() = resources.getBoolean(R.bool.catalog_art_download_enabled)

    /** Cartridge, disk and tape images in the formats openMSX reads. */
    private val images by lazy {
        DiskImageLibrary(this, DiskImageLibrary.Format(
            kinds = listOf("rom", "mx1", "mx2", "ri").associateWith { "rom" } +
                listOf("dsk", "di1", "di2", "dmk").associateWith { "fd" } + listOf("cas", "tsx").associateWith { "tape" },
            labels = mapOf("rom" to "Cartridge", "fd" to "Floppy disk", "tape" to "Cassette"),
            maxBytes = mapOf("rom" to 16L * 1024 * 1024, "fd" to 16L * 1024 * 1024, "tape" to 16L * 1024 * 1024),
            guestName = "MSX"))
    }
    override fun createCatalog() = LayeredGameCatalog(this, MsxCatalog)
    override fun createLibrary(catalog: LayeredGameCatalog) = images

    private fun isCartridge(entry: DiskImageEntry) = entry.mediaLabel == "Cartridge"
    private fun isDisk(entry: DiskImageEntry) = entry.mediaLabel == "Floppy disk"
    private fun isTape(entry: DiskImageEntry) = entry.mediaLabel == "Cassette"

    override val machineOptions = MsxCatalog.options
    override val mediaPageTitle = "Media"
    // In the core's slot order (msx_machine.h Slot).
    override val mediaSlots = listOf(MediaSlot<DiskImageEntry>("Cartridge A", ::isCartridge),
        MediaSlot("Cartridge B", ::isCartridge), MediaSlot("Disk A", ::isDisk), MediaSlot("Disk B", ::isDisk),
        MediaSlot("Cassette", ::isTape))

    /**
     * The system ROMs of the real machines (assets/system_roms.json, from openMSX's machine
     * definitions), found by content and copied to openMSX's systemroms folder, where openMSX finds
     * them by digest. One set per machine.
     */
    override val firmwareSets by lazy {
        val machines = JSONObject(assets.open("system_roms.json").bufferedReader().use { it.readText() })
            .getJSONArray("machines")
        (0 until machines.length()).map { index ->
            val machine = machines.getJSONObject(index)
            val roms = machine.getJSONArray("roms")
            val parts = (0 until roms.length()).associate { romIndex ->
                val rom = roms.getJSONObject(romIndex)
                val file = rom.getString("file")
                val digests = rom.getJSONArray("sha1")
                file to FirmwareSet.Part(file, rom.getString("label"), "$file · ${rom.getString("description")}",
                    setOf(file.lowercase()), true,
                    sha1 = (0 until digests.length()).map(digests::getString).toSet(), maxBytes = MAX_SYSTEM_ROM)
            }
            // Each requirement is met by any one of its files (system_roms.py).
            val requirements = machine.getJSONArray("requirements")
            machine.getString("label") to FirmwareSet(systemRoms, parts.values.toList(),
                (0 until requirements.length()).map { index ->
                    val options = requirements.getJSONArray(index)
                    (0 until options.length()).map { parts.getValue(options.getString(it)) }
                }, machine.getString("note"))
        } + ("MSX-DOS2" to msxDos2)
    }

    /**
     * The player's MSX-DOS2 system files, for disks that start from MSX-DOS2 without them (MsxDos2):
     * ASCII's software, which the app cannot ship. Found by name; no machine needs them.
     */
    private val msxDos2 by lazy {
        FirmwareSet(File(openmsxHome, "user/msxdos2"), listOf(MsxDos2.SYSTEM, MsxDos2.COMMAND).map { file ->
            FirmwareSet.Part(file, file, "$file · MSX-DOS2 system file",
                setOf(file.lowercase()), false, maxBytes = MAX_DOS_FILE)
        }, note = "For disks that start from MSX-DOS2 but do not include it")
    }
    private val openmsxHome by lazy { File(filesDir, "openmsx") }
    /** openMSX's user systemroms folder: its OPENMSX_HOME's share/systemroms (msx_core.cpp). */
    private val systemRoms by lazy { File(openmsxHome, "user/share/systemroms") }

    override fun startGame(entry: DiskImageEntry, game: CatalogGame, cancelled: AtomicBoolean,
                           progress: (String) -> Unit) {
        val home = prepareOpenmsx(progress)
        // In the core's slot order (msx_core.cpp): cartridge A, cartridge B, disk A, disk B, cassette.
        val media = listOf(entry.takeIf(::isCartridge), null, entry.takeIf(::isDisk), null, entry.takeIf(::isTape))
        val images = media.map { image -> image?.let { this.images.prepare(it, cancelled, progress).absolutePath } }
        val options = optionValues(game).toMutableMap()
        val dos2 = isDisk(entry) && options[MsxCatalog.msxdos2.id] == "on"
        if (options[MsxCatalog.machine.id] == MsxCatalog.AUTOMATIC) {
            options[MsxCatalog.machine.id] = when {
                dos2 -> MsxCatalog.DOS2_MACHINE
                isCartridge(entry) -> MsxCatalog.CARTRIDGE_MACHINE
                else -> MsxCatalog.DISK_MACHINE
            }
        }
        if (dos2) MsxDos2.install(File(checkNotNull(images[2]))) { name ->
            msxDos2.file(msxDos2.parts.single { it.name == name }).takeIf { it.isFile }
        }
        check(bridge.start("", home.absolutePath, images, options)) { "Another machine is still running" }
        awaitRunning()
        mediaStarted(media)
    }

    /**
     * The core's home (msx_core.cpp): "system", openMSX's data folder copied from assets/openmsx
     * when its STAMP differs, and "user", openMSX's own files.
     */
    private fun prepareOpenmsx(progress: (String) -> Unit): File {
        val home = openmsxHome
        val system = File(home, "system")
        val stamp = assets.open("openmsx/STAMP").bufferedReader().use { it.readText() }
        if (File(system, "STAMP").takeIf { it.isFile }?.readText() != stamp) {
            progress("Preparing the emulator")
            val copy = File(home, "system.new")
            copy.deleteRecursively()
            copyAsset("openmsx", copy)
            system.deleteRecursively()
            check(copy.renameTo(system)) { "Could not install the emulator's data in $system" }
        }
        File(home, "user").mkdirs()
        return home
    }

    private fun copyAsset(path: String, dest: File) {
        val children = assets.list(path) ?: error("Missing asset $path")
        if (children.isEmpty()) {
            assets.open(path).use { input -> dest.outputStream().use { input.copyTo(it) } }
        } else {
            check(dest.mkdirs() || dest.isDirectory) { "Could not create $dest" }
            for (child in children) copyAsset("$path/$child", File(dest, child))
        }
    }

    private companion object {
        /** The largest MSX system ROM, the turbo R's 4 MiB firmware. */
        const val MAX_SYSTEM_ROM = 4L * 1024 * 1024
        /** Larger than any MSX-DOS2 system file (COMMAND2.COM is about 15 KB). */
        const val MAX_DOS_FILE = 64L * 1024
    }

    override fun stateError(code: Int) = when (code) {
        2 -> "the emulator could not read or write the state"
        3 -> "the emulator refused the image"
        else -> "code $code"
    }
}
