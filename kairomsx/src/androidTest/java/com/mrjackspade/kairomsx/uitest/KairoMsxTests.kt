package com.mrjackspade.kairomsx.uitest

import android.view.KeyEvent
import com.mrjackspade.kairo.uitest.AppUnderTest
import com.mrjackspade.kairo.uitest.AppUnderTest.BiosFile
import com.mrjackspade.kairo.uitest.AppUnderTest.MediaSwap
import com.mrjackspade.kairo.uitest.KairoUiTest
import com.mrjackspade.kairo.uitest.suites.FirstRunTests
import com.mrjackspade.kairo.uitest.suites.GameTests
import com.mrjackspade.kairo.uitest.suites.LibraryTests
import com.mrjackspade.kairo.uitest.suites.SearchTests
import com.mrjackspade.kairo.uitest.suites.WordingTests
import com.mrjackspade.kairo.uitest.suites.MediaTests
import com.mrjackspade.kairo.uitest.suites.MenuTests
import com.mrjackspade.kairo.uitest.suites.ControllerTests
import com.mrjackspade.kairo.uitest.suites.TouchTests
import com.mrjackspade.kairo.uitest.suites.LaunchTests
import com.mrjackspade.kairo.uitest.suites.LibraryManagementTests
import com.mrjackspade.kairo.uitest.suites.SettingsTests
import com.mrjackspade.kairo.uitest.suites.StateTests
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
    override val arrowKeys = listOf(com.mrjackspade.kairomsx.MsxKeys.UP, com.mrjackspade.kairomsx.MsxKeys.DOWN,
        com.mrjackspade.kairomsx.MsxKeys.LEFT, com.mrjackspade.kairomsx.MsxKeys.RIGHT)
    override val returnKey = com.mrjackspade.kairomsx.MsxKeys.RETURN
    override val spaceKey = com.mrjackspade.kairomsx.MsxKeys.SPACE
    override val bootGameFile = "games/msx/Champion Boulder Dash (1985) (Comptiq).zip"
    override val otherGame = "The Dam Busters"
    override val mediaSwap = MediaSwap("The Dam Busters", "Cartridge B", "Brain")
}

class KairoMsxFirstRunTests : FirstRunTests() { override val app = KairoMsxApp }
class KairoMsxLibraryTests : LibraryTests() { override val app = KairoMsxApp }
class KairoMsxSearchTests : SearchTests() { override val app = KairoMsxApp }
class KairoMsxGameTests : GameTests() { override val app = KairoMsxApp }
class KairoMsxWordingTests : WordingTests() { override val app = KairoMsxApp }
class KairoMsxMediaTests : MediaTests() { override val app = KairoMsxApp }
class KairoMsxMenuTests : MenuTests() { override val app = KairoMsxApp }
class KairoMsxControllerTests : ControllerTests() { override val app = KairoMsxApp }
class KairoMsxTouchTests : TouchTests() { override val app = KairoMsxApp }
class KairoMsxLaunchTests : LaunchTests() { override val app = KairoMsxApp }
class KairoMsxLibraryManagementTests : LibraryManagementTests() { override val app = KairoMsxApp }
class KairoMsxSettingsTests : SettingsTests() { override val app = KairoMsxApp }
class KairoMsxStateTests : StateTests() { override val app = KairoMsxApp }

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
    fun theTurboRKanjiFontIsFoundByItsContent() {
        screens.completeFirstRun()
        val machine = screens.openMachinePage()
        expect.equal("the FS-A1GT kanji font after importing a folder that holds only the FS-A1WSX one", "Installed",
            screens.biosRow(machine, "fs-a1gt_kanjifont.rom").find(com.mrjackspade.kairo.uitest.By.id("value")).text())
    }

    @Test
    fun aGamesOwnMachineChoiceWins() {
        val library = screens.completeFirstRun()
        val settings = library.open("Boulder Dash").openSettings()
        screens.choose(settings, "machine", "C-BIOS_MSX2+_JP")
        driver.press(KeyEvent.KEYCODE_BUTTON_B)
        com.mrjackspade.kairo.uitest.GamePage(driver).play()
        expect.equal("the machine Boulder Dash started on after choosing its own", "C-BIOS_MSX2+_JP",
            driver.probe().details["machine.machine"])
    }

    /** KairoMSX#3: the Memory setting reaches the machine, 512 KB unless the game is given another. */
    @Test
    fun theMemorySettingReachesTheMachine() {
        val library = screens.completeFirstRun()
        library.open("Boulder Dash").play()
        expect.equal("the memory a game starts with by default", "512", driver.probe().details["machine.memory"])
        screens.endGame()
        val settings = library.open("Boulder Dash").openSettings()
        screens.choose(settings, "memory", "1024")
        driver.press(KeyEvent.KEYCODE_BUTTON_B)
        com.mrjackspade.kairo.uitest.GamePage(driver).play()
        expect.equal("the memory after choosing 1 MB for the game", "1024", driver.probe().details["machine.memory"])
    }

    /**
     * KairoMSX#6: Dragon Quest 1 answers the controller. Its title waits for a key: A (Return) must
     * change the picture by more than the title changes on its own over the same frames.
     */
    @Test
    fun dragonQuest1AnswersTheController() {
        screens.completeFirstRun().open("Dragon Quest 1").play()
        driver.waitFrames(900)
        val title = driver.pictures.game("title")
        driver.waitFrames(180)
        val idle = driver.pictures.game("title-idle")
        driver.press(KeyEvent.KEYCODE_BUTTON_A)
        driver.waitFrames(180)
        val pressed = driver.pictures.game("after-a")
        val alone = idle.difference(title)
        val withA = pressed.difference(idle)
        expect.that("A changes the title screen beyond its own motion", withA > alone + 0.05) { "%.3f with A, %.3f alone".format(withA, alone) }
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
