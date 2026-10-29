package com.mrjackspade.kairomsx

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class MsxCatalogTest {
    /** A game opened at its prompt needs MSX-BASIC, which only the real machines have. */
    @Test
    fun onlyRealMachinesHaveBasic() {
        assertTrue(MsxCatalog.hasBasic(MsxCatalog.DISK_MACHINE))
        assertTrue(MsxCatalog.hasBasic("Philips_NMS_8250"))
        assertFalse(MsxCatalog.hasBasic(MsxCatalog.AUTOMATIC))
        assertFalse(MsxCatalog.hasBasic(MsxCatalog.CARTRIDGE_MACHINE))
        assertFalse(MsxCatalog.hasBasic("C-BIOS_MSX1_JP"))
    }
}
