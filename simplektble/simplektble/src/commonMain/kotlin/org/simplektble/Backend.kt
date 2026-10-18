package org.simplektble

import kotlinx.atomicfu.locks.SynchronizedObject
import kotlinx.atomicfu.locks.synchronized
import kotlin.jvm.JvmStatic

/**
 * A source of adapters: the operating system's Bluetooth stack, a Dongl, or the simulator.
 *
 * SimpleBLE has one backend per identifier, and each is represented by one Backend object.
 */
public class Backend private constructor(public val identifier: String) {
    public fun isBluetoothEnabled(): Boolean {
        return Native.backendIsBluetoothEnabled(identifier)
    }

    /** The adapters of this backend, the same objects [Adapter.getAdapters] returns. */
    public fun getAdapters(): List<Adapter> {
        return Native.backendGetAdapters(identifier).map(Adapter::forId)
    }

    public companion object {
        private val backendCacheLock = SynchronizedObject()
        private val backendCache = mutableMapOf<String, Backend>()

        private fun backend(identifier: String): Backend {
            return synchronized(backendCacheLock) { backendCache.getOrPut(identifier) { Backend(identifier) } }
        }

        @JvmStatic
        public fun getBackends(): List<Backend> {
            return Native.getBackends().map(::backend)
        }

        /** The operating system's Bluetooth stack. */
        @JvmStatic
        public fun getNative(): Backend {
            return backend(Native.getNativeBackend())
        }

        @JvmStatic
        public fun getDongl(): Backend {
            return backend(Native.getDonglBackend())
        }

        @JvmStatic
        public fun getSimulator(): Backend {
            return backend(Native.getSimulatorBackend())
        }
    }
}
