@file:Suppress("DEPRECATION")

package org.simpleble.android

/** Set configuration before retrieving adapters. */
@Deprecated("SimpleDroidBLE is deprecated. Use SimpleKtBLE (org.simpleble:simplektble) instead.")
object Config {
    init { SimpleDroidBle.ensureLoaded() }

    object Android {
        /** Negotiate MTU on connect. Defaults to true. */
        @JvmStatic
        var negotiateMtu: Boolean
            get() = nativeGetNegotiateMtu()
            set(value) = nativeSetNegotiateMtu(value)

        @JvmStatic
        fun reset() { nativeResetAndroid() }
    }

    @JvmStatic
    fun resetAll() { nativeResetAll() }

    private external fun nativeGetNegotiateMtu(): Boolean
    private external fun nativeSetNegotiateMtu(value: Boolean)
    private external fun nativeResetAndroid()
    private external fun nativeResetAll()
}
