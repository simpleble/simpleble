package org.simplektble

internal interface AdapterCallback {
    fun onPowerOn()
    fun onPowerOff()
    fun onScanStart()
    fun onScanStop()
    fun onScanUpdated(peripheralId: Long)
    fun onScanFound(peripheralId: Long)
}

internal interface PeripheralCallback {
    fun onConnected()
    fun onDisconnected()

    /** The Dongl pairing callbacks run on a SimpleBLE thread, which waits for the answer. */
    fun onPasskeyRequest(): String?
    fun onPasskeyDisplay(passkey: String)
    fun onNumericComparison(passkey: String): Boolean
}

internal interface DataCallback {
    fun onDataReceived(data: ByteArray)

    /** The peripheral disconnected, which ended the subscription. Runs after every value received before it. */
    fun onEnded()
}

internal interface LocalPeripheralCallback {
    fun onClientConnected(address: String)
    fun onClientDisconnected(address: String)
}

internal interface LocalCharacteristicCallback {
    /** Runs on a SimpleBLE thread, which waits for the value. Returns null to serve the stored value. */
    fun onRead(): ByteArray?
    fun onWrite(value: ByteArray)
    fun onSubscribed()
    fun onUnsubscribed()
}

/**
 * The calls into SimpleBLE behind the public classes, implemented once per platform.
 *
 * Adapters, peripherals and local GATT objects are named by IDs the native side assigns, and a peripheral ID only has
 * meaning together with its adapter ID. Failures are thrown as [SimpleKtBleException].
 */
