@file:OptIn(ExperimentalForeignApi::class)

package org.simplektble

import kotlinx.atomicfu.locks.SynchronizedObject
import kotlinx.atomicfu.locks.synchronized
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.COpaquePointerVar
import kotlinx.cinterop.CPointed
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CPointerVar
import kotlinx.cinterop.CValue
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.UByteVar
import kotlinx.cinterop.addressOf
import kotlinx.cinterop.alloc
import kotlinx.cinterop.allocArray
import kotlinx.cinterop.allocArrayOf
import kotlinx.cinterop.cValue
import kotlinx.cinterop.convert
import kotlinx.cinterop.cstr
import kotlinx.cinterop.free
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.nativeHeap
import kotlinx.cinterop.pointed
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
import kotlinx.cinterop.value
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.launch
import org.simplektble.cinterop.*
import platform.posix.memset
import platform.posix.size_tVar

/**
 * Native over the SimpleCBLE C API.
 *
 * SimpleCBLE hands out a new handle every time it returns an adapter, peripheral, local service or local
 * characteristic, so each one is recognized by the value of its simpleble_*_identity() function. The first handle is
 * kept and later ones are released. Each object gets an ID the first time it is seen, and IDs are never reused, so a
 * released object's ID cannot reach another one.
 * Callbacks receive the ID as their userdata and run, in order, on one coroutine, the same way the JNI layer runs them
 * on one thread.
 */
