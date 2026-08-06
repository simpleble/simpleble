package org.simpleble.android

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.flow.callbackFlow
import kotlinx.coroutines.withContext

class Peripheral internal constructor(
    private val adapterId: Long,
    private val instanceId: Long
) {

    private val _onConnected = MutableSharedFlow<Unit>(extraBufferCapacity = 1)
    private val _onConnectionActive = MutableSharedFlow<Boolean>(replay = 1, extraBufferCapacity = 1)
    private val _onDisconnected = MutableSharedFlow<Unit>(extraBufferCapacity = 1)

    private val callbacks = object : Callback {
        override fun onConnected() {
            _onConnected.tryEmit(Unit)
            _onConnectionActive.tryEmit(true)
        }

        override fun onDisconnected() {
            _onDisconnected.tryEmit(Unit)
            _onConnectionActive.tryEmit(false)
        }
    }

    init { nativePeripheralRegister(adapterId, instanceId, callbacks) }

    val identifier: String get() = nativePeripheralIdentifier(adapterId, instanceId)

    val address: BluetoothAddress get() = BluetoothAddress(nativePeripheralAddress(adapterId, instanceId))

    val addressType: BluetoothAddressType get() {
        return BluetoothAddressType.fromInt(nativePeripheralAddressType(adapterId, instanceId))
    }

    val rssi: Int get() {
        return nativePeripheralRssi(adapterId, instanceId)
    }

    val txPower: Int get() {
        return nativePeripheralTxPower(adapterId, instanceId)
    }

    val mtu: Int get() {
        return nativePeripheralMtu(adapterId, instanceId)
    }

    suspend fun connect() {
        withContext(Dispatchers.IO) {
            nativePeripheralConnect(adapterId, instanceId)
        }
    }

    suspend fun disconnect() {
        withContext(Dispatchers.IO) {
            nativePeripheralDisconnect(adapterId, instanceId)
        }
    }

    val isConnected: Boolean get() {
        return nativePeripheralIsConnected(adapterId, instanceId)
    }

    val isConnectable: Boolean get() {
        return nativePeripheralIsConnectable(adapterId, instanceId)
    }

    val isPaired: Boolean get() {
        return nativePeripheralIsPaired(adapterId, instanceId)
    }

    fun unpair() {
        nativePeripheralUnpair(adapterId, instanceId)
    }

    fun services(): List<Service> {
        return nativePeripheralServices(adapterId, instanceId)
    }

    fun manufacturerData(): Map<Int, ByteArray> {
        return nativePeripheralManufacturerData(adapterId, instanceId)
    }

    suspend fun read(service: BluetoothUUID, characteristic: BluetoothUUID): ByteArray {
        return withContext(Dispatchers.IO) {
            nativePeripheralRead(adapterId, instanceId, service.toString(), characteristic.toString())
        }
    }

    suspend fun writeRequest(service: BluetoothUUID, characteristic: BluetoothUUID, data: ByteArray) {
        withContext(Dispatchers.IO) {
            nativePeripheralWriteRequest(adapterId, instanceId, service.toString(), characteristic.toString(), data)
        }
    }

    suspend fun writeCommand(service: BluetoothUUID, characteristic: BluetoothUUID, data: ByteArray) {
        withContext(Dispatchers.IO) {
            nativePeripheralWriteCommand(adapterId, instanceId, service.toString(), characteristic.toString(), data)
        }
    }

    fun notify(
        service: BluetoothUUID,
        characteristic: BluetoothUUID
    ): Flow<ByteArray> = callbackFlow {
        val dataCallback = object : DataCallback {
            override fun onDataReceived(data: ByteArray) {
                val result = trySend(data)
                if (result.isFailure && !result.isClosed) {
                    close(SimpleDroidBleException("Notification buffer overflow for $service/$characteristic"))
                }
            }
        }

        var subscribed = false
        try {
            withContext(NonCancellable + Dispatchers.IO) {
                nativePeripheralNotify(adapterId, instanceId, service.toString(), characteristic.toString(), dataCallback)
                subscribed = true
            }
            awaitClose {}
        } finally {
            if (subscribed) {
                withContext(NonCancellable) {
                    unsubscribe(service, characteristic)
                }
            }
        }
    }

    fun indicate(
        service: BluetoothUUID,
        characteristic: BluetoothUUID
    ): Flow<ByteArray> = callbackFlow {
        val dataCallback = object : DataCallback {
            override fun onDataReceived(data: ByteArray) {
                val result = trySend(data)
                if (result.isFailure && !result.isClosed) {
                    close(SimpleDroidBleException("Indication buffer overflow for $service/$characteristic"))
                }
            }
        }

        var subscribed = false
        try {
            withContext(NonCancellable + Dispatchers.IO) {
                nativePeripheralIndicate(adapterId, instanceId, service.toString(), characteristic.toString(), dataCallback)
                subscribed = true
            }
            awaitClose {}
        } finally {
            if (subscribed) {
                withContext(NonCancellable) {
                    unsubscribe(service, characteristic)
                }
            }
        }
    }

    suspend fun unsubscribe(service: BluetoothUUID, characteristic: BluetoothUUID) {
        withContext(Dispatchers.IO) {
            nativePeripheralUnsubscribe(adapterId, instanceId, service.toString(), characteristic.toString())
        }
    }

    suspend fun read(
        service: BluetoothUUID,
        characteristic: BluetoothUUID,
        descriptor: BluetoothUUID
    ): ByteArray {
        return withContext(Dispatchers.IO) {
            nativePeripheralDescriptorRead(
                adapterId,
                instanceId,
                service.toString(),
                characteristic.toString(),
                descriptor.toString()
            )
        }
    }

    suspend fun write(
        service: BluetoothUUID,
        characteristic: BluetoothUUID,
        descriptor: BluetoothUUID,
        data: ByteArray
    ) {
        withContext(Dispatchers.IO) {
            nativePeripheralDescriptorWrite(
                adapterId,
                instanceId,
                service.toString(),
                characteristic.toString(),
                descriptor.toString(),
                data
            )
        }
    }

    val onConnected: SharedFlow<Unit> get() = _onConnected

    val onDisconnected: SharedFlow<Unit> get() = _onDisconnected

    val onConnectionActive: SharedFlow<Boolean> get() = _onConnectionActive

    /// ----------------------------------------------------------------------------

    private external fun nativePeripheralRegister(adapterId: Long, instanceId: Long, callback: Callback)

    private external fun nativePeripheralIdentifier(adapterId: Long, instanceId: Long): String

    private external fun nativePeripheralAddress(adapterId: Long, instanceId: Long): String

    private external fun nativePeripheralAddressType(adapterId: Long, instanceId: Long): Int

    private external fun nativePeripheralRssi(adapterId: Long, instanceId: Long): Int

    private external fun nativePeripheralTxPower(adapterId: Long, instanceId: Long): Int

    private external fun nativePeripheralMtu(adapterId: Long, instanceId: Long): Int

    private external fun nativePeripheralConnect(adapterId: Long, instanceId: Long)

    private external fun nativePeripheralDisconnect(adapterId: Long, instanceId: Long)

    private external fun nativePeripheralIsConnected(adapterId: Long, instanceId: Long): Boolean

    private external fun nativePeripheralIsConnectable(adapterId: Long, instanceId: Long): Boolean

    private external fun nativePeripheralIsPaired(adapterId: Long, instanceId: Long): Boolean

    private external fun nativePeripheralUnpair(adapterId: Long, instanceId: Long)

    private external fun nativePeripheralServices(adapterId: Long, instanceId: Long): List<Service>

    private external fun nativePeripheralManufacturerData(adapterId: Long, instanceId: Long): Map<Int, ByteArray>

    private external fun nativePeripheralRead(
        adapterId: Long,
        instanceId: Long,
        service: String,
        characteristic: String
    ): ByteArray

    private external fun nativePeripheralWriteRequest(
        adapterId: Long,
        instanceId: Long,
        service: String,
        characteristic: String,
        data: ByteArray
    )

    private external fun nativePeripheralWriteCommand(
        adapterId: Long,
        instanceId: Long,
        service: String,
        characteristic: String,
        data: ByteArray
    )

    private external fun nativePeripheralNotify(
        adapterId: Long,
        instanceId: Long,
        service: String,
        characteristic: String,
        dataCallback: DataCallback
    )

    private external fun nativePeripheralIndicate(
        adapterId: Long,
        instanceId: Long,
        service: String,
        characteristic: String,
        dataCallback: DataCallback
    )

    private external fun nativePeripheralUnsubscribe(
        adapterId: Long,
        instanceId: Long,
        service: String,
        characteristic: String)

    private external fun nativePeripheralDescriptorRead(
        adapterId: Long,
        instanceId: Long,
        service: String,
        characteristic: String,
        descriptor: String
    ): ByteArray

    private external fun nativePeripheralDescriptorWrite(
        adapterId: Long,
        instanceId: Long,
        service: String,
        characteristic: String,
        descriptor: String,
        data: ByteArray
    )

    // ----------------------------------------------------------------------------

    private interface DataCallback {
        fun onDataReceived(data: ByteArray)
    }


    private interface Callback {
        fun onConnected()
        fun onDisconnected()
    }

}
