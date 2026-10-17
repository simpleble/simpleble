package org.simplektble

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.IO
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.flow.callbackFlow
import kotlinx.coroutines.withContext

public class Peripheral internal constructor(
    private val adapterId: Long,
    private val instanceId: Long
) {

    private val _onConnected = MutableSharedFlow<Unit>(extraBufferCapacity = 1)
    private val _onConnectionActive = MutableSharedFlow<Boolean>(replay = 1, extraBufferCapacity = 1)
    private val _onDisconnected = MutableSharedFlow<Unit>(extraBufferCapacity = 1)

    private val callbacks = object : PeripheralCallback {
        override fun onConnected() {
            _onConnected.tryEmit(Unit)
            _onConnectionActive.tryEmit(true)
        }

        override fun onDisconnected() {
            _onDisconnected.tryEmit(Unit)
            _onConnectionActive.tryEmit(false)
        }
    }

    init { Native.peripheralRegister(adapterId, instanceId, callbacks) }

    public val identifier: String get() = Native.peripheralIdentifier(adapterId, instanceId)

    public val address: BluetoothAddress get() = BluetoothAddress(Native.peripheralAddress(adapterId, instanceId))

    public val addressType: BluetoothAddressType get() {
        return BluetoothAddressType.fromInt(Native.peripheralAddressType(adapterId, instanceId))
    }

    public val rssi: Int get() {
        return Native.peripheralRssi(adapterId, instanceId)
    }

    public val txPower: Int get() {
        return Native.peripheralTxPower(adapterId, instanceId)
    }

    public val mtu: Int get() {
        return Native.peripheralMtu(adapterId, instanceId)
    }

    public suspend fun connect() {
        withContext(Dispatchers.IO) {
            Native.peripheralConnect(adapterId, instanceId)
        }
    }

    public suspend fun disconnect() {
        withContext(Dispatchers.IO) {
            Native.peripheralDisconnect(adapterId, instanceId)
        }
    }

    public val isConnected: Boolean get() {
        return Native.peripheralIsConnected(adapterId, instanceId)
    }

    public val isConnectable: Boolean get() {
        return Native.peripheralIsConnectable(adapterId, instanceId)
    }

    public val isPaired: Boolean get() {
        return Native.peripheralIsPaired(adapterId, instanceId)
    }

    public fun unpair() {
        Native.peripheralUnpair(adapterId, instanceId)
    }

    public fun services(): List<Service> {
        return Native.peripheralServices(adapterId, instanceId)
    }

    public fun manufacturerData(): Map<Int, ByteArray> {
        return Native.peripheralManufacturerData(adapterId, instanceId)
    }

    public suspend fun read(service: BluetoothUUID, characteristic: BluetoothUUID): ByteArray {
        return withContext(Dispatchers.IO) {
            Native.peripheralRead(adapterId, instanceId, service.toString(), characteristic.toString())
        }
    }

    public suspend fun writeRequest(service: BluetoothUUID, characteristic: BluetoothUUID, data: ByteArray) {
        withContext(Dispatchers.IO) {
            Native.peripheralWriteRequest(adapterId, instanceId, service.toString(), characteristic.toString(), data)
        }
    }

    public suspend fun writeCommand(service: BluetoothUUID, characteristic: BluetoothUUID, data: ByteArray) {
        withContext(Dispatchers.IO) {
            Native.peripheralWriteCommand(adapterId, instanceId, service.toString(), characteristic.toString(), data)
        }
    }

    public fun notify(
        service: BluetoothUUID,
        characteristic: BluetoothUUID
    ): Flow<ByteArray> = callbackFlow {
        val dataCallback = object : DataCallback {
            override fun onDataReceived(data: ByteArray) {
                val result = trySend(data)
                if (result.isFailure && !result.isClosed) {
                    close(SimpleKtBleException("Notification buffer overflow for $service/$characteristic"))
                }
            }
        }

        var subscribed = false
        try {
            withContext(NonCancellable + Dispatchers.IO) {
                Native.peripheralNotify(adapterId, instanceId, service.toString(), characteristic.toString(), dataCallback)
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

    public fun indicate(
        service: BluetoothUUID,
        characteristic: BluetoothUUID
    ): Flow<ByteArray> = callbackFlow {
        val dataCallback = object : DataCallback {
            override fun onDataReceived(data: ByteArray) {
                val result = trySend(data)
                if (result.isFailure && !result.isClosed) {
                    close(SimpleKtBleException("Indication buffer overflow for $service/$characteristic"))
                }
            }
        }

        var subscribed = false
        try {
            withContext(NonCancellable + Dispatchers.IO) {
                Native.peripheralIndicate(adapterId, instanceId, service.toString(), characteristic.toString(), dataCallback)
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

    public suspend fun unsubscribe(service: BluetoothUUID, characteristic: BluetoothUUID) {
        withContext(Dispatchers.IO) {
            Native.peripheralUnsubscribe(adapterId, instanceId, service.toString(), characteristic.toString())
        }
    }

    public suspend fun read(
        service: BluetoothUUID,
        characteristic: BluetoothUUID,
        descriptor: BluetoothUUID
    ): ByteArray {
        return withContext(Dispatchers.IO) {
            Native.peripheralDescriptorRead(
                adapterId,
                instanceId,
                service.toString(),
                characteristic.toString(),
                descriptor.toString()
            )
        }
    }

    public suspend fun write(
        service: BluetoothUUID,
        characteristic: BluetoothUUID,
        descriptor: BluetoothUUID,
        data: ByteArray
    ) {
        withContext(Dispatchers.IO) {
            Native.peripheralDescriptorWrite(
                adapterId,
                instanceId,
                service.toString(),
                characteristic.toString(),
                descriptor.toString(),
                data
            )
        }
    }

    public val onConnected: SharedFlow<Unit> get() = _onConnected

    public val onDisconnected: SharedFlow<Unit> get() = _onDisconnected

    public val onConnectionActive: SharedFlow<Boolean> get() = _onConnectionActive
}
