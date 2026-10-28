package com.mrjackspade.kairomsx

import com.mrjackspade.kairo.frontend.UiText
import java.io.File
import java.nio.file.Files
import java.nio.file.StandardCopyOption

/**
 * Makes a disk start MSX-DOS2 on a machine with its kernel (the turbo R): adds [SYSTEM] and
 * [COMMAND] to its root folder when it lacks them, from the player's own copies, and marks a
 * DOS1-format boot sector as DOS2 format, without which the kernel starts Disk BASIC instead.
 *
 * The mark is a jump at 0x1E to 0x30, VOL_ID at 0x20, and at 0x30 a boot routine of KairoMSX's
 * own, RET NC then RET: the kernel loads MSXDOS2.SYS itself, and calls the routine only to start
 * MSX-DOS1. The image is a FAT12 sector image changed in place: it is the game's working copy,
 * which keeps the change.
 */
object MsxDos2 {
    const val SYSTEM = "MSXDOS2.SYS"
    const val COMMAND = "COMMAND2.COM"

    /** The boot sector from 0x1E: the jump, the DOS2 signature, a zero volume ID, and the routine. */
    private val BOOT = byteArrayOf(0x18, 0x10, 'V'.code.toByte(), 'O'.code.toByte(), 'L'.code.toByte(),
        '_'.code.toByte(), 'I'.code.toByte(), 'D'.code.toByte(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0xD0.toByte(), 0xC9.toByte())
    private const val BOOT_AT = 0x1E

    /**
     * Readies [image] to start MSX-DOS2. [copy] gives the player's copy of a file the disk lacks,
     * or null when it was never imported, which fails the start with what to import. [text] puts
     * a failure in the player's language.
     */
    fun install(image: File, text: (UiText) -> String, copy: (String) -> File?) {
        val bytes = image.readBytes()
        val disk = Fat12(bytes, text)
        var changed = false
        for (name in listOf(SYSTEM, COMMAND)) {
            if (disk.has(name)) continue
            val source = copy(name) ?: error(text(UiText.res(R.string.dos2_missing, "MSX-DOS2", SYSTEM, COMMAND)))
            disk.add(name, source.readBytes())
            changed = true
        }
        if (!dos2Format(bytes)) {
            BOOT.copyInto(bytes, BOOT_AT)
            changed = true
        }
        if (!changed) return
        val partial = File(image.path + ".part")
        partial.writeBytes(bytes)
        Files.move(partial.toPath(), image.toPath(), StandardCopyOption.REPLACE_EXISTING, StandardCopyOption.ATOMIC_MOVE)
    }

    /** Whether a boot sector is DOS2 format: VOL_ID at 0x20. */
    fun dos2Format(image: ByteArray) = image.size >= 0x26 &&
        String(image, 0x20, 6, Charsets.ISO_8859_1) == "VOL_ID"
}

/**
 * The root folder and allocation table of a FAT12 sector image, changed in [image] itself. [text]
 * puts a failure the player sees in their language.
 */
internal class Fat12(private val image: ByteArray, private val text: (UiText) -> String) {
    private fun u8(at: Int) = image[at].toInt() and 0xff
    private fun u16(at: Int) = u8(at) or (u8(at + 1) shl 8)

    private val sectorBytes = u16(11)
    private val clusterSectors = u8(13)
    private val reserved = u16(14)
    private val fats = u8(16)
    private val rootEntries = u16(17)
    private val totalSectors = u16(19)
    private val fatSectors = u16(22)

    init {
        require(sectorBytes == 512 && clusterSectors in setOf(1, 2, 4, 8) && fats in 1..2 && rootEntries in 1..512 &&
            fatSectors > 0 && totalSectors.toLong() * sectorBytes <= image.size) { text(UiText.res(R.string.disk_not_fat12)) }
    }

    private val fatAt = reserved * sectorBytes
    private val rootAt = (reserved + fats * fatSectors) * sectorBytes
    private val dataAt = rootAt + rootEntries * 32
    private val clusterBytes = clusterSectors * sectorBytes
    private val clusters = (totalSectors * sectorBytes - dataAt) / clusterBytes + 2

    private fun entry(index: Int) = rootAt + index * 32

    /** The 11-byte directory form of an 8.3 name, such as "MSXDOS2 SYS". */
    private fun directoryName(name: String): ByteArray {
        val stem = name.substringBeforeLast('.')
        val extension = name.substringAfterLast('.', "")
        require(stem.length in 1..8 && extension.length <= 3) { "$name is not an 8.3 name" }
        return (stem.padEnd(8) + extension.padEnd(3)).toByteArray(Charsets.ISO_8859_1)
    }

    fun has(name: String): Boolean {
        val wanted = directoryName(name)
        for (index in 0 until rootEntries) {
            val at = entry(index)
            if (u8(at) == 0) return false
            if (u8(at) != 0xE5 && image.copyOfRange(at, at + 11).contentEquals(wanted)) return true
        }
        return false
    }

    private fun next(cluster: Int): Int {
        val pair = u16(fatAt + cluster * 3 / 2)
        return if (cluster and 1 == 1) pair shr 4 else pair and 0xfff
    }

    private fun link(cluster: Int, value: Int) {
        for (copy in 0 until fats) {
            val at = fatAt + copy * fatSectors * sectorBytes + cluster * 3 / 2
            val pair = u16(at)
            val updated = if (cluster and 1 == 1) (pair and 0x000f) or (value shl 4) else (pair and 0xf000) or value
            image[at] = updated.toByte()
            image[at + 1] = (updated shr 8).toByte()
        }
    }

    /** Adds [name] to the root folder with [body], in free clusters. */
    fun add(name: String, body: ByteArray) {
        require(body.isNotEmpty()) { "$name is empty" }
        val slot = (0 until rootEntries).firstOrNull { u8(entry(it)) == 0 || u8(entry(it)) == 0xE5 }
            ?: error(text(UiText.res(R.string.disk_root_full, name)))
        val needed = (body.size + clusterBytes - 1) / clusterBytes
        val free = (2 until clusters).filter { next(it) == 0 }.take(needed)
        check(free.size == needed) { text(UiText.res(R.string.disk_full, name)) }
        free.forEachIndexed { index, cluster ->
            link(cluster, free.getOrElse(index + 1) { 0xfff })
            val start = index * clusterBytes
            val end = minOf(body.size, start + clusterBytes)
            body.copyInto(image, dataAt + (cluster - 2) * clusterBytes, start, end)
        }
        val at = entry(slot)
        image.fill(0, at, at + 32)
        directoryName(name).copyInto(image, at)
        image[at + 11] = 0x20 // archive
        // 1 January 1990, as no date of its own exists.
        image[at + 24] = (DATE and 0xff).toByte()
        image[at + 25] = (DATE shr 8).toByte()
        image[at + 26] = (free[0] and 0xff).toByte()
        image[at + 27] = (free[0] shr 8).toByte()
        for (byte in 0 until 4) image[at + 28 + byte] = (body.size shr (8 * byte)).toByte()
    }

    private companion object {
        const val DATE = ((1990 - 1980) shl 9) or (1 shl 5) or 1
    }
}
