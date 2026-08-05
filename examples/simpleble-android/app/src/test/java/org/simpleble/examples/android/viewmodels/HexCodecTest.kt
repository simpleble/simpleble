package org.simpleble.examples.android.viewmodels

import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertThrows
import org.junit.Test

class HexCodecTest {
    @Test
    fun parsesCommonHexFormats() {
        assertArrayEquals(
            byteArrayOf(0x01, 0x02, 0xff.toByte()),
            parseHex("0x01 02:ff")
        )
    }

    @Test
    fun rejectsIncompleteBytes() {
        assertThrows(IllegalArgumentException::class.java) {
            parseHex("abc")
        }
    }

    @Test
    fun rejectsInvalidCharactersInsteadOfDiscardingThem() {
        assertThrows(IllegalArgumentException::class.java) {
            parseHex("01 nope 02")
        }
    }

    @Test
    fun rejectsEmptyWrites() {
        assertThrows(IllegalArgumentException::class.java) {
            parseHex("  ")
        }
    }

    @Test
    fun formatsUnsignedBytes() {
        assertEquals("00 7f 80 ff", byteArrayOf(0, 0x7f, 0x80.toByte(), 0xff.toByte()).toHexString())
    }
}
