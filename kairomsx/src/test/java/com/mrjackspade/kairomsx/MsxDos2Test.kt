package com.mrjackspade.kairomsx

import com.mrjackspade.kairo.frontend.UiText
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File

class MsxDos2Test {
    @get:Rule val folder = TemporaryFolder()

    /** A failure as its resource and arguments, which a test compares without a Context. */
    private val text: (UiText) -> String = { (it as UiText.Res).let { res -> "${res.id} ${res.args}" } }

    /** A blank 720 KB MSX disk: 2 sectors a cluster, 2 FATs of 3 sectors, 112 root entries. */
    private fun blankDisk(): ByteArray {
        val image = ByteArray(1440 * 512)
        image[0] = 0xEB.toByte(); image[1] = 0xFE.toByte(); image[2] = 0x90.toByte()
        fun u16(at: Int, value: Int) { image[at] = value.toByte(); image[at + 1] = (value shr 8).toByte() }
        u16(11, 512); image[13] = 2; u16(14, 1); image[16] = 2; u16(17, 112); u16(19, 1440)
        image[21] = 0xF9.toByte(); u16(22, 3); u16(24, 9); u16(26, 2)
        // A DOS1 boot routine at 0x1E, as most disks have.
        image[0x1E] = 0xD0.toByte(); image[0x1F] = 0xED.toByte()
        for (fat in 0 until 2) {
            val at = 512 + fat * 3 * 512
            image[at] = 0xF9.toByte(); image[at + 1] = 0xFF.toByte(); image[at + 2] = 0xFF.toByte()
        }
        return image
    }

    private fun disk(bytes: ByteArray) = folder.newFile("game.dsk").apply { writeBytes(bytes) }
    private fun copy(name: String, size: Int) = folder.newFile(name).apply { writeBytes(ByteArray(size) { (it % 251).toByte() }) }

    /** The bytes of [name] in [image], read back through its root entry and cluster chain. */
    private fun read(image: ByteArray, name: String): ByteArray? {
        fun u8(at: Int) = image[at].toInt() and 0xff
        fun u16(at: Int) = u8(at) or (u8(at + 1) shl 8)
        val wanted = (name.substringBefore('.').padEnd(8) + name.substringAfter('.').padEnd(3)).toByteArray()
        val root = (1 + 2 * 3) * 512
        val data = root + 112 * 32
        for (index in 0 until 112) {
            val at = root + index * 32
            if (!image.copyOfRange(at, at + 11).contentEquals(wanted)) continue
            val size = u16(at + 28) or (u16(at + 30) shl 16)
            val out = java.io.ByteArrayOutputStream()
            var cluster = u16(at + 26)
            while (cluster in 2..0xff6) {
                out.write(image, data + (cluster - 2) * 1024, 1024)
                val pair = u16(512 + cluster * 3 / 2)
                cluster = if (cluster and 1 == 1) pair shr 4 else pair and 0xfff
            }
            return out.toByteArray().copyOf(size)
        }
        return null
    }

    @Test
    fun aDiskWithoutDos2GetsTheFilesAndADos2BootSector() {
        val image = disk(blankDisk())
        val system = copy("MSXDOS2.SYS", 4480)
        val command = copy("COMMAND2.COM", 15000)
        MsxDos2.install(image, text) { mapOf(MsxDos2.SYSTEM to system, MsxDos2.COMMAND to command)[it] }
        val result = image.readBytes()
        assertArrayEquals(system.readBytes(), read(result, MsxDos2.SYSTEM))
        assertArrayEquals(command.readBytes(), read(result, MsxDos2.COMMAND))
        assertTrue(MsxDos2.dos2Format(result))
        assertEquals(0x18, result[0x1E].toInt() and 0xff)
        assertEquals(0x10, result[0x1F].toInt() and 0xff)
        assertEquals(0xD0, result[0x30].toInt() and 0xff)
        assertEquals(0xC9, result[0x31].toInt() and 0xff)
        // Both FAT copies agree.
        assertArrayEquals(result.copyOfRange(512, 512 + 3 * 512), result.copyOfRange(512 + 3 * 512, 512 + 6 * 512))
    }

    @Test
    fun aDiskThatHasTheFilesOnlyGetsTheBootSector() {
        val image = disk(blankDisk())
        val system = copy("MSXDOS2.SYS", 4480)
        val command = copy("COMMAND2.COM", 15000)
        MsxDos2.install(image, text) { mapOf(MsxDos2.SYSTEM to system, MsxDos2.COMMAND to command)[it] }
        val withFiles = image.readBytes()
        // Back to a DOS1 boot sector: the files stay, and none is asked for again.
        withFiles[0x20] = 0
        image.writeBytes(withFiles)
        MsxDos2.install(image, text) { error("asked for $it") }
        assertTrue(MsxDos2.dos2Format(image.readBytes()))
    }

    @Test
    fun aReadyDiskIsLeftAlone() {
        val image = disk(blankDisk())
        val system = copy("MSXDOS2.SYS", 4480)
        val command = copy("COMMAND2.COM", 15000)
        MsxDos2.install(image, text) { mapOf(MsxDos2.SYSTEM to system, MsxDos2.COMMAND to command)[it] }
        val ready = image.readBytes()
        MsxDos2.install(image, text) { error("asked for $it") }
        assertArrayEquals(ready, image.readBytes())
    }

    @Test
    fun aMissingFileFailsTheStartAndLeavesTheDiskAlone() {
        val blank = blankDisk()
        val image = disk(blank)
        val message = runCatching { MsxDos2.install(image, text) { null } }.exceptionOrNull()?.message.orEmpty()
        assertEquals(text(UiText.res(R.string.dos2_missing, "MSX-DOS2", MsxDos2.SYSTEM, MsxDos2.COMMAND)), message)
        assertArrayEquals(blank, image.readBytes())
    }

    @Test
    fun aFullDiskFailsTheStartAndLeavesTheDiskAlone() {
        val full = blankDisk()
        // Every cluster in use.
        for (fat in 0 until 2) for (at in 512 + fat * 1536 + 3 until 512 + fat * 1536 + 1536) full[at] = 0xFF.toByte()
        val image = disk(full)
        val result = runCatching { MsxDos2.install(image, text) { copy(it, 4480) } }
        assertEquals(text(UiText.res(R.string.disk_full, MsxDos2.SYSTEM)), result.exceptionOrNull()?.message)
        assertArrayEquals(full, image.readBytes())
    }

    @Test
    fun aDiskWithoutAFileSystemIsRefused() {
        val image = disk(ByteArray(1440 * 512))
        val message = runCatching { MsxDos2.install(image, text) { null } }.exceptionOrNull()?.message.orEmpty()
        assertEquals(text(UiText.res(R.string.disk_not_fat12)), message)
    }
}
