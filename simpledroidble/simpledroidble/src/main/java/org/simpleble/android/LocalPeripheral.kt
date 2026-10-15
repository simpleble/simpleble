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

    fun addAdvertisedService(serviceUuid: BluetoothUUID) {
        addAdvertisedService(listOf(serviceUuid))
    }

    fun addAdvertisedService(serviceUuids: List<BluetoothUUID>) {
        nativeAddAdvertisedService(
            instanceId,
            serviceUuids.map(BluetoothUUID::value).toTypedArray()
        )
    }

    /** Adds manufacturer specific data to the advertisement. Setting data for the same company identifier again replaces it. */
    fun setAdvertisedManufacturerData(companyId: Int, data: ByteArray) {
        require(companyId in 0..0xFFFF) { "companyId must be between 0 and 0xFFFF" }
        nativeSetAdvertisedManufacturerData(instanceId, companyId, data)
    }

    /** Chooses whether centrals can connect. Local peripherals are connectable by default. */
    fun setConnectable(connectable: Boolean) {
        nativeSetConnectable(instanceId, connectable)
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
    private external fun nativeAddAdvertisedService(peripheralId: Long, serviceUuids: Array<String>)
    private external fun nativeSetAdvertisedManufacturerData(peripheralId: Long, companyId: Int, data: ByteArray)
    private external fun nativeSetConnectable(peripheralId: Long, connectable: Boolean)
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
