package org.simplektble

/** A failure reported by SimpleBLE, with its [code]. */
public class SimpleKtBleException(public val code: ErrorCode, message: String) : RuntimeException(message) {
    // NOTE: The JNI library raises it with the numeric code SimpleBLE uses.
    internal constructor(code: Int, message: String) : this(ErrorCode.fromValue(code), message)
}

/** Runs [block] and ignores SimpleBLE's failures, for teardown steps that must not stop the ones after them. */
internal inline fun ignoringFailures(block: () -> Unit) {
    try {
        block()
    } catch (exception: SimpleKtBleException) {
        // NOTE: The failure is ignored on purpose.
    }
}
