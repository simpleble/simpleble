@file:OptIn(ExperimentalForeignApi::class)

package org.simplektble

import kotlinx.atomicfu.locks.SynchronizedObject
import kotlinx.atomicfu.locks.synchronized
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.CPointed
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CPointerVar
import kotlinx.cinterop.CValue
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.UByteVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.convert
import kotlinx.cinterop.cValue
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.readBytes
import kotlinx.cinterop.reinterpret
import kotlinx.cinterop.set
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.staticCFunction
import kotlinx.cinterop.toCPointer
import kotlinx.cinterop.toKString
import kotlinx.cinterop.toLong
import kotlinx.cinterop.usePinned
import kotlinx.cinterop.addressOf
import kotlinx.cinterop.value
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.launch
import org.simplektble.cinterop.SIMPLEBLE_UUID_STR_LEN
import org.simplektble.cinterop.simpleble_adapter_get_count
import org.simplektble.cinterop.simpleble_adapter_get_handle
import org.simplektble.cinterop.simpleble_adapter_get_paired_peripherals_count
import org.simplektble.cinterop.simpleble_adapter_get_paired_peripherals_handle
import org.simplektble.cinterop.simpleble_adapter_identifier
import org.simplektble.cinterop.simpleble_adapter_address
import org.simplektble.cinterop.simpleble_adapter_is_bluetooth_enabled
import org.simplektble.cinterop.simpleble_adapter_release_handle
import org.simplektble.cinterop.simpleble_adapter_scan_for
import org.simplektble.cinterop.simpleble_adapter_scan_get_results_count
import org.simplektble.cinterop.simpleble_adapter_scan_get_results_handle
import org.simplektble.cinterop.simpleble_adapter_scan_is_active
import org.simplektble.cinterop.simpleble_adapter_scan_start
import org.simplektble.cinterop.simpleble_adapter_scan_stop
import org.simplektble.cinterop.simpleble_adapter_set_callback_on_scan_found
import org.simplektble.cinterop.simpleble_adapter_set_callback_on_scan_start
import org.simplektble.cinterop.simpleble_adapter_set_callback_on_scan_stop
import org.simplektble.cinterop.simpleble_adapter_set_callback_on_scan_updated
import org.simplektble.cinterop.simpleble_adapter_t
import org.simplektble.cinterop.simpleble_adapter_underlying
import org.simplektble.cinterop.simpleble_error_message
import org.simplektble.cinterop.simpleble_error_release
import org.simplektble.cinterop.simpleble_error_t
import org.simplektble.cinterop.simpleble_free
import org.simplektble.cinterop.simpleble_get_version
import org.simplektble.cinterop.simpleble_manufacturer_data_release
import org.simplektble.cinterop.simpleble_manufacturer_data_t
import org.simplektble.cinterop.simpleble_peripheral_address
import org.simplektble.cinterop.simpleble_peripheral_address_type
import org.simplektble.cinterop.simpleble_peripheral_connect
import org.simplektble.cinterop.simpleble_peripheral_disconnect
import org.simplektble.cinterop.simpleble_peripheral_identifier
import org.simplektble.cinterop.simpleble_peripheral_indicate
import org.simplektble.cinterop.simpleble_peripheral_is_connectable
import org.simplektble.cinterop.simpleble_peripheral_is_connected
import org.simplektble.cinterop.simpleble_peripheral_is_paired
import org.simplektble.cinterop.simpleble_peripheral_manufacturer_data_count
import org.simplektble.cinterop.simpleble_peripheral_manufacturer_data_get
import org.simplektble.cinterop.simpleble_peripheral_mtu
import org.simplektble.cinterop.simpleble_peripheral_notify
import org.simplektble.cinterop.simpleble_peripheral_read
import org.simplektble.cinterop.simpleble_peripheral_read_descriptor
import org.simplektble.cinterop.simpleble_peripheral_release_handle
import org.simplektble.cinterop.simpleble_peripheral_rssi
import org.simplektble.cinterop.simpleble_peripheral_services_count
import org.simplektble.cinterop.simpleble_peripheral_services_get
import org.simplektble.cinterop.simpleble_peripheral_set_callback_on_connected
import org.simplektble.cinterop.simpleble_peripheral_set_callback_on_disconnected
import org.simplektble.cinterop.simpleble_peripheral_t
import org.simplektble.cinterop.simpleble_peripheral_tx_power
import org.simplektble.cinterop.simpleble_peripheral_underlying
import org.simplektble.cinterop.simpleble_peripheral_unpair
import org.simplektble.cinterop.simpleble_peripheral_unsubscribe
import org.simplektble.cinterop.simpleble_peripheral_write_command
import org.simplektble.cinterop.simpleble_peripheral_write_descriptor
import org.simplektble.cinterop.simpleble_peripheral_write_request
import org.simplektble.cinterop.simpleble_service_release
import org.simplektble.cinterop.simpleble_service_t
import org.simplektble.cinterop.simpleble_uuid_t
import platform.posix.memset
import platform.posix.size_tVar

