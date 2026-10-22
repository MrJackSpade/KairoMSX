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
    override val identity = Identity("KAIROMSX", "KairoMSX", "MSX", "kairomsx",
        LibraryStrings("KAIROMSX", "KairoMSX"),
        "KairoMSX uses openMSX's MSX emulation and the C-BIOS system software. Source and provenance: " +
            "github.com/MrJackSpade/KairoMSX.",
        "Choose a ROM folder to find cartridge, disk and tape images and the ZIP and 7z archives holding them",
        "Folder access expired. Select the ROM folder again.",
        MachineFirstRun.Folder("Choose a ROM folder", "Select the folder containing your MSX games. KairoMSX " +
            "will scan its cartridge, disk and tape images and the ZIP and 7z archives holding them.", "Choose ROM folder…"),
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
            machine.getString("label") to FirmwareSet(systemRoms, (0 until roms.length()).map { romIndex ->
                val rom = roms.getJSONObject(romIndex)
                val file = rom.getString("file")
                val digests = rom.getJSONArray("sha1")
                FirmwareSet.Part(file, file, setOf(file.lowercase()), true,
                    sha1 = (0 until digests.length()).map(digests::getString).toSet(), maxBytes = MAX_SYSTEM_ROM)
            })
        }
    }
    override val firmwareText get() = Triple("MSX system ROMs",
        "Choose the folder holding the ROM dumps of your own MSX machines. KairoMSX copies the ones it recognizes.",
        "Cartridge games need none. Disk and tape games start on a machine whose ROMs are installed.")
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
        if (options[MsxCatalog.machine.id] == MsxCatalog.AUTOMATIC) {
            options[MsxCatalog.machine.id] = if (isCartridge(entry)) MsxCatalog.CARTRIDGE_MACHINE else MsxCatalog.DISK_MACHINE
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
    }

    override fun stateError(code: Int) = when (code) {
        2 -> "the emulator could not read or write the state"
        3 -> "the emulator refused the image"
        else -> "code $code"
    }
}
