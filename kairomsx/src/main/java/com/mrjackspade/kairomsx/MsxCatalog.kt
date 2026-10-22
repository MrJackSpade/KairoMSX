package com.mrjackspade.kairomsx

import com.mrjackspade.kairo.frontend.ChoiceOption
import com.mrjackspade.kairo.frontend.LayeredGameCatalog
import com.mrjackspade.kairo.frontend.MachineOption
import org.json.JSONArray

/**
 * KairoMSX's catalog records beyond the common fields: the machine a game starts on (`machine`,
 * by openMSX's machine name).
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

    /**
     * The machines the app ships: C-BIOS, which starts cartridges. Each MSX generation runs the
     * cartridges of the ones before it; the Japanese machines have the Japanese character set and
     * the European ones run at 50 Hz.
     */
    val machine = MachineOption("machine", "Machine", listOf(
        ChoiceOption("C-BIOS_MSX2+", "MSX2+"),
        ChoiceOption("C-BIOS_MSX2+_JP", "MSX2+ Japanese"),
        ChoiceOption("C-BIOS_MSX2+_EU", "MSX2+ European"),
        ChoiceOption("C-BIOS_MSX2", "MSX2"),
        ChoiceOption("C-BIOS_MSX2_JP", "MSX2 Japanese"),
        ChoiceOption("C-BIOS_MSX2_EU", "MSX2 European"),
        ChoiceOption("C-BIOS_MSX1", "MSX"),
        ChoiceOption("C-BIOS_MSX1_JP", "MSX Japanese"),
        ChoiceOption("C-BIOS_MSX1_EU", "MSX European")), "C-BIOS_MSX2+")
    val options = listOf(machine)

    override val fields: Map<String, Set<String>?> = mapOf(MachineOption.SECTION to options.map { it.id }.toSet())
    override val singleMediaRoles = emptySet<String>()

    override fun validMedia(role: String, contentId: String) = false

    override fun validField(field: String, value: Any): Boolean =
        field == MachineOption.SECTION && MachineOption.validSection(options, value)

    override fun validBindings(bindings: JSONArray) = MsxKeys.bindings.valid(bindings)
}
