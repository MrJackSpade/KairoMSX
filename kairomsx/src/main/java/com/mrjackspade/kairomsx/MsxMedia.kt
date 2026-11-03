package com.mrjackspade.kairomsx

import com.mrjackspade.kairo.frontend.DiskImageLibrary
import com.mrjackspade.kairo.frontend.R as FrontendR
import com.mrjackspade.kairo.frontend.UiText

/** The library's media format, as the app's launch builds it (Kairo#125). */
object MsxMedia {
    /** Cartridge, disk and tape images in the formats openMSX reads. */
    val format = DiskImageLibrary.Format(
        kinds = listOf("rom", "mx1", "mx2", "ri").associateWith { "rom" } +
            listOf("dsk", "di1", "di2", "dmk").associateWith { "fd" } + listOf("cas", "tsx").associateWith { "tape" } +
            listOf("hdd", "ide", "hda", "img").associateWith { "hd" },
        labels = mapOf("rom" to UiText.res(FrontendR.string.kairo_cartridge), "fd" to UiText.res(FrontendR.string.kairo_floppy_disk),
            "tape" to UiText.res(FrontendR.string.kairo_cassette), "hd" to UiText.res(FrontendR.string.kairo_hard_disk)),
        maxBytes = mapOf("rom" to 16L * 1024 * 1024, "fd" to 16L * 1024 * 1024, "tape" to 16L * 1024 * 1024,
            "hd" to 4L * 1024 * 1024 * 1024),
        guestName = "MSX")
}
