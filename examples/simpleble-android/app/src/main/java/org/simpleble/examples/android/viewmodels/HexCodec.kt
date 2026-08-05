package org.simpleble.examples.android.viewmodels

import java.util.Locale

internal fun ByteArray.toHexString(): String {
    return joinToString(" ") { byte ->
        "%02x".format(Locale.US, byte.toInt() and 0xff)
    }
}

internal fun parseHex(value: String): ByteArray {
    val withoutPrefixes = value.trim().replace(Regex("(?i)(^|[\\s,:;_-])0x")) { match ->
        match.groupValues[1]
    }
    val normalized = withoutPrefixes.replace(Regex("[\\s,:;_-]"), "")

    require(normalized.isNotEmpty()) { "Enter at least one byte." }
    require(normalized.all { it.isDigit() || it.lowercaseChar() in 'a'..'f' }) {
        "Hex input contains an invalid character."
    }
    require(normalized.length % 2 == 0) { "Hex input must contain complete bytes." }

    return normalized.chunked(2)
        .map { it.toInt(16).toByte() }
        .toByteArray()
}
