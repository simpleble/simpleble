package org.simpleble.android

import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow

class LocalCharacteristic internal constructor(private val instanceId: Long) {
    private val _onWrite = MutableSharedFlow<ByteArray>(
        extraBufferCapacity = 16,
        onBufferOverflow = BufferOverflow.DROP_OLDEST
    )
    private val _onSubscribed = MutableSharedFlow<Unit>(extraBufferCapacity = 1)
    private val _onUnsubscribed = MutableSharedFlow<Unit>(extraBufferCapacity = 1)
    @Volatile
    private var readHandler: (() -> ByteArray)? = null
    @Volatile
    private var writeHandler: ((ByteArray) -> Unit)? = null

    private val callbacks = object : Callback {
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
        nativeRegister(instanceId, callbacks)
    }

    val uuid: BluetoothUUID = BluetoothUUID(nativeUuid(instanceId))

    val capabilities: Set<LocalCharacteristicCapability> = nativeCapabilities(instanceId)
        .let { mask -> LocalCharacteristicCapability.entries.filterTo(linkedSetOf()) { mask and it.bit != 0 } }

    var value: ByteArray
        get() = nativeValue(instanceId).copyOf()
        set(value) = nativeSetValue(instanceId, value.copyOf())

    val onWrite: SharedFlow<ByteArray>
        get() = _onWrite

    val onSubscribed: SharedFlow<Unit>
        get() = _onSubscribed

    val onUnsubscribed: SharedFlow<Unit>
        get() = _onUnsubscribed

    /**
     * Configure a dynamic read handler. Pass null to serve [value] directly.
     * The handler runs on an Android Bluetooth callback thread and should
     * return promptly.
     */
    fun setReadHandler(handler: (() -> ByteArray)?) {
        readHandler = handler
        nativeSetReadHandler(instanceId, handler != null)
    }

    /**
     * Configure reliable write handling. The handler receives every accepted
     * write on SimpleDroidBLE's callback thread and should return promptly.
     * [onWrite] remains available for UI and telemetry-style observation.
     */
    fun setWriteHandler(handler: ((ByteArray) -> Unit)?) {
        writeHandler = handler
    }

    private external fun nativeRegister(characteristicId: Long, callback: Callback)
    private external fun nativeUuid(characteristicId: Long): String
    private external fun nativeCapabilities(characteristicId: Long): Int
    private external fun nativeValue(characteristicId: Long): ByteArray
    private external fun nativeSetValue(characteristicId: Long, value: ByteArray)
    private external fun nativeSetReadHandler(characteristicId: Long, enabled: Boolean)

    private interface Callback {
        fun onRead(): ByteArray
        fun onWrite(value: ByteArray)
        fun onSubscribed()
        fun onUnsubscribed()
    }
}
