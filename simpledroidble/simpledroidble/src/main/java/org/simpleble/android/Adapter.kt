package org.simpleble.android

import android.util.Log
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class Adapter private constructor(newInstanceId: Long) {
    private val callbackScope = CoroutineScope(SupervisorJob() + Dispatchers.Main.immediate)
    private val _onScanStart = MutableSharedFlow<Unit>(extraBufferCapacity = 1, onBufferOverflow = BufferOverflow.DROP_OLDEST)
    private val _onScanStop = MutableSharedFlow<Unit>(extraBufferCapacity = 1, onBufferOverflow = BufferOverflow.DROP_OLDEST)
    private val _onScanActive = MutableSharedFlow<Boolean>(replay = 1, extraBufferCapacity = 1, onBufferOverflow = BufferOverflow.DROP_OLDEST)
    private val _onScanUpdated = MutableSharedFlow<Peripheral>(extraBufferCapacity = 64, onBufferOverflow = BufferOverflow.DROP_OLDEST)
    private val _onScanFound = MutableSharedFlow<Peripheral>(extraBufferCapacity = 64, onBufferOverflow = BufferOverflow.DROP_OLDEST)

    private var instanceId: Long = newInstanceId

    private val callbacks = object : Callback {
        override fun onScanStart() {
            callbackScope.launch {
                _onScanStart.emit(Unit)
                _onScanActive.emit(true)
            }
        }

        override fun onScanStop() {
            callbackScope.launch {
                _onScanStop.emit(Unit)
                _onScanActive.emit(false)
            }
        }

        override fun onScanUpdated(peripheralId: Long) {
            callbackScope.launch {
                _onScanUpdated.emit(Peripheral(instanceId, peripheralId))
            }
        }

        override fun onScanFound(peripheralId: Long) {
            callbackScope.launch {
                _onScanFound.emit(Peripheral(instanceId, peripheralId))
            }
        }
    }

    init {
        Log.d("SimpleBLE", "Adapter ${this.hashCode()}.init")
        nativeAdapterRegister(instanceId, callbacks)
    }

    val identifier: String get() {
        return nativeAdapterIdentifier(instanceId) ?: ""
    }

    val address: BluetoothAddress get() {
        return BluetoothAddress(nativeAdapterAddress(instanceId) ?: "")
    }

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
        return nativeAdapterScanGetResults(instanceId).map { Peripheral(instanceId, it) }
    }

    val onScanStart: SharedFlow<Unit> get() = _onScanStart

    val onScanStop: SharedFlow<Unit> get() = _onScanStop

    val onScanActive: SharedFlow<Boolean> get() = _onScanActive

    val onScanUpdated: SharedFlow<Peripheral> get() = _onScanUpdated

    val onScanFound: SharedFlow<Peripheral> get() = _onScanFound

    fun getPairedPeripherals(): List<Peripheral> {
        return nativeAdapterGetPairedPeripherals(instanceId).map { Peripheral(instanceId, it) }
    }

    companion object {

        @JvmStatic
        fun isBluetoothEnabled(): Boolean {
            return nativeIsBluetoothEnabled()
        }

        @JvmStatic
        fun getAdapters(): List<Adapter> {
            val nativeAdapterIds = nativeGetAdapters()
            val adapters = ArrayList<Adapter>()

            for (nativeAdapterId in nativeAdapterIds) {
                adapters.add(Adapter(nativeAdapterId))
            }

            return adapters
        }

        @JvmStatic
        private external fun nativeGetAdapters(): LongArray

        private external fun nativeIsBluetoothEnabled(): Boolean
    }

    // ----------------------------------------------------------------------------

    private external fun nativeAdapterRegister(adapterId: Long, callback: Callback)

    private external fun nativeAdapterIdentifier(adapterId: Long): String?

    private external fun nativeAdapterAddress(adapterId: Long): String?

    private external fun nativeAdapterScanStart(adapterId: Long)

    private external fun nativeAdapterScanStop(adapterId: Long)

    private external fun nativeAdapterScanFor(adapterId: Long, timeout: Int)

    private external fun nativeAdapterScanIsActive(adapterId: Long): Boolean

    private external fun nativeAdapterScanGetResults(adapterId: Long) : LongArray

    private external fun nativeAdapterGetPairedPeripherals(adapterId: Long): LongArray

    // ----------------------------------------------------------------------------

    private interface Callback {
        fun onScanStart()
        fun onScanStop()
        fun onScanUpdated(peripheralId: Long)
        fun onScanFound(peripheralId: Long)
    }

}