internal actual object Native {
    private data class Subscription(val peripheralId: Long, val service: String, val characteristic: String)

    private data class PeripheralEntry(val adapterId: Long, val handle: simpleble_peripheral_t)

    private val lock = SynchronizedObject()
    // The ID of each object, by the value of its identity function.
    private val ids = mutableMapOf<Long, Long>()
    private var nextId = 1L
    private val adapters = mutableMapOf<Long, simpleble_adapter_t>()
    private val peripherals = mutableMapOf<Long, PeripheralEntry>()
    private val adapterCallbacks = mutableMapOf<Long, AdapterCallback>()
    private val peripheralCallbacks = mutableMapOf<Long, PeripheralCallback>()
    private val subscriptions = mutableMapOf<Subscription, Long>()
    private val dataCallbacks = mutableMapOf<Long, DataCallback>()
    private var nextSubscriptionId = 1L

    // Local peripherals are never listed, so each one gets a new ID.
    private val localPeripherals = mutableMapOf<Long, simpleble_local_peripheral_t>()
    private val localServices = mutableMapOf<Long, simpleble_local_service_t>()
    private val localCharacteristics = mutableMapOf<Long, simpleble_local_characteristic_t>()
    private val localPeripheralCallbacks = mutableMapOf<Long, LocalPeripheralCallback>()
    private val localCharacteristicCallbacks = mutableMapOf<Long, LocalCharacteristicCallback>()
    // NOTE: SimpleCBLE copies a read value after the callback returns, so each buffer lives until the next read.
    private val readBuffers = mutableMapOf<Long, CPointer<UByteVar>>()

    // SimpleBLE has one backend per identifier, so backends are kept by identifier.
    private val backends = mutableMapOf<String, simpleble_backend_t>()

    private val events = Channel<() -> Unit>(Channel.UNLIMITED)

    init {
        CoroutineScope(Dispatchers.Default).launch {
            for (event in events) {
                try {
                    event()
                } catch (exception: Throwable) {
                    logError("dispatch", "Callback failed: $exception")
                }
            }
        }
    }

    private fun dispatch(event: () -> Unit) {
        events.trySend(event)
    }

    actual fun version(): String = simpleble_get_version()?.toKString() ?: ""

    actual fun isBluetoothEnabled(): Boolean = call { simpleble_adapter_is_bluetooth_enabled(it) }

    actual fun getAdapters(): LongArray = readList(
        { list, error -> simpleble_adapter_get_adapters(list, error) },
        { simpleble_adapter_list_release(it) }
    ) { list, index -> internAdapter(call { simpleble_adapter_list_get(list, index.convert(), it) }) }.toLongArray()

    actual fun adapterRegister(adapterId: Long, callback: AdapterCallback) {
        val handle = adapter(adapterId)
        synchronized(lock) { adapterCallbacks[adapterId] = callback }
        val userdata = adapterId.toCPointer<CPointed>()
        simpleble_adapter_set_callback_on_power_on(handle, onPowerOn, userdata)
        simpleble_adapter_set_callback_on_power_off(handle, onPowerOff, userdata)
        simpleble_adapter_set_callback_on_scan_start(handle, onScanStart, userdata)
        simpleble_adapter_set_callback_on_scan_stop(handle, onScanStop, userdata)
        simpleble_adapter_set_callback_on_scan_updated(handle, onScanUpdated, userdata)
        simpleble_adapter_set_callback_on_scan_found(handle, onScanFound, userdata)
    }

    actual fun adapterRelease(adapterId: Long) {
        val handle = adapter(adapterId)
        // NOTE: SimpleBLE keeps an adapter's callbacks while anything holds the adapter, so they are cleared first.
        simpleble_adapter_set_callback_on_power_on(handle, null, null)
        simpleble_adapter_set_callback_on_power_off(handle, null, null)
        simpleble_adapter_set_callback_on_scan_start(handle, null, null)
        simpleble_adapter_set_callback_on_scan_stop(handle, null, null)
        simpleble_adapter_set_callback_on_scan_updated(handle, null, null)
        simpleble_adapter_set_callback_on_scan_found(handle, null, null)

        val (owned, ended) = synchronized(lock) {
            adapters.remove(adapterId)
            adapterCallbacks.remove(adapterId)
            ids.remove(simpleble_adapter_identity(handle).toLong())
            val adapterPeripherals = peripherals.filterValues { it.adapterId == adapterId }
            for ((peripheralId, entry) in adapterPeripherals) {
                peripherals.remove(peripheralId)
                peripheralCallbacks.remove(peripheralId)
                ids.remove(simpleble_peripheral_identity(entry.handle).toLong())
            }
            val keys = subscriptions.keys.filter { it.peripheralId in adapterPeripherals }
            val ended = keys.mapNotNull { key -> subscriptions.remove(key)?.let(dataCallbacks::remove) }
            adapterPeripherals.values to ended
        }
        // NOTE: The subscriptions end after the values queued before them, as on a disconnection.
        dispatch { ended.forEach(DataCallback::onEnded) }
        owned.forEach { simpleble_peripheral_release_handle(it.handle) }
        simpleble_adapter_release_handle(handle)
    }

    actual fun adapterIdentifier(adapterId: Long): String {
        val handle = adapter(adapterId)
        return call { simpleble_adapter_identifier(handle, it) }.consume()
    }

    actual fun adapterAddress(adapterId: Long): String {
        val handle = adapter(adapterId)
        return call { simpleble_adapter_address(handle, it) }.consume()
    }

    actual fun adapterIsPowered(adapterId: Long): Boolean {
        val handle = adapter(adapterId)
        return call { simpleble_adapter_is_powered(handle, it) }
    }

    actual fun adapterPowerOn(adapterId: Long) {
        val handle = adapter(adapterId)
        call { simpleble_adapter_power_on(handle, it) }
    }

    actual fun adapterPowerOff(adapterId: Long) {
        val handle = adapter(adapterId)
        call { simpleble_adapter_power_off(handle, it) }
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
        return internPeripheralList(adapterId) { list, error ->
            simpleble_adapter_scan_get_results(handle, list, error)
        }
    }

    actual fun adapterGetPairedPeripherals(adapterId: Long): LongArray {
        val handle = adapter(adapterId)
        return internPeripheralList(adapterId) { list, error ->
            simpleble_adapter_get_paired_peripherals(handle, list, error)
        }
    }

    actual fun adapterGetConnectedPeripherals(adapterId: Long): LongArray {
        val handle = adapter(adapterId)
        return internPeripheralList(adapterId) { list, error ->
            simpleble_adapter_get_connected_peripherals(handle, list, error)
        }
    }

    actual fun adapterRetrieveCachedPeripherals(adapterId: Long, identifiers: Array<String>): LongArray {
        val handle = adapter(adapterId)
        return memScoped {
            val values = allocArrayOf(identifiers.map { it.cstr.ptr })
            val size = identifiers.size.toULong()
            internPeripherals(adapterId) { count, error ->
                simplektble_adapter_retrieve_cached_peripherals(handle, values, size, count, error)
            }
        }
    }

    // NOTE: Only BlueZ has these Linux pairing calls, so every Apple adapter reports them as unsupported.
    actual fun adapterSetPasskeyRequestHandler(adapterId: Long, enabled: Boolean) {
        adapter(adapterId)
        throw unsupported()
    }

    actual fun adapterSetPasskeyDisplayHandler(adapterId: Long, enabled: Boolean) {
        adapter(adapterId)
        throw unsupported()
    }

    actual fun adapterSetNumericComparisonHandler(adapterId: Long, enabled: Boolean) {
        adapter(adapterId)
        throw unsupported()
    }

    actual fun adapterSetPairingAuthorizationHandler(adapterId: Long, enabled: Boolean) {
        adapter(adapterId)
        throw unsupported()
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
        return readList(
            { list, error -> simpleble_peripheral_services(handle, list, error) },
            { simpleble_service_list_release(it) }
        ) { list, index ->
            memScoped {
                val service = alloc<simpleble_service_t>()
                memset(service.ptr, 0, sizeOf<simpleble_service_t>().convert())
                call { simpleble_service_list_get(list, index.convert(), service.ptr, it) }
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
                    Service(
                        service.uuid.value.toKString(),
                        service.data.toByteArray(service.data_length.toInt()),
                        characteristics
                    )
                } finally {
                    simpleble_service_release(service.ptr)
                }
            }
        }
    }

    actual fun peripheralManufacturerData(adapterId: Long, peripheralId: Long): Map<Int, ByteArray> {
        val handle = peripheral(peripheralId)
        return readList(
            { list, error -> simpleble_peripheral_manufacturer_data(handle, list, error) },
            { simpleble_manufacturer_data_list_release(it) }
        ) { list, index ->
            memScoped {
                val data = alloc<simpleble_manufacturer_data_t>()
                memset(data.ptr, 0, sizeOf<simpleble_manufacturer_data_t>().convert())
                call { simpleble_manufacturer_data_list_get(list, index.convert(), data.ptr, it) }
                try {
                    data.manufacturer_id.toInt() to data.data.toByteArray(data.data_length.toInt())
                } finally {
                    simpleble_manufacturer_data_release(data.ptr)
                }
            }
        }.toMap()
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


    actual fun adapterCreateLocalPeripheral(adapterId: Long): Long {
        val handle = adapter(adapterId)
        val local = call { simpleble_adapter_create_local_peripheral(handle, it) }
            ?: throw unclassified("SimpleBLE returned no local peripheral")
        return addLocal(localPeripherals, local)
    }

    actual fun localPeripheralRegister(peripheralId: Long, callback: LocalPeripheralCallback) {
        val handle = localPeripheral(peripheralId)
        synchronized(lock) { localPeripheralCallbacks[peripheralId] = callback }
        val userdata = peripheralId.toCPointer<CPointed>()
        simpleble_local_peripheral_set_callback_on_client_connected(handle, onClientConnected, userdata)
        simpleble_local_peripheral_set_callback_on_client_disconnected(handle, onClientDisconnected, userdata)
    }

    actual fun localPeripheralRelease(peripheralId: Long) {
        val handle = synchronized(lock) {
            localPeripheralCallbacks.remove(peripheralId)
            localPeripherals.remove(peripheralId)
        } ?: return
        simpleble_local_peripheral_release_handle(handle)
    }

    actual fun localPeripheralAddAdvertisedService(peripheralId: Long, serviceUuids: Array<String>) {
        val handle = localPeripheral(peripheralId)
        for (serviceUuid in serviceUuids) {
            call { simpleble_local_peripheral_add_advertised_service(handle, uuid(serviceUuid), it) }
        }
    }

    actual fun localPeripheralSetAdvertisedManufacturerData(peripheralId: Long, companyId: Int, data: ByteArray) {
        val handle = localPeripheral(peripheralId)
        data.withPointer { pointer, length ->
            call {
                simpleble_local_peripheral_set_advertised_manufacturer_data(
                    handle, companyId.toUShort(), pointer, length, it
                )
            }
        }
    }

    actual fun localPeripheralSetConnectable(peripheralId: Long, connectable: Boolean) {
        val handle = localPeripheral(peripheralId)
        call { simpleble_local_peripheral_set_connectable(handle, connectable, it) }
    }

    actual fun localPeripheralSetAdvertisementLocalName(peripheralId: Long, name: String?) {
        val handle = localPeripheral(peripheralId)
        call { simplektble_local_peripheral_set_advertisement_local_name(handle, name, it) }
    }

    actual fun localPeripheralAddService(peripheralId: Long, uuid: String): Long {
        val handle = localPeripheral(peripheralId)
        return internLocalService(call { simpleble_local_peripheral_add_service(handle, uuid(uuid), it) })
    }

    actual fun localPeripheralServices(peripheralId: Long): LongArray {
        val handle = localPeripheral(peripheralId)
        return readList(
            { list, error -> simpleble_local_peripheral_services(handle, list, error) },
            { simpleble_local_service_list_release(it) }
        ) { list, index ->
            internLocalService(call { simpleble_local_service_list_get(list, index.convert(), it) })
        }.toLongArray()
    }

    actual fun localPeripheralRemoveAllServices(peripheralId: Long) {
        val handle = localPeripheral(peripheralId)
        call { simpleble_local_peripheral_remove_all_services(handle, it) }
    }

    actual fun localPeripheralStart(peripheralId: Long) {
        val handle = localPeripheral(peripheralId)
        call { simpleble_local_peripheral_start(handle, it) }
    }

    actual fun localPeripheralStop(peripheralId: Long) {
        val handle = localPeripheral(peripheralId)
        call { simpleble_local_peripheral_stop(handle, it) }
    }

    actual fun localPeripheralIsStarted(peripheralId: Long): Boolean {
        val handle = localPeripheral(peripheralId)
        return call { simpleble_local_peripheral_is_started(handle, it) }
    }

    actual fun localPeripheralIsAdvertising(peripheralId: Long): Boolean {
        val handle = localPeripheral(peripheralId)
        return call { simpleble_local_peripheral_is_advertising(handle, it) }
    }

    actual fun localServiceRelease(serviceId: Long) {
        val handle = synchronized(lock) {
            localServices.remove(serviceId)?.also { ids.remove(simpleble_local_service_identity(it).toLong()) }
        } ?: return
        simpleble_local_service_release_handle(handle)
    }

    actual fun localServiceUuid(serviceId: Long): String {
        val handle = local(localServices, serviceId)
        return memScoped {
            val uuid = alloc<simpleble_uuid_t>()
            call { simpleble_local_service_uuid(handle, uuid.ptr, it) }
            uuid.value.toKString()
        }
    }

    actual fun localServiceAddCharacteristic(serviceId: Long, uuid: String, capabilities: Int): Long {
        val handle = local(localServices, serviceId)
        return internLocalCharacteristic(call {
            simpleble_local_service_add_characteristic(handle, uuid(uuid), capabilities.toUInt(), it)
        })
    }

    actual fun localServiceCharacteristics(serviceId: Long): LongArray {
        val handle = local(localServices, serviceId)
        return readList(
            { list, error -> simpleble_local_service_characteristics(handle, list, error) },
            { simpleble_local_characteristic_list_release(it) }
        ) { list, index ->
            internLocalCharacteristic(call { simpleble_local_characteristic_list_get(list, index.convert(), it) })
        }.toLongArray()
    }

    actual fun localCharacteristicRegister(characteristicId: Long, callback: LocalCharacteristicCallback) {
        val handle = localCharacteristic(characteristicId)
        synchronized(lock) { localCharacteristicCallbacks[characteristicId] = callback }
        val userdata = characteristicId.toCPointer<CPointed>()
        simpleble_local_characteristic_set_callback_on_write(handle, onLocalWrite, userdata)
        simpleble_local_characteristic_set_callback_on_subscribed(handle, onLocalSubscribed, userdata)
        simpleble_local_characteristic_set_callback_on_unsubscribed(handle, onLocalUnsubscribed, userdata)
    }

    actual fun localCharacteristicRelease(characteristicId: Long) {
        val (handle, buffer) = synchronized(lock) {
            localCharacteristicCallbacks.remove(characteristicId)
            val handle = localCharacteristics.remove(characteristicId)
            handle?.let { ids.remove(simpleble_local_characteristic_identity(it).toLong()) }
            handle to readBuffers.remove(characteristicId)
        }
        if (handle != null) simpleble_local_characteristic_release_handle(handle)
        buffer?.let(nativeHeap::free)
    }

    actual fun localCharacteristicUuid(characteristicId: Long): String {
        val handle = localCharacteristic(characteristicId)
        return memScoped {
            val uuid = alloc<simpleble_uuid_t>()
            call { simpleble_local_characteristic_uuid(handle, uuid.ptr, it) }
            uuid.value.toKString()
        }
    }

    actual fun localCharacteristicCapabilities(characteristicId: Long): Int {
        val handle = localCharacteristic(characteristicId)
        return call { simpleble_local_characteristic_capabilities(handle, it) }.toInt()
    }

    actual fun localCharacteristicValue(characteristicId: Long): ByteArray {
        val handle = localCharacteristic(characteristicId)
        return memScoped {
            val length = alloc<size_tVar>()
            val data = call { simpleble_local_characteristic_value(handle, length.ptr, it) }
            try {
                data.toByteArray(length.value.toInt())
            } finally {
                simpleble_free(data)
            }
        }
    }

    actual fun localCharacteristicSetValue(characteristicId: Long, value: ByteArray) {
        val handle = localCharacteristic(characteristicId)
        value.withPointer { pointer, length ->
            call { simpleble_local_characteristic_set_value(handle, pointer, length, it) }
        }
    }

    actual fun localCharacteristicSetReadHandler(characteristicId: Long, enabled: Boolean) {
        val handle = localCharacteristic(characteristicId)
        simpleble_local_characteristic_set_callback_on_read(
            handle, if (enabled) onLocalRead else null, characteristicId.toCPointer<CPointed>()
        )
    }

    actual fun peripheralSetPasskeyRequestHandler(adapterId: Long, peripheralId: Long, enabled: Boolean) {
        val handle = peripheral(peripheralId)
        call {
            simpleble_advanced_dongl_set_passkey_request_callback(
                handle, if (enabled) onPasskeyRequest else null, peripheralId.toCPointer<CPointed>(), it
            )
        }
    }

    actual fun peripheralSetPasskeyDisplayHandler(adapterId: Long, peripheralId: Long, enabled: Boolean) {
        val handle = peripheral(peripheralId)
        call {
            simpleble_advanced_dongl_set_passkey_display_callback(
                handle, if (enabled) onPasskeyDisplay else null, peripheralId.toCPointer<CPointed>(), it
            )
        }
    }

    actual fun peripheralSetNumericComparisonHandler(adapterId: Long, peripheralId: Long, enabled: Boolean) {
        val handle = peripheral(peripheralId)
        call {
            simpleble_advanced_dongl_set_numeric_comparison_callback(
                handle, if (enabled) onNumericComparison else null, peripheralId.toCPointer<CPointed>(), it
            )
        }
    }

    actual fun peripheralSecureConnection(adapterId: Long, peripheralId: Long, mitm: Boolean) {
        val handle = peripheral(peripheralId)
        call { simpleble_advanced_dongl_secure_connection(handle, mitm, it) }
    }

    actual fun getBackends(): List<String> = readList(
        { list, error -> simpleble_backend_get_backends(list, error) },
        { simpleble_backend_list_release(it) }
    ) { list, index -> internBackend(call { simpleble_backend_list_get(list, index.convert(), it) }) }

    actual fun getNativeBackend(): String = internBackend(call { simpleble_backend_get_native(it) })

    actual fun getDonglBackend(): String = internBackend(call { simpleble_backend_get_dongl(it) })

    actual fun getSimulatorBackend(): String = internBackend(call { simpleble_backend_get_simulator(it) })

    actual fun backendIsBluetoothEnabled(backend: String): Boolean {
        val handle = backend(backend)
        return call { simpleble_backend_is_bluetooth_enabled(handle, it) }
    }

    actual fun backendGetAdapters(backend: String): LongArray {
        val handle = backend(backend)
        return readList(
            { list, error -> simpleble_backend_get_adapters(handle, list, error) },
            { simpleble_adapter_list_release(it) }
        ) { list, index -> internAdapter(call { simpleble_adapter_list_get(list, index.convert(), it) }) }.toLongArray()
    }

    // The keys and groups match ConfigKey and ConfigGroup in Config.kt.
    actual fun configReset(group: Int) {
        when (group) {
            ConfigGroup.ALL -> simpleble_config_reset_all()
            ConfigGroup.SIMPLEBLUEZ -> simpleble_config_simplebluez_reset()
            ConfigGroup.WINRT -> simpleble_config_winrt_reset()
            ConfigGroup.COREBLUETOOTH -> simpleble_config_corebluetooth_reset()
            ConfigGroup.DONGL -> simpleble_config_dongl_reset()
            ConfigGroup.ANDROID -> simpleble_config_android_reset()
            else -> throw unclassified("Unknown settings group")
        }
    }

    actual fun configGetBoolean(key: Int): Boolean = when (key) {
        ConfigKey.SIMPLEBLUEZ_USE_SYSTEM_BUS -> simpleble_config_simplebluez_get_use_system_bus()
        ConfigKey.SIMPLEBLUEZ_INTERACTIVE_PAIRING -> simpleble_config_simplebluez_get_interactive_pairing()
        ConfigKey.WINRT_REINITIALIZE_APARTMENT_ON_MAIN_THREAD ->
            simpleble_config_winrt_get_experimental_reinitialize_winrt_apartment_on_main_thread()
        ConfigKey.WINRT_USE_DEFERRED_DISCONNECT -> simpleble_config_winrt_get_use_deferred_disconnect()
        ConfigKey.DONGL_USE_DONGL_BACKEND -> simpleble_config_dongl_get_use_dongl_backend()
        ConfigKey.DONGL_AUTO_UPDATE -> simpleble_config_dongl_get_auto_update()
        ConfigKey.DONGL_FORCE_UPDATE -> simpleble_config_dongl_get_force_update()
        ConfigKey.ANDROID_NEGOTIATE_MTU -> simpleble_config_android_get_negotiate_mtu()
        else -> throw unclassified("Unknown boolean setting")
    }

    actual fun configSetBoolean(key: Int, value: Boolean) {
        when (key) {
            ConfigKey.SIMPLEBLUEZ_USE_SYSTEM_BUS -> simpleble_config_simplebluez_set_use_system_bus(value)
            ConfigKey.SIMPLEBLUEZ_INTERACTIVE_PAIRING -> simpleble_config_simplebluez_set_interactive_pairing(value)
            ConfigKey.WINRT_REINITIALIZE_APARTMENT_ON_MAIN_THREAD ->
                simpleble_config_winrt_set_experimental_reinitialize_winrt_apartment_on_main_thread(value)
            ConfigKey.WINRT_USE_DEFERRED_DISCONNECT -> simpleble_config_winrt_set_use_deferred_disconnect(value)
            ConfigKey.DONGL_USE_DONGL_BACKEND -> simpleble_config_dongl_set_use_dongl_backend(value)
            ConfigKey.DONGL_AUTO_UPDATE -> simpleble_config_dongl_set_auto_update(value)
            ConfigKey.DONGL_FORCE_UPDATE -> simpleble_config_dongl_set_force_update(value)
            ConfigKey.ANDROID_NEGOTIATE_MTU -> simpleble_config_android_set_negotiate_mtu(value)
            else -> throw unclassified("Unknown boolean setting")
        }
    }

    actual fun configGetInt(key: Int): Int = when (key) {
        ConfigKey.ANDROID_CONNECTION_PRIORITY -> simpleble_config_android_get_connection_priority()
        else -> throw unclassified("Unknown integer setting")
    }

    actual fun configSetInt(key: Int, value: Int) {
        when (key) {
            ConfigKey.ANDROID_CONNECTION_PRIORITY -> simpleble_config_android_set_connection_priority(value)
            else -> throw unclassified("Unknown integer setting")
        }
    }

    actual fun configGetMilliseconds(key: Int): Long = when (key) {
        ConfigKey.SIMPLEBLUEZ_CONNECTION_TIMEOUT -> simpleble_config_simplebluez_get_connection_timeout_ms()
        ConfigKey.SIMPLEBLUEZ_DISCONNECTION_TIMEOUT -> simpleble_config_simplebluez_get_disconnection_timeout_ms()
        else -> throw unclassified("Unknown duration setting")
    }

    actual fun configSetMilliseconds(key: Int, value: Long) {
        when (key) {
            ConfigKey.SIMPLEBLUEZ_CONNECTION_TIMEOUT -> simpleble_config_simplebluez_set_connection_timeout_ms(value)
            ConfigKey.SIMPLEBLUEZ_DISCONNECTION_TIMEOUT ->
                simpleble_config_simplebluez_set_disconnection_timeout_ms(value)
            else -> throw unclassified("Unknown duration setting")
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
                val message = "$kind collector is already active for characteristic $characteristic"
                throw SimpleKtBleException(ErrorCode.OPERATION_FAILED, message)
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

    private fun backend(identifier: String): simpleble_backend_t {
        return synchronized(lock) { backends[identifier] } ?: throw unclassified("Unknown backend")
    }

    private fun internBackend(handle: simpleble_backend_t?): String {
        if (handle == null) throw unclassified("SimpleBLE returned no backend")
        val identifier = call { simpleble_backend_identifier(handle, it) }.consume()
        val known = synchronized(lock) { backends.getOrPut(identifier) { handle } != handle }
        if (known) simpleble_backend_release_handle(handle)
        return identifier
    }

    /** Pairing runs on a SimpleBLE thread, which waits for the answer. */
    internal fun peripheralCallback(peripheralId: Long): PeripheralCallback? {
        return synchronized(lock) { peripheralCallbacks[peripheralId] }
    }

    private fun adapter(adapterId: Long): simpleble_adapter_t {
        return synchronized(lock) { adapters[adapterId] } ?: throw released()
    }

    private fun peripheral(peripheralId: Long): simpleble_peripheral_t {
        return synchronized(lock) { peripherals[peripheralId]?.handle } ?: throw released()
    }

    /**
     * Returns the ID of the object with [identity], and whether it already had one. A new ID is passed to [store] under
     * the lock.
     */
    private inline fun assignId(identity: Long, store: (Long) -> Unit): Pair<Long, Boolean> = synchronized(lock) {
        val known = ids[identity]
        if (known != null) return@synchronized known to true
        val id = nextId++
        ids[identity] = id
        store(id)
        id to false
    }

    private fun internAdapter(handle: simpleble_adapter_t?): Long {
        if (handle == null) throw unclassified("SimpleBLE returned no adapter")
        val (id, known) = assignId(simpleble_adapter_identity(handle).toLong()) { adapters[it] = handle }
        if (known) simpleble_adapter_release_handle(handle)
        return id
    }

    internal fun internPeripheral(adapterId: Long, handle: simpleble_peripheral_t?): Long {
        if (handle == null) throw unclassified("SimpleBLE returned no peripheral")
        val identity = simpleble_peripheral_identity(handle).toLong()
        val (id, known) = assignId(identity) { peripherals[it] = PeripheralEntry(adapterId, handle) }
        if (known) simpleble_peripheral_release_handle(handle)
        return id
    }

    /** Interns every peripheral in a list that one SimpleCBLE call read. */
    private inline fun internPeripheralList(
        adapterId: Long,
        read: (CPointer<COpaquePointerVar>, CPointer<CPointerVar<simpleble_error_t>>) -> ULong
    ): LongArray = readList(read, { simpleble_peripheral_list_release(it) }) { list, index ->
        internPeripheral(adapterId, call { simpleble_peripheral_list_get(list, index.convert(), it) })
    }.toLongArray()

    /** Interns every handle in an array from simplektble/snapshot.h, and frees the array. */
    private inline fun internPeripherals(
        adapterId: Long,
        snapshot: (CPointer<size_tVar>, CPointer<CPointerVar<simpleble_error_t>>) -> CPointer<COpaquePointerVar>?
    ): LongArray = memScoped {
        val count = alloc<size_tVar>()
        val handles = call { snapshot(count.ptr, it) }
        try {
            LongArray(count.value.toInt()) { index -> internPeripheral(adapterId, handles!![index]) }
        } finally {
            simpleble_free(handles)
        }
    }

    // NOTE: The callback is looked up when the event runs, so a released adapter's queued events are dropped, as on
    // the JNI side.
    internal fun onAdapterEvent(adapterId: Long, event: (AdapterCallback) -> Unit) {
        dispatch { synchronized(lock) { adapterCallbacks[adapterId] }?.let(event) }
    }

    internal fun onPeripheralEvent(peripheralId: Long, event: (PeripheralCallback) -> Unit) {
        dispatch { synchronized(lock) { peripheralCallbacks[peripheralId] }?.let(event) }
    }

    // NOTE: The disconnection ends the subscriptions it had, and only those, after the values queued before it.
    internal fun onPeripheralDisconnected(peripheralId: Long) {
        val (ended, callback) = synchronized(lock) {
            val keys = subscriptions.keys.filter { it.peripheralId == peripheralId }
            keys.mapNotNull { key -> subscriptions.remove(key)?.let(dataCallbacks::remove) } to
                peripheralCallbacks[peripheralId]
        }
        dispatch {
            ended.forEach(DataCallback::onEnded)
            callback?.onDisconnected()
        }
    }

    private fun <T> addLocal(objects: MutableMap<Long, T>, handle: T): Long {
        return synchronized(lock) { (nextId++).also { objects[it] = handle } }
    }

    private fun internLocalService(handle: simpleble_local_service_t?): Long {
        if (handle == null) throw unclassified("SimpleBLE returned no local service")
        val (id, known) = assignId(simpleble_local_service_identity(handle).toLong()) { localServices[it] = handle }
        if (known) simpleble_local_service_release_handle(handle)
        return id
    }

    private fun internLocalCharacteristic(handle: simpleble_local_characteristic_t?): Long {
        if (handle == null) throw unclassified("SimpleBLE returned no local characteristic")
        val identity = simpleble_local_characteristic_identity(handle).toLong()
        val (id, known) = assignId(identity) { localCharacteristics[it] = handle }
        if (known) simpleble_local_characteristic_release_handle(handle)
        return id
    }

    private fun <T : Any> local(objects: Map<Long, T>, id: Long): T {
        return synchronized(lock) { objects[id] } ?: throw released()
    }

    private fun localPeripheral(peripheralId: Long) = local(localPeripherals, peripheralId)

    private fun localCharacteristic(characteristicId: Long) = local(localCharacteristics, characteristicId)

    internal fun onLocalPeripheralEvent(peripheralId: Long, event: (LocalPeripheralCallback) -> Unit) {
        val callback = synchronized(lock) { localPeripheralCallbacks[peripheralId] } ?: return
        dispatch { event(callback) }
    }

    internal fun onLocalCharacteristicEvent(characteristicId: Long, event: (LocalCharacteristicCallback) -> Unit) {
        val callback = synchronized(lock) { localCharacteristicCallbacks[characteristicId] } ?: return
        dispatch { event(callback) }
    }

    /**
     * Runs on the SimpleBLE thread that serves the read, which waits for the value.
     * If the handler fails or returns null, the stored value is served instead, as on the JNI side.
     */
    internal fun onLocalRead(characteristicId: Long, length: CPointer<size_tVar>?): CPointer<UByteVar>? {
        length?.pointed?.value = 0u
        val callback = synchronized(lock) { localCharacteristicCallbacks[characteristicId] }
        val value = try {
            callback?.onRead()
        } catch (exception: Exception) {
            logError("onLocalRead", "Read handler failed: $exception")
            null
        } ?: localCharacteristicValue(characteristicId)
        val buffer = nativeHeap.allocArray<UByteVar>(maxOf(value.size, 1))
        value.forEachIndexed { index, byte -> buffer[index] = byte.toUByte() }
        synchronized(lock) { readBuffers.put(characteristicId, buffer) }?.let(nativeHeap::free)
        length?.pointed?.value = value.size.toULong()
        return buffer
    }

    internal fun onDataEvent(subscriptionId: Long, data: ByteArray) {
        val callback = synchronized(lock) { dataCallbacks[subscriptionId] } ?: return
        dispatch { callback.onDataReceived(data) }
    }
}

private val onPowerOn = staticCFunction { _: simpleble_adapter_t?, userdata: COpaquePointer? ->
    guarded { Native.onAdapterEvent(userdata.toLong()) { it.onPowerOn() } }
}

private val onPowerOff = staticCFunction { _: simpleble_adapter_t?, userdata: COpaquePointer? ->
    guarded { Native.onAdapterEvent(userdata.toLong()) { it.onPowerOff() } }
}

private val onScanStart = staticCFunction { _: simpleble_adapter_t?, userdata: COpaquePointer? ->
    guarded { Native.onAdapterEvent(userdata.toLong()) { it.onScanStart() } }
}

private val onScanStop = staticCFunction { _: simpleble_adapter_t?, userdata: COpaquePointer? ->
    guarded { Native.onAdapterEvent(userdata.toLong()) { it.onScanStop() } }
}

private val onScanUpdated = staticCFunction { _: simpleble_adapter_t?, peripheral: simpleble_peripheral_t?, userdata: COpaquePointer? ->
    guarded {
        val adapterId = userdata.toLong()
        val peripheralId = Native.internPeripheral(adapterId, peripheral)
        Native.onAdapterEvent(adapterId) { it.onScanUpdated(peripheralId) }
    }
}

private val onScanFound = staticCFunction { _: simpleble_adapter_t?, peripheral: simpleble_peripheral_t?, userdata: COpaquePointer? ->
    guarded {
        val adapterId = userdata.toLong()
        val peripheralId = Native.internPeripheral(adapterId, peripheral)
        Native.onAdapterEvent(adapterId) { it.onScanFound(peripheralId) }
    }
}

private val onConnected = staticCFunction { _: simpleble_peripheral_t?, userdata: COpaquePointer? ->
    guarded { Native.onPeripheralEvent(userdata.toLong()) { it.onConnected() } }
}

private val onDisconnected = staticCFunction { _: simpleble_peripheral_t?, userdata: COpaquePointer? ->
    guarded { Native.onPeripheralDisconnected(userdata.toLong()) }
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

private val onClientConnected = staticCFunction { _: simpleble_local_peripheral_t?, address: CPointer<ByteVar>?, userdata: COpaquePointer? ->
    guarded {
        val client = address?.toKString() ?: ""
        Native.onLocalPeripheralEvent(userdata.toLong()) { it.onClientConnected(client) }
    }
}

private val onClientDisconnected = staticCFunction { _: simpleble_local_peripheral_t?, address: CPointer<ByteVar>?, userdata: COpaquePointer? ->
    guarded {
        val client = address?.toKString() ?: ""
        Native.onLocalPeripheralEvent(userdata.toLong()) { it.onClientDisconnected(client) }
    }
}

private val onLocalRead = staticCFunction { _: simpleble_local_characteristic_t?, length: CPointer<size_tVar>?, userdata: COpaquePointer? ->
    try {
        Native.onLocalRead(userdata.toLong(), length)
    } catch (exception: Throwable) {
        logError("onLocalRead", "Read handler failed: $exception")
        null
    }
}

private val onLocalWrite = staticCFunction { _: simpleble_local_characteristic_t?, data: CPointer<UByteVar>?, length: ULong, userdata: COpaquePointer? ->
    guarded {
        val value = data.toByteArray(length.toInt())
        Native.onLocalCharacteristicEvent(userdata.toLong()) { it.onWrite(value) }
    }
}

private val onLocalSubscribed = staticCFunction { _: simpleble_local_characteristic_t?, userdata: COpaquePointer? ->
    guarded { Native.onLocalCharacteristicEvent(userdata.toLong()) { it.onSubscribed() } }
}

private val onLocalUnsubscribed = staticCFunction { _: simpleble_local_characteristic_t?, userdata: COpaquePointer? ->
    guarded { Native.onLocalCharacteristicEvent(userdata.toLong()) { it.onUnsubscribed() } }
}

private val onPasskeyRequest = staticCFunction { _: simpleble_peripheral_t?, passkey: CPointer<ByteVar>?, userdata: COpaquePointer? ->
    try {
        val digits = Native.peripheralCallback(userdata.toLong())?.onPasskeyRequest()?.encodeToByteArray()
        // SimpleBLE validates the six digits, and rejects pairing when this returns false.
        if (digits == null || passkey == null || digits.size != 6) {
            false
        } else {
            digits.forEachIndexed { index, digit -> passkey[index] = digit }
            passkey[6] = 0
            true
        }
    } catch (exception: Throwable) {
        logError("onPasskeyRequest", "Passkey request handler failed: $exception")
        false
    }
}

private val onPasskeyDisplay = staticCFunction { _: simpleble_peripheral_t?, passkey: CPointer<ByteVar>?, userdata: COpaquePointer? ->
    guarded { Native.peripheralCallback(userdata.toLong())?.onPasskeyDisplay(passkey?.toKString() ?: "") }
}

private val onNumericComparison = staticCFunction { _: simpleble_peripheral_t?, passkey: CPointer<ByteVar>?, userdata: COpaquePointer? ->
    try {
        Native.peripheralCallback(userdata.toLong())?.onNumericComparison(passkey?.toKString() ?: "") ?: false
    } catch (exception: Throwable) {
        logError("onNumericComparison", "Numeric comparison handler failed: $exception")
        false
    }
}

/** Logs through SimpleBLE's logger, where the JNI layer logs its callback failures. */
private fun logError(function: String, message: String) {
    simplektble_log_error(function, message)
}

/** Exceptions must not cross back into C. */
private inline fun guarded(block: () -> Unit) {
    try {
        block()
    } catch (exception: Throwable) {
        logError("guarded", "Native callback failed: $exception")
    }
}

/** Runs a SimpleCBLE call and throws its error, if any, as a [SimpleKtBleException]. */
private inline fun <T> call(block: (CPointer<CPointerVar<simpleble_error_t>>) -> T): T = memScoped {
    val error = alloc<CPointerVar<simpleble_error_t>>()
    error.value = null
    val result = block(error.ptr)
    error.value?.let { raised ->
        val code = ErrorCode.fromValue(simpleble_error_code(raised).toInt())
        val message = simpleble_error_message(raised)?.toKString() ?: "Unknown native error"
        simpleble_error_release(error.ptr)
        throw SimpleKtBleException(code, message)
    }
    result
}

/** Reads a SimpleCBLE list once, maps each entry, and releases the list. */
private inline fun <T> readList(
    read: (CPointer<COpaquePointerVar>, CPointer<CPointerVar<simpleble_error_t>>) -> ULong,
    release: (COpaquePointer?) -> Unit,
    entry: (COpaquePointer?, Int) -> T
): List<T> = memScoped {
    val list = alloc<COpaquePointerVar>()
    val count = call { read(list.ptr, it) }.toInt()
    try {
        List(count) { index -> entry(list.value, index) }
    } finally {
        release(list.value)
    }
}

/** A failure in this bridge, which the JNI side raises as a plain C++ exception. */
private fun unclassified(message: String) = SimpleKtBleException(ErrorCode.UNCLASSIFIED_EXCEPTION, message)

/** A call on an object that was released, which the JNI side raises as SimpleBLE's NotInitialized. */
private fun released() = SimpleKtBleException(ErrorCode.OBJECT_NOT_INITIALIZED, "Object has not been initialized.")

/** A call no Apple backend has, which the JNI side raises as SimpleBLE's OperationNotSupported. */
private fun unsupported() =
    SimpleKtBleException(ErrorCode.OPERATION_NOT_SUPPORTED, "The requested operation is not supported.")

private fun uuid(value: String): CValue<simpleble_uuid_t> {
    val bytes = value.encodeToByteArray()
    if (bytes.size >= SIMPLEBLE_UUID_STR_LEN) {
        throw SimpleKtBleException(ErrorCode.INVALID_ARGUMENT, "Invalid UUID: $value")
    }
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

private inline fun ByteArray.withPointer(block: (CPointer<UByteVar>, ULong) -> Unit) {
    // NOTE: SimpleCBLE's writes reject a NULL pointer even when the length is zero.
    val bytes = if (isEmpty()) ByteArray(1) else this
    bytes.usePinned { pinned -> block(pinned.addressOf(0).reinterpret(), size.toULong()) }
}
