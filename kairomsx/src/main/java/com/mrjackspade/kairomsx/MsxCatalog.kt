package com.mrjackspade.kairomsx

import com.mrjackspade.kairo.frontend.ChoiceOption
import com.mrjackspade.kairo.frontend.LayeredGameCatalog
import com.mrjackspade.kairo.frontend.MachineOption
import org.json.JSONArray

/**
 * KairoMSX's catalog records beyond the common fields: the machine a game starts on (`machine`,
 * by openMSX's machine name) and its main RAM (`memory`, in KB).
 */
object MsxCatalog : LayeredGameCatalog.Schema {
    override val platform = "msx"
    override val updateUrl = "https://raw.githubusercontent.com/MrJackSpade/KairoMSX/main/catalog/core-v3.json"
    override val metadataUrl = "https://raw.githubusercontent.com/MrJackSpade/KairoMSX/main/catalog/core-v3.meta.json"
    override val coreVersion = 3
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
    val machine = MachineOption("machine", "Machine", listOf(
        ChoiceOption(AUTOMATIC, "Automatic"),
        ChoiceOption("Panasonic_FS-A1WSX", "Panasonic FS-A1WSX (MSX2+)"),
        ChoiceOption("Panasonic_FS-A1GT", "Panasonic FS-A1GT (turbo R)"),
        ChoiceOption("Philips_NMS_8250", "Philips NMS 8250 (MSX2)"),
        ChoiceOption("C-BIOS_MSX2+", "MSX2+ (C-BIOS)"),
        ChoiceOption("C-BIOS_MSX2+_JP", "MSX2+ Japanese (C-BIOS)"),
        ChoiceOption("C-BIOS_MSX2+_EU", "MSX2+ European (C-BIOS)"),
        ChoiceOption("C-BIOS_MSX2", "MSX2 (C-BIOS)"),
        ChoiceOption("C-BIOS_MSX2_JP", "MSX2 Japanese (C-BIOS)"),
        ChoiceOption("C-BIOS_MSX2_EU", "MSX2 European (C-BIOS)"),
        ChoiceOption("C-BIOS_MSX1", "MSX (C-BIOS)"),
        ChoiceOption("C-BIOS_MSX1_JP", "MSX Japanese (C-BIOS)"),
        ChoiceOption("C-BIOS_MSX1_EU", "MSX European (C-BIOS)")), AUTOMATIC)
    /**
     * The main RAM in KB (msx_machine.h kMemorySizes) for a machine whose main RAM is a memory
     * mapper; MSX1 machines and the turbo R keep their own. 512 KB, the largest internal mapper of
     * the era's machines, is the default.
     */
    val memory = MachineOption("memory", "Memory", listOf(64, 128, 256, 512, 1024, 2048, 4096).map { kb ->
        ChoiceOption(kb.toString(), if (kb < 1024) "$kb KB" else "${kb / 1024} MB")
    }, "512", numeric = true)
    val options = listOf(machine, memory)

    override val fields: Map<String, Set<String>?> = mapOf(MachineOption.SECTION to options.map { it.id }.toSet())
    override val singleMediaRoles = emptySet<String>()

    override fun validMedia(role: String, contentId: String) = false

    override fun validField(field: String, value: Any): Boolean =
        field == MachineOption.SECTION && MachineOption.validSection(options, value)

    override fun validBindings(bindings: JSONArray) = MsxKeys.bindings.valid(bindings)
}
