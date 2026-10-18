package org.simplektble

import kotlin.jvm.JvmStatic

/** Backend settings. Set them before the first call that reaches a backend. */
public object Config {
    @JvmStatic
    @Throws(SimpleKtBleException::class)
    public fun resetAll() {
        Native.configReset(ConfigGroup.ALL)
    }

    public object SimpleBluez {
        @JvmStatic
        public var useSystemBus: Boolean
            get() = Native.configGetBoolean(ConfigKey.SIMPLEBLUEZ_USE_SYSTEM_BUS)
            set(value) = Native.configSetBoolean(ConfigKey.SIMPLEBLUEZ_USE_SYSTEM_BUS, value)

        /** How long connecting waits for BlueZ, in milliseconds. */
        @JvmStatic
        public var connectionTimeoutMs: Long
            get() = Native.configGetMilliseconds(ConfigKey.SIMPLEBLUEZ_CONNECTION_TIMEOUT)
            set(value) = Native.configSetMilliseconds(ConfigKey.SIMPLEBLUEZ_CONNECTION_TIMEOUT, checkTimeout(value))

        /** How long disconnecting waits for BlueZ, in milliseconds. */
        @JvmStatic
        public var disconnectionTimeoutMs: Long
            get() = Native.configGetMilliseconds(ConfigKey.SIMPLEBLUEZ_DISCONNECTION_TIMEOUT)
            set(value) = Native.configSetMilliseconds(ConfigKey.SIMPLEBLUEZ_DISCONNECTION_TIMEOUT, checkTimeout(value))

        @JvmStatic
        @Throws(SimpleKtBleException::class)
        public fun reset() {
            Native.configReset(ConfigGroup.SIMPLEBLUEZ)
        }
    }

    public object WinRT {
        @JvmStatic
        public var experimentalReinitializeWinrtApartmentOnMainThread: Boolean
            get() = Native.configGetBoolean(ConfigKey.WINRT_REINITIALIZE_APARTMENT_ON_MAIN_THREAD)
            set(value) = Native.configSetBoolean(ConfigKey.WINRT_REINITIALIZE_APARTMENT_ON_MAIN_THREAD, value)

        @JvmStatic
        public var useDeferredDisconnect: Boolean
            get() = Native.configGetBoolean(ConfigKey.WINRT_USE_DEFERRED_DISCONNECT)
            set(value) = Native.configSetBoolean(ConfigKey.WINRT_USE_DEFERRED_DISCONNECT, value)

        @JvmStatic
        @Throws(SimpleKtBleException::class)
        public fun reset() {
            Native.configReset(ConfigGroup.WINRT)
        }
    }

    public object CoreBluetooth {
        @JvmStatic
        @Throws(SimpleKtBleException::class)
        public fun reset() {
            Native.configReset(ConfigGroup.COREBLUETOOTH)
        }
    }

    public object Android {
        /** What each connection asks Android for once connected. [ConnectionPriorityRequest.DISABLED] asks nothing. */
        @JvmStatic
        public var connectionPriorityRequest: ConnectionPriorityRequest
            get() = ConnectionPriorityRequest.fromValue(Native.configGetInt(ConfigKey.ANDROID_CONNECTION_PRIORITY))
            set(value) = Native.configSetInt(ConfigKey.ANDROID_CONNECTION_PRIORITY, value.value)

        @JvmStatic
        @Throws(SimpleKtBleException::class)
        public fun reset() {
            Native.configReset(ConfigGroup.ANDROID)
        }

        /** The connection priorities of Android's `BluetoothGatt.requestConnectionPriority()`. */
        public enum class ConnectionPriorityRequest(internal val value: Int) {
            DISABLED(-1),
            BALANCED(0),
            HIGH(1),
            LOW_POWER(2),
            DCK(3);

            internal companion object {
                fun fromValue(value: Int): ConnectionPriorityRequest = entries.first { it.value == value }
            }
        }
    }

    public object Dongl {
        @JvmStatic
        public var useDonglBackend: Boolean
            get() = Native.configGetBoolean(ConfigKey.DONGL_USE_DONGL_BACKEND)
            set(value) = Native.configSetBoolean(ConfigKey.DONGL_USE_DONGL_BACKEND, value)

        @JvmStatic
        public var autoUpdate: Boolean
            get() = Native.configGetBoolean(ConfigKey.DONGL_AUTO_UPDATE)
            set(value) = Native.configSetBoolean(ConfigKey.DONGL_AUTO_UPDATE, value)

        @JvmStatic
        public var forceUpdate: Boolean
            get() = Native.configGetBoolean(ConfigKey.DONGL_FORCE_UPDATE)
            set(value) = Native.configSetBoolean(ConfigKey.DONGL_FORCE_UPDATE, value)

        @JvmStatic
        @Throws(SimpleKtBleException::class)
        public fun reset() {
            Native.configReset(ConfigGroup.DONGL)
        }
    }
}

/** The settings Native reads and writes. The native side uses the same numbers. */
internal object ConfigKey {
    const val SIMPLEBLUEZ_USE_SYSTEM_BUS = 0
    const val SIMPLEBLUEZ_CONNECTION_TIMEOUT = 1
    const val SIMPLEBLUEZ_DISCONNECTION_TIMEOUT = 2
    const val WINRT_REINITIALIZE_APARTMENT_ON_MAIN_THREAD = 3
    const val WINRT_USE_DEFERRED_DISCONNECT = 4
    const val DONGL_USE_DONGL_BACKEND = 5
    const val DONGL_AUTO_UPDATE = 6
    const val DONGL_FORCE_UPDATE = 7
    const val ANDROID_CONNECTION_PRIORITY = 8
}

/** The groups Native resets. The native side uses the same numbers. */
internal object ConfigGroup {
    const val ALL = 0
    const val SIMPLEBLUEZ = 1
    const val WINRT = 2
    const val COREBLUETOOTH = 3
    const val DONGL = 4
    const val ANDROID = 5
}

// SimpleBLE keeps timeouts in nanoseconds, so longer ones do not fit.
private const val MAX_TIMEOUT_MS = Long.MAX_VALUE / 1_000_000

private fun checkTimeout(milliseconds: Long): Long {
    require(milliseconds in 0..MAX_TIMEOUT_MS) { "Timeouts must be between 0 and $MAX_TIMEOUT_MS milliseconds" }
    return milliseconds
}