/**
 * Native over the SimpleCBLE C API.
 *
 * SimpleCBLE hands out a new handle every time it returns an adapter or peripheral, so each one is identified by the
 * SimpleBLE object behind it. The first handle is kept and later ones are released. Callbacks receive the ID as
 * their userdata and run, in order, on one coroutine, the same way the JNI layer runs them on one thread.
 */
internal actual object Native {
    private data class Subscription(val peripheralId: Long, val service: String, val characteristic: String)

    private val lock = SynchronizedObject()
    private val adapters = mutableMapOf<Long, simpleble_adapter_t>()
    private val peripherals = mutableMapOf<Long, simpleble_peripheral_t>()
    private val adapterCallbacks = mutableMapOf<Long, AdapterCallback>()
    private val peripheralCallbacks = mutableMapOf<Long, PeripheralCallback>()
    private val subscriptions = mutableMapOf<Subscription, Long>()
    private val dataCallbacks = mutableMapOf<Long, DataCallback>()
    private var nextSubscriptionId = 1L

    private val events = Channel<() -> Unit>(Channel.UNLIMITED)

    init {
        CoroutineScope(Dispatchers.Default).launch {
            for (event in events) {
                try {
                    event()
                } catch (exception: Throwable) {
                    println("SimpleKtBLE: callback failed: $exception")
                }
            }
        }
    }

    private fun dispatch(event: () -> Unit) {
        events.trySend(event)
    }

    actual fun version(): String = simpleble_get_version()?.toKString() ?: ""

    actual fun isBluetoothEnabled(): Boolean = call { simpleble_adapter_is_bluetooth_enabled(it) }

    actual fun getAdapters(): LongArray {
        val count = call { simpleble_adapter_get_count(it) }.toInt()
        return LongArray(count) { index -> internAdapter(call { simpleble_adapter_get_handle(index.convert(), it) }) }
    }

    actual fun adapterRegister(adapterId: Long, callback: AdapterCallback) {
        val handle = adapter(adapterId)
        synchronized(lock) { adapterCallbacks[adapterId] = callback }
        val userdata = adapterId.toCPointer<CPointed>()
        simpleble_adapter_set_callback_on_scan_start(handle, onScanStart, userdata)
        simpleble_adapter_set_callback_on_scan_stop(handle, onScanStop, userdata)
        simpleble_adapter_set_callback_on_scan_updated(handle, onScanUpdated, userdata)
        simpleble_adapter_set_callback_on_scan_found(handle, onScanFound, userdata)
    }

    actual fun adapterIdentifier(adapterId: Long): String {
        val handle = adapter(adapterId)
        return call { simpleble_adapter_identifier(handle, it) }.consume()
    }

    actual fun adapterAddress(adapterId: Long): String {
        val handle = adapter(adapterId)
        return call { simpleble_adapter_address(handle, it) }.consume()
    }

    actual fun adapterScanStart(adapterId: Long) {
        val handle = adapter(adapterId)
        call { simpleble_adapter_scan_start(handle, it) }
    }

    actual fun adapterScanStop(adapterId: Long) {
        val handle = adapter(adapterId)
        call { simpleble_adapter_scan_stop(handle, it) }
    }

    actual fun adapterScanFor(adapterId: Long, timeoutMs: Int) {
        val handle = adapter(adapterId)
        call { simpleble_adapter_scan_for(handle, timeoutMs, it) }
    }

    actual fun adapterScanIsActive(adapterId: Long): Boolean {
        val handle = adapter(adapterId)
        return call { simpleble_adapter_scan_is_active(handle, it) }
    }

    actual fun adapterScanGetResults(adapterId: Long): LongArray {
        val handle = adapter(adapterId)
        val count = call { simpleble_adapter_scan_get_results_count(handle, it) }.toInt()
        return LongArray(count) { index ->
            internPeripheral(call { simpleble_adapter_scan_get_results_handle(handle, index.convert(), it) })
        }
    }

    actual fun adapterGetPairedPeripherals(adapterId: Long): LongArray {
        val handle = adapter(adapterId)
        val count = call { simpleble_adapter_get_paired_peripherals_count(handle, it) }.toInt()
        return LongArray(count) { index ->
            internPeripheral(call { simpleble_adapter_get_paired_peripherals_handle(handle, index.convert(), it) })
        }
    }

    actual fun peripheralRegister(adapterId: Long, peripheralId: Long, callback: PeripheralCallback) {
        val handle = peripheral(peripheralId)
        synchronized(lock) { peripheralCallbacks[peripheralId] = callback }
        val userdata = peripheralId.toCPointer<CPointed>()
        simpleble_peripheral_set_callback_on_connected(handle, onConnected, userdata)
        simpleble_peripheral_set_callback_on_disconnected(handle, onDisconnected, userdata)
    }

    actual fun peripheralIdentifier(adapterId: Long, peripheralId: Long): String {
        val handle = peripheral(peripheralId)
        return call { simpleble_peripheral_identifier(handle, it) }.consume()
    }

    actual fun peripheralAddress(adapterId: Long, peripheralId: Long): String {
        val handle = peripheral(peripheralId)
        return call { simpleble_peripheral_address(handle, it) }.consume()
    }

    actual fun peripheralAddressType(adapterId: Long, peripheralId: Long): Int {
        val handle = peripheral(peripheralId)
        return call { simpleble_peripheral_address_type(handle, it) }.toInt()
    }

    actual fun peripheralRssi(adapterId: Long, peripheralId: Long): Int {
        val handle = peripheral(peripheralId)
        return call { simpleble_peripheral_rssi(handle, it) }.toInt()
    }

    actual fun peripheralTxPower(adapterId: Long, peripheralId: Long): Int {
        val handle = peripheral(peripheralId)
        return call { simpleble_peripheral_tx_power(handle, it) }.toInt()
    }

    actual fun peripheralMtu(adapterId: Long, peripheralId: Long): Int {
        val handle = peripheral(peripheralId)
        return call { simpleble_peripheral_mtu(handle, it) }.toInt()
    }

    actual fun peripheralConnect(adapterId: Long, peripheralId: Long) {
        val handle = peripheral(peripheralId)
        call { simpleble_peripheral_connect(handle, it) }
    }

    actual fun peripheralDisconnect(adapterId: Long, peripheralId: Long) {
        val handle = peripheral(peripheralId)
        call { simpleble_peripheral_disconnect(handle, it) }
    }

    actual fun peripheralIsConnected(adapterId: Long, peripheralId: Long): Boolean {
        val handle = peripheral(peripheralId)
        return call { simpleble_peripheral_is_connected(handle, it) }
    }

    actual fun peripheralIsConnectable(adapterId: Long, peripheralId: Long): Boolean {
        val handle = peripheral(peripheralId)
        return call { simpleble_peripheral_is_connectable(handle, it) }
    }

    actual fun peripheralIsPaired(adapterId: Long, peripheralId: Long): Boolean {
        val handle = peripheral(peripheralId)
        return call { simpleble_peripheral_is_paired(handle, it) }
    }

    actual fun peripheralUnpair(adapterId: Long, peripheralId: Long) {
        val handle = peripheral(peripheralId)
        call { simpleble_peripheral_unpair(handle, it) }
    }

    actual fun peripheralServices(adapterId: Long, peripheralId: Long): List<Service> {
        val handle = peripheral(peripheralId)
        val count = call { simpleble_peripheral_services_count(handle, it) }.toInt()
        return List(count) { index ->
            memScoped {
                val service = alloc<simpleble_service_t>()
                memset(service.ptr, 0, sizeOf<simpleble_service_t>().convert())
                call { simpleble_peripheral_services_get(handle, index.convert(), service.ptr, it) }
                try {
                    val characteristics = List(service.characteristic_count.toInt()) { characteristicIndex ->
                        val characteristic = service.characteristics!![characteristicIndex]
                        Characteristic(
                            characteristic.uuid.value.toKString(),
                            List(characteristic.descriptor_count.toInt()) { descriptorIndex ->
                                Descriptor(characteristic.descriptors!![descriptorIndex].uuid.value.toKString())
                            },
                            characteristic.can_read,
                            characteristic.can_write_request,
                            characteristic.can_write_command,
                            characteristic.can_notify,
                            characteristic.can_indicate
                        )
                    }
                    Service(service.uuid.value.toKString(), characteristics)
                } finally {
                    simpleble_service_release(service.ptr)
                }
            }
        }
    }

    actual fun peripheralManufacturerData(adapterId: Long, peripheralId: Long): Map<Int, ByteArray> {
        val handle = peripheral(peripheralId)
        val count = call { simpleble_peripheral_manufacturer_data_count(handle, it) }.toInt()
        val manufacturerData = mutableMapOf<Int, ByteArray>()
        for (index in 0 until count) {
            memScoped {
                val data = alloc<simpleble_manufacturer_data_t>()
                memset(data.ptr, 0, sizeOf<simpleble_manufacturer_data_t>().convert())
                call { simpleble_peripheral_manufacturer_data_get(handle, index.convert(), data.ptr, it) }
                try {
                    manufacturerData[data.manufacturer_id.toInt()] = data.data.toByteArray(data.data_length.toInt())
                } finally {
                    simpleble_manufacturer_data_release(data.ptr)
                }
            }
        }
        return manufacturerData
    }

    actual fun peripheralRead(adapterId: Long, peripheralId: Long, service: String, characteristic: String): ByteArray {
        val handle = peripheral(peripheralId)
        return memScoped {
            val length = alloc<size_tVar>()
            val data = call { simpleble_peripheral_read(handle, uuid(service), uuid(characteristic), length.ptr, it) }
            try {
                data.toByteArray(length.value.toInt())
            } finally {
                simpleble_free(data)
            }
        }
    }

    actual fun peripheralWriteRequest(
        adapterId: Long,
        peripheralId: Long,
        service: String,
        characteristic: String,
        data: ByteArray
    ) {
        val handle = peripheral(peripheralId)
        data.withPointer { pointer, length ->
            call { simpleble_peripheral_write_request(handle, uuid(service), uuid(characteristic), pointer, length, it) }
        }
    }

    actual fun peripheralWriteCommand(
        adapterId: Long,
        peripheralId: Long,
        service: String,
        characteristic: String,
        data: ByteArray
    ) {
        val handle = peripheral(peripheralId)
        data.withPointer { pointer, length ->
            call { simpleble_peripheral_write_command(handle, uuid(service), uuid(characteristic), pointer, length, it) }
        }
    }

    actual fun peripheralNotify(
        adapterId: Long,
        peripheralId: Long,
        service: String,
        characteristic: String,
        callback: DataCallback
    ) {
        subscribe(peripheralId, service, characteristic, callback, indicate = false)
    }

    actual fun peripheralIndicate(
        adapterId: Long,
        peripheralId: Long,
        service: String,
        characteristic: String,
        callback: DataCallback
    ) {
        subscribe(peripheralId, service, characteristic, callback, indicate = true)
    }

    actual fun peripheralUnsubscribe(adapterId: Long, peripheralId: Long, service: String, characteristic: String) {
        val handle = peripheral(peripheralId)
        try {
            call { simpleble_peripheral_unsubscribe(handle, uuid(service), uuid(characteristic), it) }
        } finally {
            synchronized(lock) {
                subscriptions.remove(Subscription(peripheralId, service, characteristic))?.let(dataCallbacks::remove)
            }
        }
    }

    actual fun peripheralDescriptorRead(
        adapterId: Long,
        peripheralId: Long,
        service: String,
        characteristic: String,
        descriptor: String
    ): ByteArray {
        val handle = peripheral(peripheralId)
        return memScoped {
            val length = alloc<size_tVar>()
            val data = call {
                simpleble_peripheral_read_descriptor(
                    handle, uuid(service), uuid(characteristic), uuid(descriptor), length.ptr, it
                )
            }
            try {
                data.toByteArray(length.value.toInt())
            } finally {
                simpleble_free(data)
            }
        }
    }

    actual fun peripheralDescriptorWrite(
        adapterId: Long,
        peripheralId: Long,
        service: String,
        characteristic: String,
        descriptor: String,
        data: ByteArray
    ) {
        val handle = peripheral(peripheralId)
        data.withPointer { pointer, length ->
            call {
                simpleble_peripheral_write_descriptor(
                    handle, uuid(service), uuid(characteristic), uuid(descriptor), pointer, length, it
                )
            }
        }
    }

    // ----------------------------------------------------------------------------

    private fun subscribe(
        peripheralId: Long,
        service: String,
        characteristic: String,
        callback: DataCallback,
        indicate: Boolean
    ) {
        val handle = peripheral(peripheralId)
        val subscription = Subscription(peripheralId, service, characteristic)
        val subscriptionId = synchronized(lock) {
            if (subscription in subscriptions) {
                val kind = if (indicate) "An indication" else "A notification"
                throw SimpleKtBleException("$kind collector is already active for characteristic $characteristic")
            }
            val id = nextSubscriptionId++
            subscriptions[subscription] = id
            dataCallbacks[id] = callback
            id
        }

        try {
            val userdata = subscriptionId.toCPointer<CPointed>()
            call {
                if (indicate) {
                    simpleble_peripheral_indicate(handle, uuid(service), uuid(characteristic), onData, userdata, it)
                } else {
                    simpleble_peripheral_notify(handle, uuid(service), uuid(characteristic), onData, userdata, it)
                }
            }
        } catch (exception: Throwable) {
            synchronized(lock) {
                if (subscriptions[subscription] == subscriptionId) subscriptions.remove(subscription)
                dataCallbacks.remove(subscriptionId)
            }
            throw exception
        }
    }

    private fun adapter(adapterId: Long): simpleble_adapter_t {
        return synchronized(lock) { adapters[adapterId] } ?: throw SimpleKtBleException("Unknown adapter")
    }

    private fun peripheral(peripheralId: Long): simpleble_peripheral_t {
        return synchronized(lock) { peripherals[peripheralId] } ?: throw SimpleKtBleException("Unknown peripheral")
    }

    private fun internAdapter(handle: simpleble_adapter_t?): Long {
        if (handle == null) throw SimpleKtBleException("SimpleBLE returned no adapter")
        val id = call { simpleble_adapter_underlying(handle, it) }.toLong()
        val known = synchronized(lock) { adapters.getOrPut(id) { handle } != handle }
        if (known) simpleble_adapter_release_handle(handle)
        return id
    }

    internal fun internPeripheral(handle: simpleble_peripheral_t?): Long {
        if (handle == null) throw SimpleKtBleException("SimpleBLE returned no peripheral")
        val id = call { simpleble_peripheral_underlying(handle, it) }.toLong()
        val known = synchronized(lock) { peripherals.getOrPut(id) { handle } != handle }
        if (known) simpleble_peripheral_release_handle(handle)
        return id
    }

    internal fun onAdapterEvent(adapterId: Long, event: (AdapterCallback) -> Unit) {
        val callback = synchronized(lock) { adapterCallbacks[adapterId] } ?: return
        dispatch { event(callback) }
    }

    internal fun onPeripheralEvent(peripheralId: Long, event: (PeripheralCallback) -> Unit) {
        val callback = synchronized(lock) { peripheralCallbacks[peripheralId] } ?: return
        dispatch { event(callback) }
    }

    internal fun onDataEvent(subscriptionId: Long, data: ByteArray) {
        val callback = synchronized(lock) { dataCallbacks[subscriptionId] } ?: return
        dispatch { callback.onDataReceived(data) }
    }
}

