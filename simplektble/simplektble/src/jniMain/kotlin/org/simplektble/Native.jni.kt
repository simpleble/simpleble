package org.simplektble

/** Loads the JNI library, which each platform ships differently. */
internal expect fun loadNativeLibrary()

/** The Android application context, which hosting needs, or null where there is none. */
internal expect fun platformContext(): Any?

internal actual object Native {
    init {
        loadNativeLibrary()
    }

    actual external fun version(): String

    actual external fun isBluetoothEnabled(): Boolean

    actual external fun getAdapters(): LongArray

    actual external fun adapterRegister(adapterId: Long, callback: AdapterCallback)

    actual external fun adapterRelease(adapterId: Long)

    actual external fun adapterIdentifier(adapterId: Long): String

    actual external fun adapterAddress(adapterId: Long): String

    actual external fun adapterIsPowered(adapterId: Long): Boolean

    actual external fun adapterPowerOn(adapterId: Long)

    actual external fun adapterPowerOff(adapterId: Long)

    actual external fun adapterScanStart(adapterId: Long)

    actual external fun adapterScanStop(adapterId: Long)

    actual external fun adapterScanFor(adapterId: Long, timeoutMs: Int)

    actual external fun adapterScanIsActive(adapterId: Long): Boolean

    actual external fun adapterScanGetResults(adapterId: Long): LongArray

    actual external fun adapterGetPairedPeripherals(adapterId: Long): LongArray

    actual external fun adapterGetConnectedPeripherals(adapterId: Long): LongArray

    actual external fun adapterRetrieveCachedPeripherals(adapterId: Long, identifiers: Array<String>): LongArray

    actual external fun adapterSetPasskeyRequestHandler(adapterId: Long, enabled: Boolean)

    actual external fun adapterSetPasskeyDisplayHandler(adapterId: Long, enabled: Boolean)

    actual external fun adapterSetNumericComparisonHandler(adapterId: Long, enabled: Boolean)

    actual external fun adapterSetPairingAuthorizationHandler(adapterId: Long, enabled: Boolean)

    actual external fun peripheralRegister(adapterId: Long, peripheralId: Long, callback: PeripheralCallback)

    actual external fun peripheralIdentifier(adapterId: Long, peripheralId: Long): String

    actual external fun peripheralAddress(adapterId: Long, peripheralId: Long): String

    actual external fun peripheralAddressType(adapterId: Long, peripheralId: Long): Int

    actual external fun peripheralRssi(adapterId: Long, peripheralId: Long): Int

    actual external fun peripheralTxPower(adapterId: Long, peripheralId: Long): Int

    actual external fun peripheralMtu(adapterId: Long, peripheralId: Long): Int

    actual external fun peripheralConnect(adapterId: Long, peripheralId: Long)

    actual external fun peripheralDisconnect(adapterId: Long, peripheralId: Long)

    actual external fun peripheralIsConnected(adapterId: Long, peripheralId: Long): Boolean

    actual external fun peripheralIsConnectable(adapterId: Long, peripheralId: Long): Boolean

    actual external fun peripheralIsPaired(adapterId: Long, peripheralId: Long): Boolean

    actual external fun peripheralUnpair(adapterId: Long, peripheralId: Long)

    actual external fun peripheralServices(adapterId: Long, peripheralId: Long): List<Service>

    actual external fun peripheralManufacturerData(adapterId: Long, peripheralId: Long): Map<Int, ByteArray>

    actual external fun peripheralRead(adapterId: Long, peripheralId: Long, service: String, characteristic: String): ByteArray

    actual external fun peripheralWriteRequest(
        adapterId: Long,
        peripheralId: Long,
        service: String,
        characteristic: String,
        data: ByteArray
    )

    actual external fun peripheralWriteCommand(
        adapterId: Long,
        peripheralId: Long,
        service: String,
        characteristic: String,
        data: ByteArray
    )

    actual external fun peripheralNotify(
        adapterId: Long,
        peripheralId: Long,
        service: String,
        characteristic: String,
        callback: DataCallback
    )

    actual external fun peripheralIndicate(
        adapterId: Long,
        peripheralId: Long,
        service: String,
        characteristic: String,
        callback: DataCallback
    )

    actual external fun peripheralUnsubscribe(adapterId: Long, peripheralId: Long, service: String, characteristic: String)

    actual external fun peripheralDescriptorRead(
        adapterId: Long,
        peripheralId: Long,
        service: String,
        characteristic: String,
        descriptor: String
    ): ByteArray

    actual external fun peripheralDescriptorWrite(
        adapterId: Long,
        peripheralId: Long,
        service: String,
        characteristic: String,
        descriptor: String,
        data: ByteArray
    )

    actual fun adapterCreateLocalPeripheral(adapterId: Long): Long {
        return adapterCreateLocalPeripheralWithContext(adapterId, platformContext())
    }

    private external fun adapterCreateLocalPeripheralWithContext(adapterId: Long, context: Any?): Long

    actual external fun localPeripheralRegister(peripheralId: Long, callback: LocalPeripheralCallback)

    actual external fun localPeripheralRelease(peripheralId: Long)

    actual external fun localPeripheralAddAdvertisedService(peripheralId: Long, serviceUuids: Array<String>)

    actual external fun localPeripheralSetAdvertisedManufacturerData(peripheralId: Long, companyId: Int, data: ByteArray)

    actual external fun localPeripheralSetConnectable(peripheralId: Long, connectable: Boolean)

    actual external fun localPeripheralSetAdvertisementLocalName(peripheralId: Long, name: String?)

    actual external fun localPeripheralAddService(peripheralId: Long, uuid: String): Long

    actual external fun localPeripheralServices(peripheralId: Long): LongArray

    actual external fun localPeripheralRemoveAllServices(peripheralId: Long)

    actual external fun localPeripheralStart(peripheralId: Long)

    actual external fun localPeripheralStop(peripheralId: Long)

    actual external fun localPeripheralIsStarted(peripheralId: Long): Boolean

    actual external fun localPeripheralIsAdvertising(peripheralId: Long): Boolean

    actual external fun localServiceRelease(serviceId: Long)

    actual external fun localServiceUuid(serviceId: Long): String

    actual external fun localServiceAddCharacteristic(serviceId: Long, uuid: String, capabilities: Int): Long

    actual external fun localServiceCharacteristics(serviceId: Long): LongArray

    actual external fun localCharacteristicRegister(characteristicId: Long, callback: LocalCharacteristicCallback)

    actual external fun localCharacteristicRelease(characteristicId: Long)

    actual external fun localCharacteristicUuid(characteristicId: Long): String

    actual external fun localCharacteristicCapabilities(characteristicId: Long): Int

    actual external fun localCharacteristicValue(characteristicId: Long): ByteArray

    actual external fun localCharacteristicSetValue(characteristicId: Long, value: ByteArray)

    actual external fun localCharacteristicSetReadHandler(characteristicId: Long, enabled: Boolean)

    actual external fun peripheralSetPasskeyRequestHandler(adapterId: Long, peripheralId: Long, enabled: Boolean)

    actual external fun peripheralSetPasskeyDisplayHandler(adapterId: Long, peripheralId: Long, enabled: Boolean)

    actual external fun peripheralSetNumericComparisonHandler(adapterId: Long, peripheralId: Long, enabled: Boolean)

    actual external fun peripheralSecureConnection(adapterId: Long, peripheralId: Long, mitm: Boolean)

    actual external fun getBackends(): List<String>

    actual external fun getNativeBackend(): String

    actual external fun getDonglBackend(): String

    actual external fun getSimulatorBackend(): String

    actual external fun backendIsBluetoothEnabled(backend: String): Boolean

    actual external fun backendGetAdapters(backend: String): LongArray

    actual external fun configReset(group: Int)

    actual external fun configGetBoolean(key: Int): Boolean

    actual external fun configSetBoolean(key: Int, value: Boolean)

    actual external fun configGetInt(key: Int): Int

    actual external fun configSetInt(key: Int, value: Int)

    actual external fun configGetMilliseconds(key: Int): Long

    actual external fun configSetMilliseconds(key: Int, value: Long)
}
