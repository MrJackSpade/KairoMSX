package com.mrjackspade.kairomsx

import com.mrjackspade.kairo.frontend.R as FrontendR
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
import com.mrjackspade.kairo.frontend.ShippedAssets
import com.mrjackspade.kairo.frontend.UiText
import com.mrjackspade.kairo.frontend.text

import org.json.JSONObject
import java.io.File
import java.util.concurrent.atomic.AtomicBoolean

/** KairoMSX: the MSX on openMSX, in the shared Kairo frontend. */
class MainActivity : MachineActivity<DiskImageEntry>() {
    override val identity = Identity("KAIROMSX", 0xff972AFB.toInt(), "KairoMSX", "MSX", "kairomsx",
        LibraryStrings("KAIROMSX", "KairoMSX"),
        UiText.res(R.string.credits, "KairoMSX", "openMSX", "MSX", "C-BIOS", "MrJackSpade/KairoMSX"),
        UiText.res(R.string.library_empty), UiText.res(FrontendR.string.kairo_library_expired),
        MachineFirstRun.Folder(UiText.res(FrontendR.string.kairo_folder_title), UiText.res(R.string.folder_description, "MSX", "KairoMSX"),
            UiText.res(FrontendR.string.kairo_folder_button)),
        UiText.res(FrontendR.string.kairo_saves_group), "MrJackSpade/KairoMSX")
    override val machine = Machine(320, 240, 4.0 / 3.0, "state.oms")
    override val bridge by lazy { NativeMachine("kairomsx") }
    override val keyboard = MsxKeys.layout
    override val maxKeyCode = MsxKeys.MAX_CODE
    override fun guestKey(androidKey: Int) = MsxKeys.fromAndroid(androidKey)
    override val joystickControls get() = MsxKeys.joystick
    override val controllerSpec = ControllerGuestSpec("MSX", MsxKeys.bindableCodes, { MsxKeys.label(it) ?: "Key $it" },
        setOf(MsxKeys.SHIFT, MsxKeys.CTRL, MsxKeys.GRAPH, MsxKeys.CODE), MsxKeys.joystick.zip(MsxKeys.joystickLabels),
        UiText.res(R.string.joystick_help), ControllerGuestSpec.STANDARD_ACTIONS, MsxKeys.layout)
    override val bindingsCodec = MsxKeys.bindings
    override val artworkDownloadsEnabled: Boolean get() = resources.getBoolean(R.bool.catalog_art_download_enabled)

    /** Cartridge, disk and tape images in the formats openMSX reads. */
    private val format = DiskImageLibrary.Format(
        kinds = listOf("rom", "mx1", "mx2", "ri").associateWith { "rom" } +
            listOf("dsk", "di1", "di2", "dmk").associateWith { "fd" } + listOf("cas", "tsx").associateWith { "tape" },
        labels = mapOf("rom" to UiText.res(FrontendR.string.kairo_cartridge), "fd" to UiText.res(FrontendR.string.kairo_floppy_disk),
            "tape" to UiText.res(FrontendR.string.kairo_cassette)),
        maxBytes = mapOf("rom" to 16L * 1024 * 1024, "fd" to 16L * 1024 * 1024, "tape" to 16L * 1024 * 1024),
        guestName = "MSX")
    private val images by lazy { DiskImageLibrary(this, format) }
    override fun createCatalog() = LayeredGameCatalog(this, MsxCatalog)
    override fun createLibrary(catalog: LayeredGameCatalog) = images

    private fun isCartridge(entry: DiskImageEntry) = format.kind(entry.displayName) == "rom"
    private fun isDisk(entry: DiskImageEntry) = format.kind(entry.displayName) == "fd"
    private fun isTape(entry: DiskImageEntry) = format.kind(entry.displayName) == "tape"

    override val machineOptions = MsxCatalog.options
    override val mediaPageTitle = UiText.res(R.string.media)
    // In the core's slot order (msx_machine.h Slot).
    override val mediaSlots = listOf(MediaSlot<DiskImageEntry>(UiText.res(R.string.cartridge_a), ::isCartridge),
        MediaSlot(UiText.res(R.string.cartridge_b), ::isCartridge), MediaSlot(UiText.res(R.string.disk_a), ::isDisk),
        MediaSlot(UiText.res(R.string.disk_b), ::isDisk), MediaSlot(UiText.res(FrontendR.string.kairo_cassette), ::isTape))

    /**
     * The system ROMs of the real machines (assets/system_roms.json, from openMSX's machine
     * definitions), found by content and copied to openMSX's systemroms folder, where openMSX finds
     * them by digest. One set per machine. Their names and notes are the asset's English.
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
                file to FirmwareSet.Part(file, UiText.fixed(rom.getString("label")),
                    UiText.res(FrontendR.string.kairo_firmware_part, file, UiText.fixed(rom.getString("description"))), setOf(file.lowercase()), true,
                    sha1 = (0 until digests.length()).map(digests::getString).toSet(), maxBytes = MAX_SYSTEM_ROM)
            }
            // Each requirement is met by any one of its files (system_roms.py).
            val requirements = machine.getJSONArray("requirements")
            UiText.fixed(machine.getString("label")) to FirmwareSet(systemRoms, parts.values.toList(),
                (0 until requirements.length()).map { index ->
                    val options = requirements.getJSONArray(index)
                    (0 until options.length()).map { parts.getValue(options.getString(it)) }
                }, UiText.fixed(machine.getString("note")))
        } + (UiText.fixed(DOS2) to msxDos2)
    }

    /**
     * The player's MSX-DOS2 system files, for disks that start from MSX-DOS2 without them (MsxDos2):
     * ASCII's software, which the app cannot ship. Found by name; no machine needs them.
     */
    private val msxDos2 by lazy {
        FirmwareSet(File(openmsxHome, "user/msxdos2"), listOf(MsxDos2.SYSTEM, MsxDos2.COMMAND).map { file ->
            FirmwareSet.Part(file, UiText.fixed(file), UiText.res(FrontendR.string.kairo_firmware_part, file,
                UiText.res(R.string.dos2_system_file, DOS2)), setOf(file.lowercase()), false, maxBytes = MAX_DOS_FILE)
        }, note = UiText.res(R.string.dos2_note, DOS2))
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
        if (dos2) MsxDos2.install(File(checkNotNull(images[2])), { text(it) }) { name ->
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
        ShippedAssets.install(this, "openmsx", File(home, "system"), progress)
        File(home, "user").mkdirs()
        return home
    }

    private companion object {
        /** The largest MSX system ROM, the turbo R's 4 MiB firmware. */
        const val MAX_SYSTEM_ROM = 4L * 1024 * 1024
        /** Larger than any MSX-DOS2 system file (COMMAND2.COM is about 15 KB). */
        const val MAX_DOS_FILE = 64L * 1024
        const val DOS2 = "MSX-DOS2"
    }
}
