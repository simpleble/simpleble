package org.simplektble

import kotlin.concurrent.Volatile
import kotlinx.coroutines.flow.SharedFlow

public class LocalCharacteristic internal constructor(private val instanceId: Long) {
    private val _onWrite = eventFlow<ByteArray>()
    private val _onSubscribed = eventFlow<Unit>()
    private val _onUnsubscribed = eventFlow<Unit>()

    @Volatile
    private var readHandler: (() -> ByteArray)? = null

    @Volatile
    private var writeHandler: ((ByteArray) -> Unit)? = null

    private val callbacks = object : LocalCharacteristicCallback {
        override fun onRead(): ByteArray {
            return requireNotNull(readHandler) { "No local characteristic read handler is configured." }
                .invoke()
                .copyOf()
        }

        override fun onWrite(value: ByteArray) {
            val snapshot = value.copyOf()
            writeHandler?.invoke(snapshot.copyOf())
            _onWrite.tryEmit(snapshot)
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

    /** The stored value. Setting it notifies or indicates subscribed clients. */
    public var value: ByteArray
        get() = Native.localCharacteristicValue(instanceId).copyOf()
        set(value) = Native.localCharacteristicSetValue(instanceId, value.copyOf())

    public val onWrite: SharedFlow<ByteArray>
        get() = _onWrite

    public val onSubscribed: SharedFlow<Unit>
        get() = _onSubscribed

    public val onUnsubscribed: SharedFlow<Unit>
        get() = _onUnsubscribed

    /**
     * Computes the value for each read. Pass null to serve [value] directly.
     * The handler runs on a SimpleBLE thread and should return promptly.
     */
    public fun setReadHandler(handler: (() -> ByteArray)?) {
        readHandler = handler
        Native.localCharacteristicSetReadHandler(instanceId, handler != null)
    }

    /**
     * Receives every accepted write on the callback thread, before [onWrite] emits it.
     * The handler should return promptly.
     */
    public fun setWriteHandler(handler: ((ByteArray) -> Unit)?) {
        writeHandler = handler
    }
}
