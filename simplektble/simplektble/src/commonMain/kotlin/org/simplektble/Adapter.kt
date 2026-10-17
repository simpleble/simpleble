package org.simplektble

import kotlinx.atomicfu.locks.SynchronizedObject
import kotlinx.atomicfu.locks.synchronized
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.IO
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.withContext
import kotlin.jvm.JvmStatic

public class Adapter private constructor(private val instanceId: Long) {
    private val _onScanStart = MutableSharedFlow<Unit>(extraBufferCapacity = 1, onBufferOverflow = BufferOverflow.DROP_OLDEST)
    private val _onScanStop = MutableSharedFlow<Unit>(extraBufferCapacity = 1, onBufferOverflow = BufferOverflow.DROP_OLDEST)
    private val _onScanActive = MutableSharedFlow<Boolean>(replay = 1, extraBufferCapacity = 1, onBufferOverflow = BufferOverflow.DROP_OLDEST)
    private val _onScanUpdated = MutableSharedFlow<Peripheral>(extraBufferCapacity = 64, onBufferOverflow = BufferOverflow.DROP_OLDEST)
    private val _onScanFound = MutableSharedFlow<Peripheral>(extraBufferCapacity = 64, onBufferOverflow = BufferOverflow.DROP_OLDEST)

    private val peripheralsLock = SynchronizedObject()
    private val peripherals = mutableMapOf<Long, Peripheral>()

    private fun peripheral(peripheralId: Long): Peripheral {
        return synchronized(peripheralsLock) {
            peripherals.getOrPut(peripheralId) { Peripheral(instanceId, peripheralId) }
        }
    }

    private val callbacks = object : AdapterCallback {
        override fun onScanStart() {
            _onScanStart.tryEmit(Unit)
            _onScanActive.tryEmit(true)
        }

        override fun onScanStop() {
            _onScanStop.tryEmit(Unit)
            _onScanActive.tryEmit(false)
        }

        override fun onScanUpdated(peripheralId: Long) {
            _onScanUpdated.tryEmit(peripheral(peripheralId))
        }

        override fun onScanFound(peripheralId: Long) {
            _onScanFound.tryEmit(peripheral(peripheralId))
        }
    }

    init { Native.adapterRegister(instanceId, callbacks) }

    public val identifier: String get() = Native.adapterIdentifier(instanceId)

    public val address: BluetoothAddress get() = BluetoothAddress(Native.adapterAddress(instanceId))

    public fun scanStart() {
        Native.adapterScanStart(instanceId)
    }

    public fun scanStop() {
        Native.adapterScanStop(instanceId)
    }

    public suspend fun scanFor(timeoutMs: Int) {
        withContext(Dispatchers.IO) {
            Native.adapterScanFor(instanceId, timeoutMs)
        }
    }

    public val scanIsActive: Boolean get() {
        return Native.adapterScanIsActive(instanceId)
    }

    public fun scanGetResults(): List<Peripheral> {
        return Native.adapterScanGetResults(instanceId).map(::peripheral)
    }

    public val onScanStart: SharedFlow<Unit> get() = _onScanStart

    public val onScanStop: SharedFlow<Unit> get() = _onScanStop

    public val onScanActive: SharedFlow<Boolean> get() = _onScanActive

    public val onScanUpdated: SharedFlow<Peripheral> get() = _onScanUpdated

    public val onScanFound: SharedFlow<Peripheral> get() = _onScanFound

    public fun getPairedPeripherals(): List<Peripheral> {
        return Native.adapterGetPairedPeripherals(instanceId).map(::peripheral)
    }

    public companion object {
        private val adapterCacheLock = SynchronizedObject()
        private val adapterCache = mutableMapOf<Long, Adapter>()

        @JvmStatic
        public fun isBluetoothEnabled(): Boolean {
            return Native.isBluetoothEnabled()
        }

        @JvmStatic
        public fun getAdapters(): List<Adapter> {
            return Native.getAdapters().map { adapterId ->
                synchronized(adapterCacheLock) {
                    adapterCache.getOrPut(adapterId) { Adapter(adapterId) }
                }
            }
        }
    }
}
