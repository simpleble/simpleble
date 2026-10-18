package org.simplektble

import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.IO
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.flow.buffer
import kotlinx.coroutines.flow.callbackFlow
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlin.concurrent.Volatile

public class Peripheral internal constructor(
    private val adapterId: Long,
    private val instanceId: Long
) {

    private val _onConnected = eventFlow<Unit>()
    private val _onConnectionActive = eventFlow<Boolean>(replay = 1)
    private val _onDisconnected = eventFlow<Unit>()

    @Volatile
    private var passkeyRequestHandler: (() -> String?)? = null

    @Volatile
    private var passkeyDisplayHandler: ((String) -> Unit)? = null

    @Volatile
    private var numericComparisonHandler: ((String) -> Boolean)? = null

    private val callbacks = object : PeripheralCallback {
        override fun onConnected() {
            _onConnected.tryEmit(Unit)
            _onConnectionActive.tryEmit(true)
        }

        override fun onDisconnected() {
            _onDisconnected.tryEmit(Unit)
            _onConnectionActive.tryEmit(false)
        }

        override fun onPasskeyRequest(): String? = passkeyRequestHandler?.invoke()

        override fun onPasskeyDisplay(passkey: String) {
            passkeyDisplayHandler?.invoke(passkey)
        }

        override fun onNumericComparison(passkey: String): Boolean = numericComparisonHandler?.invoke(passkey) ?: false
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

    public fun notify(service: BluetoothUUID, characteristic: BluetoothUUID): Flow<ByteArray> {
        return subscription(service, characteristic, indicate = false)
    }

    public fun indicate(service: BluetoothUUID, characteristic: BluetoothUUID): Flow<ByteArray> {
        return subscription(service, characteristic, indicate = true)
    }

    // Collecting subscribes, and the subscription ends when the collection ends or the peripheral disconnects.
    private fun subscription(
        service: BluetoothUUID,
        characteristic: BluetoothUUID,
        indicate: Boolean
    ): Flow<ByteArray> = callbackFlow {
        val dataCallback = object : DataCallback {
            override fun onDataReceived(data: ByteArray) {
                trySend(data)
            }
        }

        // NOTE: Values and the disconnection arrive in order, so the flow delivers what came before it, then completes.
        val disconnection = launch(start = CoroutineStart.UNDISPATCHED) {
            _onDisconnected.first()
            close()
        }

        var subscribed = false
        try {
            withContext(NonCancellable + Dispatchers.IO) {
                if (indicate) {
                    Native.peripheralIndicate(adapterId, instanceId, service.toString(), characteristic.toString(), dataCallback)
                } else {
                    Native.peripheralNotify(adapterId, instanceId, service.toString(), characteristic.toString(), dataCallback)
                }
                subscribed = true
            }
            awaitClose {}
        } finally {
            disconnection.cancel()
            if (subscribed) {
                withContext(NonCancellable) {
                    try {
                        unsubscribe(service, characteristic)
                    } catch (exception: SimpleKtBleException) {
                        // After a disconnection there is no subscription left to end.
                        if (isConnected) throw exception
                    }
                }
            }
        }
    }.buffer(Channel.UNLIMITED)

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

    /**
     * Answers a Dongl's request for the passkey the peer displays: six decimal digits, or null to reject pairing.
     * Pairing can begin as soon as the peripheral connects, so set it before [connect]. Pass null to remove it.
     * The handler runs on a SimpleBLE thread, which waits for it.
     */
    public fun setPasskeyRequestHandler(handler: (() -> String?)?) {
        passkeyRequestHandler = handler
        Native.peripheralSetPasskeyRequestHandler(adapterId, instanceId, handler != null)
    }

    /** Receives the six-digit passkey a Dongl asks the user to enter on the peer. Set it before [connect]. */
    public fun setPasskeyDisplayHandler(handler: ((String) -> Unit)?) {
        passkeyDisplayHandler = handler
        Native.peripheralSetPasskeyDisplayHandler(adapterId, instanceId, handler != null)
    }

    /**
     * Confirms whether the six-digit passkey a Dongl shows matches the peer's. Return true to accept pairing.
     * Set it before [connect]. The handler runs on a SimpleBLE thread, which waits for it.
     */
    public fun setNumericComparisonHandler(handler: ((String) -> Boolean)?) {
        numericComparisonHandler = handler
        Native.peripheralSetNumericComparisonHandler(adapterId, instanceId, handler != null)
    }

    public val onConnected: SharedFlow<Unit> get() = _onConnected

    public val onDisconnected: SharedFlow<Unit> get() = _onDisconnected

    public val onConnectionActive: SharedFlow<Boolean> get() = _onConnectionActive
}
