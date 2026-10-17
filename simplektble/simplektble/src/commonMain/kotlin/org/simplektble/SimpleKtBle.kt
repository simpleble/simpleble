package org.simplektble

public object SimpleKtBle {
    /** The version of SimpleBLE this library was built from. */
    public val version: String
        get() = Native.version()
}
