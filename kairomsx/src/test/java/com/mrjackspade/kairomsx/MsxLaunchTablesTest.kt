package com.mrjackspade.kairomsx

import com.mrjackspade.kairo.KanaLegendsTest
import com.mrjackspade.kairo.LaunchTablesTest
import java.io.File

/** What KairoMSX builds as it launches (Kairo#125, KairoMSX#52), its system_roms.json as the app ships it. */
class MsxLaunchTablesTest : LaunchTablesTest() {
    override fun tables() = LaunchTables(listOf(MsxMedia.format),
        listOf(MsxKeys.layout, MsxKeys.jisLayout, MsxKeys.japaneseAnsiLayout),
        MsxFirmware.machines(File("src/main/assets/system_roms.json").readText(), File("systemroms")).map { it.second } +
            MsxFirmware.msxDos2(File("msxdos2")),
        mapOf("controller bindings" to { MsxKeys.bindings }, "catalog machines" to { MsxCatalog.options }))
}

/** The JIS MSX keyboard's kana legends (Kairo#126, KairoMSX#60); a jp_ansi model's kana are elsewhere. */
class MsxKanaLegendsTest : KanaLegendsTest() {
    override val layouts = listOf(MsxKeys.jisLayout)
}