private val onScanStart = staticCFunction { _: simpleble_adapter_t?, userdata: COpaquePointer? ->
    guarded { Native.onAdapterEvent(userdata.toLong()) { it.onScanStart() } }
}

private val onScanStop = staticCFunction { _: simpleble_adapter_t?, userdata: COpaquePointer? ->
    guarded { Native.onAdapterEvent(userdata.toLong()) { it.onScanStop() } }
}

private val onScanUpdated = staticCFunction { _: simpleble_adapter_t?, peripheral: simpleble_peripheral_t?, userdata: COpaquePointer? ->
    guarded {
        val peripheralId = Native.internPeripheral(peripheral)
        Native.onAdapterEvent(userdata.toLong()) { it.onScanUpdated(peripheralId) }
    }
}

private val onScanFound = staticCFunction { _: simpleble_adapter_t?, peripheral: simpleble_peripheral_t?, userdata: COpaquePointer? ->
    guarded {
        val peripheralId = Native.internPeripheral(peripheral)
        Native.onAdapterEvent(userdata.toLong()) { it.onScanFound(peripheralId) }
    }
}

private val onConnected = staticCFunction { _: simpleble_peripheral_t?, userdata: COpaquePointer? ->
    guarded { Native.onPeripheralEvent(userdata.toLong()) { it.onConnected() } }
}

