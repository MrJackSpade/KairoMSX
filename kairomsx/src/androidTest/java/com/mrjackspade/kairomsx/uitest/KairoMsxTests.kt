package com.mrjackspade.kairomsx.uitest

import android.view.KeyEvent
import com.mrjackspade.kairo.uitest.AppUnderTest
import com.mrjackspade.kairo.uitest.AppUnderTest.BiosFile
import com.mrjackspade.kairo.uitest.KairoUiTest
import com.mrjackspade.kairo.uitest.suites.FirstRunTests
import com.mrjackspade.kairo.uitest.suites.GameTests
import com.mrjackspade.kairo.uitest.suites.LibraryTests
import com.mrjackspade.kairo.uitest.suites.SearchTests
import com.mrjackspade.kairo.uitest.suites.WordingTests
import org.junit.Test

/**
 * KairoMSX as the shared UI tests see it. Its test resources are TOSEC cartridges (The Brain,
 * Boulder Dash, The Dam Busters), a turbo R game, and the Panasonic FS-A1WSX and FS-A1GT firmware;
 * the FS-A1GT's kanji font is the FS-A1WSX file, which the folder import finds by its checksum.
 */
object KairoMsxApp : AppUnderTest() {
    override val name = "KairoMSX"
    override val brand = "KAIROMSX"
    override val biosTitle = "MSX BIOS"
    override val biosFiles = listOf(BiosFile("fs-a1wsx_firmware.rom", true), BiosFile("fs-a1wsx_kanjifont.rom", true),
        BiosFile("fs-a1wsx_IC18.rom", false), BiosFile("fs-a1gt_firmware.rom", true), BiosFile("fs-a1gt_kanjifont.rom", true))
    override val bootGame = "Boulder Dash"
    override val searchTerm = "Boulder"
}

class KairoMsxFirstRunTests : FirstRunTests() { override val app = KairoMsxApp }
class KairoMsxLibraryTests : LibraryTests() { override val app = KairoMsxApp }
class KairoMsxSearchTests : SearchTests() { override val app = KairoMsxApp }
class KairoMsxGameTests : GameTests() { override val app = KairoMsxApp }
class KairoMsxWordingTests : WordingTests() { override val app = KairoMsxApp }

/** The MSX's own behavior: each game starts on the machine it needs, and keeps its own controls. */
class MsxTests : KairoUiTest() {
    override val app = KairoMsxApp

    private fun startedMachine(title: String): String {
        screens.completeFirstRun().open(title).play()
        return driver.probe().details["machine.machine"].orEmpty()
    }

    @Test
    fun aTurboRGameStartsOnTheFsA1gt() =
        expect.equal("the machine a turbo R game started on", "Panasonic_FS-A1GT", startedMachine("Shoot That Flying Windows!"))

    @Test
    fun aBasicCartridgeStartsOnRealFirmware() {
        expect.equal("the machine an MSX-BASIC cartridge started on", "Panasonic_FS-A1WSX", startedMachine("The Brain"))
        driver.waitFrames(300)
        val picture = driver.pictures.game("the-brain")
        expect.that("The Brain draws its screen", picture.litFraction() > 0.02) { "lit ${picture.litFraction()}" }
    }

    @Test
    fun anMsx1CartridgeStartsOnCbiosMsx1() =
        expect.equal("the machine an MSX1 cartridge started on", "C-BIOS_MSX1_JP", startedMachine("Boulder Dash"))

    /** The Dam Busters waits on its credits for the 7 key, which its Start button sends. */
    @Test
    fun theDamBustersStartsFromTheStartButton() {
        screens.completeFirstRun().open("The Dam Busters").play()
        driver.waitFrames(600)
        val credits = driver.pictures.game("credits")
        driver.press(KeyEvent.KEYCODE_BUTTON_START)
        driver.waitFrames(240)
        val after = driver.pictures.game("after-start")
        expect.that("Start leaves the credits screen", after.difference(credits) > 0.2) { "difference ${after.difference(credits)}" }
    }
}
