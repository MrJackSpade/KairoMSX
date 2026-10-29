package com.mrjackspade.kairomsx

import org.junit.Assert.assertEquals
import org.junit.Test

class MsxKeysTest {
    /** The layouts are built as MsxKeys initializes, which the app does as it starts. */
    @Test
    fun layoutsBuild() {
        for (layout in listOf(MsxKeys.jisLayout, MsxKeys.japaneseAnsiLayout)) {
            val alphabet = layout.pages.first().rows
            assertEquals(5, alphabet.size)
            // Esc, the ten digits, - ^ ¥ and BS.
            assertEquals(15, alphabet.first().size)
        }
    }
}
