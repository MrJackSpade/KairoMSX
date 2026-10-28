package com.mrjackspade.kairomsx

import com.mrjackspade.kairo.frontend.CatalogSetting
import com.mrjackspade.kairo.frontend.ChoiceOption
import com.mrjackspade.kairo.frontend.LayeredGameCatalog
import com.mrjackspade.kairo.frontend.MachineOption
import com.mrjackspade.kairo.frontend.UiText
import com.mrjackspade.kairo.frontend.R as FrontendR
import org.json.JSONArray

/**
 * KairoMSX's catalog records beyond the common fields: the machine a game starts on (`machine`,
 * by openMSX's machine name), its main RAM (`memory`, in KB) and whether it starts from MSX-DOS2
 * (`msxdos2`).
 */
object MsxCatalog : LayeredGameCatalog.Schema {
    /** A floppy disk's content ID prefix, the only media a game's disk B may be. */
    private const val FLOPPY = "sha256-fd-v1:"
    override val platform = "msx"
    override val catalogUrl = "https://raw.githubusercontent.com/MrJackSpade/KairoMSX/main/catalog/"
    override val selfHostedArt = "/MrJackSpade/KairoMSX/main/catalog/artwork/art/catalog/"
    override val contentId = Regex("sha256-(?:rom|fd|tape)-v1:[0-9a-f]{64}")
    override val mediaExtensions =
        Regex("\\.(?:zip|7z|rom|mx1|mx2|ri|dsk|di1|di2|dmk|cas|tsx)$", RegexOption.IGNORE_CASE)

    /** [machine]'s automatic choice: [CARTRIDGE_MACHINE] for cartridges, [DISK_MACHINE] for disks and tapes. */
    const val AUTOMATIC = "auto"
    const val CARTRIDGE_MACHINE = "C-BIOS_MSX2+"
    const val DISK_MACHINE = "Panasonic_FS-A1WSX"

    /**
     * The machine a game starts on, by openMSX's name. The C-BIOS machines ship with the app and
     * start cartridges; each MSX generation runs the cartridges of the ones before it, the Japanese
     * machines have the Japanese character set and the European ones run at 50 Hz. The real
     * machines, which disks and tapes need, start with the player's own system ROMs
     * (assets/system_roms.json).
     */
    val machine = MachineOption("machine", UiText.res(FrontendR.string.kairo_option_machine), listOf(
        ChoiceOption(AUTOMATIC, UiText.res(FrontendR.string.kairo_automatic)),
        ChoiceOption("Panasonic_FS-A1WSX", "Panasonic FS-A1WSX (MSX2+)"),
        ChoiceOption("Panasonic_FS-A1GT", "Panasonic FS-A1GT (turbo R)"),
        ChoiceOption("Philips_NMS_8250", "Philips NMS 8250 (MSX2)")) +
        listOf("MSX2+" to "C-BIOS_MSX2+", "MSX2" to "C-BIOS_MSX2", "MSX" to "C-BIOS_MSX1").flatMap { (generation, id) ->
            listOf(ChoiceOption(id, "$generation (C-BIOS)"),
                ChoiceOption("${id}_JP", UiText.res(R.string.machine_japanese, generation, "C-BIOS")),
                ChoiceOption("${id}_EU", UiText.res(R.string.machine_european, generation, "C-BIOS")))
        }, AUTOMATIC)
    /**
     * The main RAM in KB (msx_machine.h kMemorySizes) for a machine whose main RAM is a memory
     * mapper; MSX1 machines and the turbo R keep their own. 512 KB, the largest internal mapper of
     * the era's machines, is the default.
     */
    val memory = MachineOption("memory", UiText.res(FrontendR.string.kairo_option_memory), listOf(64, 128, 256, 512, 1024, 2048, 4096).map { kb ->
        ChoiceOption(kb.toString(), if (kb < 1024) UiText.res(FrontendR.string.kairo_memory_kb, kb) else UiText.res(FrontendR.string.kairo_memory_mb, kb / 1024))
    }, "512", numeric = true)
    /**
     * Whether a disk starts from MSX-DOS2 (MsxDos2): its working copy gets the player's MSX-DOS2
     * files and a DOS2-format boot sector, and Automatic starts it on the turbo R, the only machine
     * with the DOS2 kernel.
     */
    val msxdos2 = MachineOption("msxdos2", UiText.fixed("MSX-DOS2"), listOf(ChoiceOption("off", UiText.res(FrontendR.string.kairo_off)),
        ChoiceOption("on", UiText.res(FrontendR.string.kairo_on))), "off")
    const val DOS2_MACHINE = "Panasonic_FS-A1GT"
    /**
     * The Z80's clock in Hz: Automatic is the machine's own (3.58 MHz, or 5.37 on a Panasonic MSX2+
     * with its turbo on), and the faster choices speed slow games up. A turbo R's R800 keeps its own.
     */
    val clock = MachineOption("clock", UiText.res(FrontendR.string.kairo_option_clock),
        listOf(ChoiceOption(AUTOMATIC, UiText.res(FrontendR.string.kairo_automatic))) +
            listOf("3579545" to "3.58", "5369318" to "5.37", "7159090" to "7.16", "10738635" to "10.74")
                .map { (hz, mhz) -> ChoiceOption(hz, UiText.res(FrontendR.string.kairo_clock_mhz, mhz)) }, AUTOMATIC)
    val options = listOf(machine, clock, memory, msxdos2)

    override val settings: List<CatalogSetting> = options.map(CatalogSetting::Choice) +
        CatalogSetting.Drive("floppyB", UiText.res(FrontendR.string.kairo_drive_b)) { it.startsWith(FLOPPY) }
    override val fields = CatalogSetting.fields(settings)

    override fun validMedia(role: String, contentId: String) = role == "floppyB" && contentId.startsWith(FLOPPY)

    override fun validField(field: String, value: Any): Boolean =
        field in fields && CatalogSetting.validSection(settings, field, value)

    override fun validBindings(bindings: JSONArray) = MsxKeys.bindings.valid(bindings)
}
