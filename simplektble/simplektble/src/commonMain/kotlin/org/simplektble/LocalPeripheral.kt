package org.simplektble

import kotlinx.atomicfu.locks.SynchronizedObject
import kotlinx.atomicfu.locks.synchronized
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.IO
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.withContext

public class LocalPeripheral internal constructor(private val instanceId: Long) {
    private val servicesLock = SynchronizedObject()
    private val servicesById = mutableMapOf<Long, LocalService>()
    private val _onClientConnected = eventFlow<BluetoothAddress>()
    private val _onClientDisconnected = eventFlow<BluetoothAddress>()

    private val callbacks = object : LocalPeripheralCallback {
        override fun onClientConnected(address: String) {
            _onClientConnected.tryEmit(BluetoothAddress(address))
        }

        override fun onClientDisconnected(address: String) {
            _onClientDisconnected.tryEmit(BluetoothAddress(address))
        }
    }

    init {
        Native.localPeripheralRegister(instanceId, callbacks)
    }

    private fun service(serviceId: Long): LocalService {
        return synchronized(servicesLock) { servicesById.getOrPut(serviceId) { LocalService(serviceId) } }
    }

    @Throws(SimpleKtBleException::class)
    public fun addAdvertisedService(serviceUuid: BluetoothUUID) {
        addAdvertisedService(listOf(serviceUuid))
    }

    @Throws(SimpleKtBleException::class)
    public fun addAdvertisedService(serviceUuids: List<BluetoothUUID>) {
        Native.localPeripheralAddAdvertisedService(instanceId, serviceUuids.map(BluetoothUUID::value).toTypedArray())
    }

    /** Adds manufacturer specific data to the advertisement. Setting data for the same company identifier again replaces it. */
    @Throws(SimpleKtBleException::class, IllegalArgumentException::class)
    public fun setAdvertisedManufacturerData(companyId: Int, data: ByteArray) {
        require(companyId in 0..0xFFFF) { "companyId must be between 0 and 0xFFFF" }
        Native.localPeripheralSetAdvertisedManufacturerData(instanceId, companyId, data)
    }

    /** Chooses whether centrals can connect. Local peripherals are connectable by default. */
    @Throws(SimpleKtBleException::class)
    public fun setConnectable(connectable: Boolean) {
        Native.localPeripheralSetConnectable(instanceId, connectable)
    }

    /**
     * Sets the local name in the advertisement on iOS, macOS and Linux, or removes it with null. This changes only the
     * advertisement, not the system's Bluetooth name, and iOS leaves it out while the app is in the background. Other
     * platforms throw [ErrorCode.OPERATION_NOT_SUPPORTED].
     */
    @Throws(SimpleKtBleException::class)
    public fun setAdvertisementLocalName(name: String?) {
        Native.localPeripheralSetAdvertisementLocalName(instanceId, name)
    }

    @Throws(SimpleKtBleException::class)
    public fun services(): List<LocalService> {
        return Native.localPeripheralServices(instanceId).map(::service)
    }

    public val isStarted: Boolean
        get() = Native.localPeripheralIsStarted(instanceId)

    /** Asks the Bluetooth stack whether this peripheral is advertising. */
    @Throws(SimpleKtBleException::class, CancellationException::class)
    public suspend fun isAdvertising(): Boolean {
        return withContext(Dispatchers.IO) { Native.localPeripheralIsAdvertising(instanceId) }
    }

    public val onClientConnected: SharedFlow<BluetoothAddress>
        get() = _onClientConnected

    public val onClientDisconnected: SharedFlow<BluetoothAddress>
        get() = _onClientDisconnected

    @Throws(SimpleKtBleException::class)
    public fun addService(uuid: BluetoothUUID): LocalService {
        return service(Native.localPeripheralAddService(instanceId, uuid.value))
    }

    @Throws(SimpleKtBleException::class)
    public fun removeAllServices() {
        Native.localPeripheralRemoveAllServices(instanceId)
        synchronized(servicesLock) { servicesById.clear() }
    }

    @Throws(SimpleKtBleException::class, CancellationException::class)
    public suspend fun start() {
        withContext(Dispatchers.IO) { Native.localPeripheralStart(instanceId) }
    }

    /** Stops advertising and hosting. It finishes even when the calling coroutine is cancelled. */
    @Throws(SimpleKtBleException::class, CancellationException::class)
    public suspend fun stop() {
        runToEnd { Native.localPeripheralStop(instanceId) }
    }
}
