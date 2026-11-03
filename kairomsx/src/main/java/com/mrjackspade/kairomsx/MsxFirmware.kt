package com.mrjackspade.kairomsx

import com.mrjackspade.kairo.frontend.FirmwareSet
import com.mrjackspade.kairo.frontend.R as FrontendR
import com.mrjackspade.kairo.frontend.UiText
import org.json.JSONObject
import java.io.File

/** The firmware the Machine page lists, as the app's launch builds it (Kairo#125). */
object MsxFirmware {
    /** The largest MSX system ROM, the turbo R's 4 MiB firmware. */
    private const val MAX_SYSTEM_ROM = 4L * 1024 * 1024
    /** Larger than any MSX-DOS2 system file (COMMAND2.COM is about 15 KB). */
    private const val MAX_DOS_FILE = 64L * 1024
    const val DOS2 = "MSX-DOS2"

    /**
     * Each real machine's set, from [systemRomsJson] (the app's system_roms.json, system_roms.py),
     * with its files in [systemRoms], openMSX's systemroms folder.
     */
    fun machines(systemRomsJson: String, systemRoms: File): List<Pair<UiText, FirmwareSet>> {
        val machines = JSONObject(systemRomsJson).getJSONArray("machines")
        return (0 until machines.length()).map { index ->
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
        }
    }

    /**
     * The player's MSX-DOS2 system files in [directory], for disks that start from MSX-DOS2 without
     * them (MsxDos2): ASCII's software, which the app cannot ship. Found by name; no machine needs them.
     */
    fun msxDos2(directory: File) = FirmwareSet(directory, listOf(MsxDos2.SYSTEM, MsxDos2.COMMAND).map { file ->
        FirmwareSet.Part(file, UiText.fixed(file), UiText.res(FrontendR.string.kairo_firmware_part, file,
            UiText.res(R.string.dos2_system_file, DOS2)), setOf(file.lowercase()), false, maxBytes = MAX_DOS_FILE)
    }, note = UiText.res(R.string.dos2_note, DOS2))
}
