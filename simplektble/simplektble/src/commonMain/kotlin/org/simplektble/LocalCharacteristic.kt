package org.simplektble

import kotlin.concurrent.Volatile
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.IO
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.withContext

public class LocalCharacteristic internal constructor(private val instanceId: Long) {
    private val _onWrite = eventFlow<ByteArray>()
    private val _onSubscribed = eventFlow<Unit>()
    private val _onUnsubscribed = eventFlow<Unit>()

    @Volatile
    private var readHandler: (() -> ByteArray)? = null

    private val callbacks = object : LocalCharacteristicCallback {
        override fun onRead(): ByteArray? = readHandler?.invoke()?.copyOf()

        override fun onWrite(value: ByteArray) {
            _onWrite.tryEmit(value)
        }

        override fun onSubscribed() {
            _onSubscribed.tryEmit(Unit)
        }

        override fun onUnsubscribed() {
            _onUnsubscribed.tryEmit(Unit)
        }
    }

    init {
        Native.localCharacteristicRegister(instanceId, callbacks)
    }

    public val uuid: BluetoothUUID = BluetoothUUID(Native.localCharacteristicUuid(instanceId))

    public val capabilities: Set<LocalCharacteristicCapability> = Native.localCharacteristicCapabilities(instanceId)
        .let { mask -> LocalCharacteristicCapability.entries.filterTo(linkedSetOf()) { mask and it.bit != 0 } }

    /** The stored value, which [setValue] changes. */
    public val value: ByteArray
        get() = Native.localCharacteristicValue(instanceId).copyOf()

    /** Stores a new value, and notifies or indicates subscribed clients, which can wait on the Bluetooth stack. */
    @Throws(SimpleKtBleException::class, CancellationException::class)
    public suspend fun setValue(value: ByteArray) {
        val copy = value.copyOf()
        withContext(Dispatchers.IO) { Native.localCharacteristicSetValue(instanceId, copy) }
    }

    public val onWrite: SharedFlow<ByteArray>
        get() = _onWrite

    public val onSubscribed: SharedFlow<Unit>
        get() = _onSubscribed

    public val onUnsubscribed: SharedFlow<Unit>
        get() = _onUnsubscribed

    /**
     * Computes the value for each read. Pass null to serve [value] directly, which is also served if the handler throws.
     * The handler runs on a SimpleBLE thread and should return promptly.
     */
    @Throws(SimpleKtBleException::class)
    public fun setReadHandler(handler: (() -> ByteArray)?) {
        // NOTE: The native handler is turned off first, so no read finds it on without a Kotlin handler.
        if (handler == null) {
            Native.localCharacteristicSetReadHandler(instanceId, false)
            readHandler = null
        } else {
            readHandler = handler
            Native.localCharacteristicSetReadHandler(instanceId, true)
        }
    }
}
