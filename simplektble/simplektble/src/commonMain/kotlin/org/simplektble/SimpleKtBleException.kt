package org.simplektble

/** A failure reported by SimpleBLE, with its [code]. */
public class SimpleKtBleException(public val code: ErrorCode, message: String) : RuntimeException(message) {
    // NOTE: The JNI library raises it with the numeric code SimpleBLE uses.
    internal constructor(code: Int, message: String) : this(ErrorCode.fromValue(code), message)
}