internal expect object Native {
    fun version(): String

    fun isBluetoothEnabled(): Boolean

    fun getAdapters(): LongArray

    fun adapterRegister(adapterId: Long, callback: AdapterCallback)

    /** Releases an adapter and its peripherals, and ends their subscriptions. Their IDs then name nothing. */
    fun adapterRelease(adapterId: Long)

    fun adapterIdentifier(adapterId: Long): String

    fun adapterAddress(adapterId: Long): String

    fun adapterIsPowered(adapterId: Long): Boolean

    fun adapterPowerOn(adapterId: Long)

    fun adapterPowerOff(adapterId: Long)

    fun adapterScanStart(adapterId: Long)

    fun adapterScanStop(adapterId: Long)

    fun adapterScanFor(adapterId: Long, timeoutMs: Int)

    fun adapterScanIsActive(adapterId: Long): Boolean

    fun adapterScanGetResults(adapterId: Long): LongArray

    fun adapterGetPairedPeripherals(adapterId: Long): LongArray

    fun adapterGetConnectedPeripherals(adapterId: Long): LongArray

    fun adapterRetrieveCachedPeripherals(adapterId: Long, identifiers: Array<String>): LongArray

    fun peripheralRegister(adapterId: Long, peripheralId: Long, callback: PeripheralCallback)

    fun peripheralIdentifier(adapterId: Long, peripheralId: Long): String

    fun peripheralAddress(adapterId: Long, peripheralId: Long): String

    fun peripheralAddressType(adapterId: Long, peripheralId: Long): Int

    fun peripheralRssi(adapterId: Long, peripheralId: Long): Int

    fun peripheralTxPower(adapterId: Long, peripheralId: Long): Int

    fun peripheralMtu(adapterId: Long, peripheralId: Long): Int

    fun peripheralConnect(adapterId: Long, peripheralId: Long)

    fun peripheralDisconnect(adapterId: Long, peripheralId: Long)

    fun peripheralIsConnected(adapterId: Long, peripheralId: Long): Boolean

    fun peripheralIsConnectable(adapterId: Long, peripheralId: Long): Boolean

    fun peripheralIsPaired(adapterId: Long, peripheralId: Long): Boolean

    fun peripheralUnpair(adapterId: Long, peripheralId: Long)

    fun peripheralServices(adapterId: Long, peripheralId: Long): List<Service>

    fun peripheralManufacturerData(adapterId: Long, peripheralId: Long): Map<Int, ByteArray>

    fun peripheralRead(adapterId: Long, peripheralId: Long, service: String, characteristic: String): ByteArray

    fun peripheralWriteRequest(
        adapterId: Long,
        peripheralId: Long,
        service: String,
        characteristic: String,
        data: ByteArray
    )

    fun peripheralWriteCommand(
        adapterId: Long,
        peripheralId: Long,
        service: String,
        characteristic: String,
        data: ByteArray
    )

    fun peripheralNotify(
        adapterId: Long,
        peripheralId: Long,
        service: String,
        characteristic: String,
        callback: DataCallback
    )

    fun peripheralIndicate(
        adapterId: Long,
        peripheralId: Long,
        service: String,
        characteristic: String,
        callback: DataCallback
    )

    fun peripheralUnsubscribe(adapterId: Long, peripheralId: Long, service: String, characteristic: String)

    fun peripheralDescriptorRead(
        adapterId: Long,
        peripheralId: Long,
        service: String,
        characteristic: String,
        descriptor: String
    ): ByteArray

    fun peripheralDescriptorWrite(
        adapterId: Long,
        peripheralId: Long,
        service: String,
        characteristic: String,
        descriptor: String,
        data: ByteArray
    )

    fun adapterCreateLocalPeripheral(adapterId: Long): Long

    fun localPeripheralRegister(peripheralId: Long, callback: LocalPeripheralCallback)

    fun localPeripheralRelease(peripheralId: Long)

    fun localPeripheralAddAdvertisedService(peripheralId: Long, serviceUuids: Array<String>)

    fun localPeripheralSetAdvertisedManufacturerData(peripheralId: Long, companyId: Int, data: ByteArray)

    fun localPeripheralSetConnectable(peripheralId: Long, connectable: Boolean)

    fun localPeripheralSetAdvertisementLocalName(peripheralId: Long, name: String?)

    fun localPeripheralAddService(peripheralId: Long, uuid: String): Long

    fun localPeripheralServices(peripheralId: Long): LongArray

    fun localPeripheralRemoveAllServices(peripheralId: Long)

    fun localPeripheralStart(peripheralId: Long)

    fun localPeripheralStop(peripheralId: Long)

    fun localPeripheralIsStarted(peripheralId: Long): Boolean

    fun localPeripheralIsAdvertising(peripheralId: Long): Boolean

    fun localServiceRelease(serviceId: Long)

    fun localServiceUuid(serviceId: Long): String

    fun localServiceAddCharacteristic(serviceId: Long, uuid: String, capabilities: Int): Long

    fun localServiceCharacteristics(serviceId: Long): LongArray

    fun localCharacteristicRegister(characteristicId: Long, callback: LocalCharacteristicCallback)

    fun localCharacteristicRelease(characteristicId: Long)

    fun localCharacteristicUuid(characteristicId: Long): String

    fun localCharacteristicCapabilities(characteristicId: Long): Int

    fun localCharacteristicValue(characteristicId: Long): ByteArray

    fun localCharacteristicSetValue(characteristicId: Long, value: ByteArray)

    fun localCharacteristicSetReadHandler(characteristicId: Long, enabled: Boolean)

    fun peripheralSetPasskeyRequestHandler(adapterId: Long, peripheralId: Long, enabled: Boolean)

    fun peripheralSetPasskeyDisplayHandler(adapterId: Long, peripheralId: Long, enabled: Boolean)

    fun peripheralSetNumericComparisonHandler(adapterId: Long, peripheralId: Long, enabled: Boolean)

    fun peripheralSecureConnection(adapterId: Long, peripheralId: Long, mitm: Boolean)

    fun getBackends(): List<String>

    fun getNativeBackend(): String

    fun getDonglBackend(): String

    fun getSimulatorBackend(): String

    fun backendIsBluetoothEnabled(backend: String): Boolean

    fun backendGetAdapters(backend: String): LongArray

    fun configReset(group: Int)

    fun configGetBoolean(key: Int): Boolean

    fun configSetBoolean(key: Int, value: Boolean)

    fun configGetInt(key: Int): Int

    fun configSetInt(key: Int, value: Int)

    fun configGetMilliseconds(key: Int): Long

    fun configSetMilliseconds(key: Int, value: Long)
}
