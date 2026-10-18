package org.simplektble

import kotlin.jvm.JvmStatic
import kotlin.time.Duration
import kotlin.time.Duration.Companion.milliseconds
import kotlin.time.Duration.Companion.nanoseconds

/** Backend settings. Set them before the first call that reaches a backend. */
public object Config {
    @JvmStatic
    @Throws(SimpleKtBleException::class)
    public fun resetAll() {
        Native.configReset(ConfigGroup.ALL)
    }

    public object SimpleBluez {
        public var useSystemBus: Boolean
            get() = Native.configGetBoolean(ConfigKey.SIMPLEBLUEZ_USE_SYSTEM_BUS)
            set(value) = Native.configSetBoolean(ConfigKey.SIMPLEBLUEZ_USE_SYSTEM_BUS, value)

        public var connectionTimeout: Duration
            get() = Native.configGetMilliseconds(ConfigKey.SIMPLEBLUEZ_CONNECTION_TIMEOUT).milliseconds
            set(value) {
                Native.configSetMilliseconds(ConfigKey.SIMPLEBLUEZ_CONNECTION_TIMEOUT, value.toTimeoutMilliseconds())
            }

        public var disconnectionTimeout: Duration
            get() = Native.configGetMilliseconds(ConfigKey.SIMPLEBLUEZ_DISCONNECTION_TIMEOUT).milliseconds
            set(value) {
                Native.configSetMilliseconds(ConfigKey.SIMPLEBLUEZ_DISCONNECTION_TIMEOUT, value.toTimeoutMilliseconds())
            }

        @JvmStatic
        @Throws(SimpleKtBleException::class)
        public fun reset() {
            Native.configReset(ConfigGroup.SIMPLEBLUEZ)
        }
    }

    public object WinRT {
        public var experimentalReinitializeWinrtApartmentOnMainThread: Boolean
            get() = Native.configGetBoolean(ConfigKey.WINRT_REINITIALIZE_APARTMENT_ON_MAIN_THREAD)
            set(value) = Native.configSetBoolean(ConfigKey.WINRT_REINITIALIZE_APARTMENT_ON_MAIN_THREAD, value)

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

    public object Dongl {
        public var useDonglBackend: Boolean
            get() = Native.configGetBoolean(ConfigKey.DONGL_USE_DONGL_BACKEND)
            set(value) = Native.configSetBoolean(ConfigKey.DONGL_USE_DONGL_BACKEND, value)

        public var autoUpdate: Boolean
            get() = Native.configGetBoolean(ConfigKey.DONGL_AUTO_UPDATE)
            set(value) = Native.configSetBoolean(ConfigKey.DONGL_AUTO_UPDATE, value)

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
}

/** The groups Native resets. The native side uses the same numbers. */
internal object ConfigGroup {
    const val ALL = 0
    const val SIMPLEBLUEZ = 1
    const val WINRT = 2
    const val COREBLUETOOTH = 3
    const val DONGL = 4
}

// SimpleBLE keeps timeouts in nanoseconds, so longer ones do not fit.
private val maxTimeout = Long.MAX_VALUE.nanoseconds

private fun Duration.toTimeoutMilliseconds(): Long {
    require(this in Duration.ZERO..maxTimeout) { "Timeouts must be between zero and $maxTimeout" }
    return inWholeMilliseconds
}
