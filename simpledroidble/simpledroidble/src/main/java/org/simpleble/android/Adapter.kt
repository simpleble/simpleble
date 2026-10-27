package org.simpleble.android

import android.content.Context
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow
import java.util.concurrent.ConcurrentHashMap
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

class Adapter private constructor(private val instanceId: Long) {
    private val _onScanStart = MutableSharedFlow<Unit>(extraBufferCapacity = 1, onBufferOverflow = BufferOverflow.DROP_OLDEST)
    private val _onScanStop = MutableSharedFlow<Unit>(extraBufferCapacity = 1, onBufferOverflow = BufferOverflow.DROP_OLDEST)
    private val _onScanActive = MutableSharedFlow<Boolean>(replay = 1, extraBufferCapacity = 1, onBufferOverflow = BufferOverflow.DROP_OLDEST)
    private val _onScanUpdated = MutableSharedFlow<Peripheral>(extraBufferCapacity = 64, onBufferOverflow = BufferOverflow.DROP_OLDEST)
    private val _onScanFound = MutableSharedFlow<Peripheral>(extraBufferCapacity = 64, onBufferOverflow = BufferOverflow.DROP_OLDEST)

    private val peripherals = ConcurrentHashMap<Long, Peripheral>()

    private fun peripheral(peripheralId: Long): Peripheral {
        return peripherals.computeIfAbsent(peripheralId) { Peripheral(instanceId, it) }
    }

    private val callbacks = object : Callback {
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

    init { nativeAdapterRegister(instanceId, callbacks) }

    val identifier: String get() = nativeAdapterIdentifier(instanceId)

    val address: BluetoothAddress get() = BluetoothAddress(nativeAdapterAddress(instanceId))

    fun scanStart() {
        nativeAdapterScanStart(instanceId)
    }

    fun scanStop() {
        nativeAdapterScanStop(instanceId)
    }

    suspend fun scanFor(timeoutMs: Int) {
        withContext(Dispatchers.IO) {
            nativeAdapterScanFor(instanceId, timeoutMs)
        }
    }

    val scanIsActive: Boolean get() {
        return nativeAdapterScanIsActive(instanceId)
    }

    fun scanGetResults(): List<Peripheral> {
        return nativeAdapterScanGetResults(instanceId).map(::peripheral)
    }

    val onScanStart: SharedFlow<Unit> get() = _onScanStart

    val onScanStop: SharedFlow<Unit> get() = _onScanStop

    val onScanActive: SharedFlow<Boolean> get() = _onScanActive

    val onScanUpdated: SharedFlow<Peripheral> get() = _onScanUpdated

    val onScanFound: SharedFlow<Peripheral> get() = _onScanFound

    fun getPairedPeripherals(): List<Peripheral> {
        return nativeAdapterGetPairedPeripherals(instanceId).map(::peripheral)
    }

    /**
     * Create a BLE peripheral hosted by this Android application.
     *
     * The application context is retained by the native backend; the supplied
     * Activity is not retained. Configure services and advertising data on the
     * returned object before starting it.
     */
    fun createLocalPeripheral(context: Context): LocalPeripheral {
        return LocalPeripheral(nativeAdapterCreateLocalPeripheral(instanceId, context.applicationContext))
    }

    companion object {
        init {
            SimpleDroidBle.ensureLoaded()
        }

        @JvmStatic
        fun isBluetoothEnabled(): Boolean {
            return nativeIsBluetoothEnabled()
        }

        @JvmStatic
        fun getAdapters(): List<Adapter> {
            return nativeGetAdapters().map { adapterId ->
                adapterCache.computeIfAbsent(adapterId, ::Adapter)
            }
        }

        private val adapterCache = ConcurrentHashMap<Long, Adapter>()

        @JvmStatic
        private external fun nativeGetAdapters(): LongArray

        private external fun nativeIsBluetoothEnabled(): Boolean
    }

    // ----------------------------------------------------------------------------

    private external fun nativeAdapterRegister(adapterId: Long, callback: Callback)

    private external fun nativeAdapterIdentifier(adapterId: Long): String

    private external fun nativeAdapterAddress(adapterId: Long): String

    private external fun nativeAdapterScanStart(adapterId: Long)

    private external fun nativeAdapterScanStop(adapterId: Long)

    private external fun nativeAdapterScanFor(adapterId: Long, timeout: Int)

    private external fun nativeAdapterScanIsActive(adapterId: Long): Boolean

    private external fun nativeAdapterScanGetResults(adapterId: Long) : LongArray

    private external fun nativeAdapterGetPairedPeripherals(adapterId: Long): LongArray

    private external fun nativeAdapterCreateLocalPeripheral(adapterId: Long, applicationContext: Context): Long

    // ----------------------------------------------------------------------------

    private interface Callback {
        fun onScanStart()
        fun onScanStop()
        fun onScanUpdated(peripheralId: Long)
        fun onScanFound(peripheralId: Long)
    }

}