private val onDisconnected = staticCFunction { _: simpleble_peripheral_t?, userdata: COpaquePointer? ->
    guarded { Native.onPeripheralEvent(userdata.toLong()) { it.onDisconnected() } }
}

private val onData = staticCFunction {
        _: simpleble_peripheral_t?,
        _: CValue<simpleble_uuid_t>,
        _: CValue<simpleble_uuid_t>,
        data: CPointer<UByteVar>?,
        length: ULong,
        userdata: COpaquePointer? ->
    guarded { Native.onDataEvent(userdata.toLong(), data.toByteArray(length.toInt())) }
}

/** Exceptions must not cross back into C. */
private inline fun guarded(block: () -> Unit) {
    try {
        block()
    } catch (exception: Throwable) {
        println("SimpleKtBLE: native callback failed: $exception")
    }
}

/** Runs a SimpleCBLE call and throws its error, if any, as a [SimpleKtBleException]. */
private inline fun <T> call(block: (CPointer<CPointerVar<simpleble_error_t>>) -> T): T = memScoped {
    val error = alloc<CPointerVar<simpleble_error_t>>()
    error.value = null
    val result = block(error.ptr)
    error.value?.let { raised ->
        val message = simpleble_error_message(raised)?.toKString() ?: "Unknown native error"
        simpleble_error_release(error.ptr)
        throw SimpleKtBleException(message)
    }
    result
}

private fun uuid(value: String): CValue<simpleble_uuid_t> {
    val bytes = value.encodeToByteArray()
    if (bytes.size >= SIMPLEBLE_UUID_STR_LEN) throw SimpleKtBleException("Invalid UUID: $value")
    return cValue {
        bytes.forEachIndexed { index, byte -> this.value[index] = byte }
        this.value[bytes.size] = 0
    }
}

/** Returns the string and frees the copy SimpleCBLE allocated. */
private fun CPointer<ByteVar>?.consume(): String {
    try {
        return this?.toKString() ?: ""
    } finally {
        simpleble_free(this)
    }
}

private fun CPointer<UByteVar>?.toByteArray(length: Int): ByteArray {
    if (this == null || length == 0) return ByteArray(0)
    return reinterpret<ByteVar>().readBytes(length)
}

private inline fun ByteArray.withPointer(block: (CPointer<UByteVar>?, ULong) -> Unit) {
    if (isEmpty()) return block(null, 0u)
    usePinned { pinned -> block(pinned.addressOf(0).reinterpret(), size.toULong()) }
}
