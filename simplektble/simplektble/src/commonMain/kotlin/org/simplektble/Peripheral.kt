package org.simplektble

import kotlinx.atomicfu.locks.SynchronizedObject
import kotlinx.atomicfu.locks.synchronized
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.IO
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.channels.SendChannel
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.flow.buffer
import kotlinx.coroutines.flow.callbackFlow
import kotlinx.coroutines.withContext
import kotlin.concurrent.Volatile

public class Peripheral internal constructor(
    private val adapterId: Long,
    private val instanceId: Long
) {

    private val _onConnected = eventFlow<Unit>()
    private val _onConnectionActive = eventFlow<Boolean>(replay = 1)
    private val _onDisconnected = eventFlow<Unit>()

    private val subscriptionsLock = SynchronizedObject()
    private val subscriptions = mutableMapOf<Pair<BluetoothUUID, BluetoothUUID>, SendChannel<ByteArray>>()

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

    @Throws(SimpleKtBleException::class, CancellationException::class)
    public suspend fun connect() {
        withContext(Dispatchers.IO) {
            Native.peripheralConnect(adapterId, instanceId)
        }
    }

    /** Disconnects. It finishes even when the calling coroutine is cancelled, so it can run in a finally block. */
    @Throws(SimpleKtBleException::class, CancellationException::class)
    public suspend fun disconnect() {
        runToEnd {
            Native.peripheralDisconnect(adapterId, instanceId)
        }
    }

    public val isConnected: Boolean get() {
        return Native.peripheralIsConnected(adapterId, instanceId)
    }

    public val isConnectable: Boolean get() {
        return Native.peripheralIsConnectable(adapterId, instanceId)
    }

    /** Asks the Bluetooth stack whether the peripheral is paired. macOS and iOS do not report pairing, so there it throws. */
    @Throws(SimpleKtBleException::class, CancellationException::class)
    public suspend fun isPaired(): Boolean {
        return withContext(Dispatchers.IO) {
            Native.peripheralIsPaired(adapterId, instanceId)
        }
    }

    /** Removes the pairing. On Linux BlueZ removes the device, and a Dongl deletes its bond. */
    @Throws(SimpleKtBleException::class, CancellationException::class)
    public suspend fun unpair() {
        withContext(Dispatchers.IO) {
            Native.peripheralUnpair(adapterId, instanceId)
        }
    }

    @Throws(SimpleKtBleException::class)
    public fun services(): List<Service> {
        return Native.peripheralServices(adapterId, instanceId)
    }

    @Throws(SimpleKtBleException::class)
    public fun manufacturerData(): Map<Int, ByteArray> {
        return Native.peripheralManufacturerData(adapterId, instanceId)
    }

    @Throws(SimpleKtBleException::class, CancellationException::class)
    public suspend fun read(service: BluetoothUUID, characteristic: BluetoothUUID): ByteArray {
        return withContext(Dispatchers.IO) {
            Native.peripheralRead(adapterId, instanceId, service.toString(), characteristic.toString())
        }
    }

    @Throws(SimpleKtBleException::class, CancellationException::class)
    public suspend fun writeRequest(service: BluetoothUUID, characteristic: BluetoothUUID, data: ByteArray) {
        withContext(Dispatchers.IO) {
            Native.peripheralWriteRequest(adapterId, instanceId, service.toString(), characteristic.toString(), data)
        }
    }

    @Throws(SimpleKtBleException::class, CancellationException::class)
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

    // Collecting subscribes, and the subscription ends when the collection ends, the peripheral disconnects, or
    // unsubscribe() is called. Each characteristic has one subscription at a time, owned by the collector that made it.
    private fun subscription(
        service: BluetoothUUID,
        characteristic: BluetoothUUID,
        indicate: Boolean
    ): Flow<ByteArray> = callbackFlow {
        val key = service to characteristic
        val dataCallback = object : DataCallback {
            override fun onDataReceived(data: ByteArray) {
                trySend(data)
            }

            // NOTE: Values and the disconnection arrive in order, so the flow delivers what came first, then ends.
            override fun onEnded() {
                release(key, channel)
                close()
            }
        }

        val claimed = synchronized(subscriptionsLock) { subscriptions.getOrPut(key) { channel } === channel }
        if (!claimed) {
            val kind = if (indicate) "An indication" else "A notification"
            val message = "$kind collector is already active for characteristic $characteristic"
            throw SimpleKtBleException(ErrorCode.OPERATION_FAILED, message)
        }

        try {
            runToEnd {
                if (indicate) {
                    Native.peripheralIndicate(adapterId, instanceId, service.toString(), characteristic.toString(), dataCallback)
                } else {
                    Native.peripheralNotify(adapterId, instanceId, service.toString(), characteristic.toString(), dataCallback)
                }
            }
        } catch (exception: Throwable) {
            release(key, channel)
            throw exception
        }

        try {
            awaitClose {}
        } finally {
            // Only a collection that ended on its own still owns the subscription. After a disconnection or
            // unsubscribe(), the next subscription may already be another collector's.
            if (release(key, channel)) {
                runToEnd {
                    try {
                        Native.peripheralUnsubscribe(adapterId, instanceId, service.toString(), characteristic.toString())
                    } catch (exception: SimpleKtBleException) {
                        if (isConnected) throw exception
                    }
                }
            }
        }
    }.buffer(Channel.UNLIMITED)

    /** Gives up ownership of a subscription, and returns whether [collector] still had it. */
    private fun release(key: Pair<BluetoothUUID, BluetoothUUID>, collector: SendChannel<ByteArray>): Boolean {
        return synchronized(subscriptionsLock) {
            (subscriptions[key] === collector).also { owned -> if (owned) subscriptions.remove(key) }
        }
    }

    /**
     * Ends the subscription to a characteristic, and completes the flow that collects it. It finishes even when the
     * calling coroutine is cancelled.
     */
    @Throws(SimpleKtBleException::class, CancellationException::class)
    public suspend fun unsubscribe(service: BluetoothUUID, characteristic: BluetoothUUID) {
        val collector = synchronized(subscriptionsLock) { subscriptions.remove(service to characteristic) }
        try {
            runToEnd {
                Native.peripheralUnsubscribe(adapterId, instanceId, service.toString(), characteristic.toString())
            }
        } finally {
            collector?.close()
        }
    }

    @Throws(SimpleKtBleException::class, CancellationException::class)
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

    @Throws(SimpleKtBleException::class, CancellationException::class)
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
    @Throws(SimpleKtBleException::class)
    public fun setPasskeyRequestHandler(handler: (() -> String?)?) {
        passkeyRequestHandler = handler
        Native.peripheralSetPasskeyRequestHandler(adapterId, instanceId, handler != null)
    }

    /** Receives the six-digit passkey a Dongl asks the user to enter on the peer. Set it before [connect]. */
    @Throws(SimpleKtBleException::class)
    public fun setPasskeyDisplayHandler(handler: ((String) -> Unit)?) {
        passkeyDisplayHandler = handler
        Native.peripheralSetPasskeyDisplayHandler(adapterId, instanceId, handler != null)
    }

    /**
     * Confirms whether the six-digit passkey a Dongl shows matches the peer's. Return true to accept pairing.
     * Set it before [connect]. The handler runs on a SimpleBLE thread, which waits for it.
     */
    @Throws(SimpleKtBleException::class)
    public fun setNumericComparisonHandler(handler: ((String) -> Boolean)?) {
        numericComparisonHandler = handler
        Native.peripheralSetNumericComparisonHandler(adapterId, instanceId, handler != null)
    }

    public val onConnected: SharedFlow<Unit> get() = _onConnected

    public val onDisconnected: SharedFlow<Unit> get() = _onDisconnected

    public val onConnectionActive: SharedFlow<Boolean> get() = _onConnectionActive
}
