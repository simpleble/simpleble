package org.simpleble.android

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.withContext

class LocalPeripheral internal constructor(private val instanceId: Long) {
    private val configuredServices = mutableListOf<LocalService>()
    private val _onClientConnected = MutableSharedFlow<BluetoothAddress>(
        extraBufferCapacity = 16,
        onBufferOverflow = BufferOverflow.DROP_OLDEST
    )
    private val _onClientDisconnected = MutableSharedFlow<BluetoothAddress>(
        extraBufferCapacity = 16,
        onBufferOverflow = BufferOverflow.DROP_OLDEST
    )

    private val callbacks = object : Callback {
        override fun onClientConnected(address: String) {
            _onClientConnected.tryEmit(BluetoothAddress(address))
        }

        override fun onClientDisconnected(address: String) {
            _onClientDisconnected.tryEmit(BluetoothAddress(address))
        }
    }

    init {
        nativeRegister(instanceId, callbacks)
    }

    var advertisement: LocalAdvertisement = LocalAdvertisement()
        set(value) {
            val snapshot = value.copy(serviceUuids = value.serviceUuids.toList())
            nativeSetAdvertisement(
                instanceId,
                snapshot.localName,
                snapshot.serviceUuids.map(BluetoothUUID::value).toTypedArray()
            )
            field = snapshot
        }

    val services: List<LocalService>
        get() = synchronized(configuredServices) { configuredServices.toList() }

    val isStarted: Boolean
        get() = nativeIsStarted(instanceId)

    val isAdvertising: Boolean
        get() = nativeIsAdvertising(instanceId)

    val onClientConnected: SharedFlow<BluetoothAddress>
        get() = _onClientConnected

    val onClientDisconnected: SharedFlow<BluetoothAddress>
        get() = _onClientDisconnected

    fun addService(uuid: BluetoothUUID): LocalService {
        val service = LocalService(nativeAddService(instanceId, uuid.value))
        synchronized(configuredServices) { configuredServices += service }
        return service
    }

    fun removeAllServices() {
        nativeRemoveAllServices(instanceId)
        synchronized(configuredServices) { configuredServices.clear() }
    }

    suspend fun start() {
        withContext(Dispatchers.IO) { nativeStart(instanceId) }
    }

    suspend fun stop() {
        withContext(Dispatchers.IO) { nativeStop(instanceId) }
    }

    private external fun nativeRegister(peripheralId: Long, callback: Callback)
    private external fun nativeSetAdvertisement(peripheralId: Long, localName: String?, serviceUuids: Array<String>)
    private external fun nativeAddService(peripheralId: Long, uuid: String): Long
    private external fun nativeRemoveAllServices(peripheralId: Long)
    private external fun nativeStart(peripheralId: Long)
    private external fun nativeStop(peripheralId: Long)
    private external fun nativeIsStarted(peripheralId: Long): Boolean
    private external fun nativeIsAdvertising(peripheralId: Long): Boolean

    private interface Callback {
        fun onClientConnected(address: String)
        fun onClientDisconnected(address: String)
    }
}
