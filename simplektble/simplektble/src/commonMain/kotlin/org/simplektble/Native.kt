package org.simplektble

internal interface AdapterCallback {
    fun onScanStart()
    fun onScanStop()
    fun onScanUpdated(peripheralId: Long)
    fun onScanFound(peripheralId: Long)
}

internal interface PeripheralCallback {
    fun onConnected()
    fun onDisconnected()
}

internal interface DataCallback {
    fun onDataReceived(data: ByteArray)
}

/**
 * The calls into SimpleBLE behind the public classes, implemented once per platform.
 *
 * Adapters and peripherals are named by IDs the native side assigns, and a peripheral ID only has meaning together
 * with its adapter ID. Failures are thrown as [SimpleKtBleException].
 */
internal expect object Native {
    fun version(): String

    fun isBluetoothEnabled(): Boolean

    fun getAdapters(): LongArray

    fun adapterRegister(adapterId: Long, callback: AdapterCallback)

    fun adapterIdentifier(adapterId: Long): String

    fun adapterAddress(adapterId: Long): String

    fun adapterScanStart(adapterId: Long)

    fun adapterScanStop(adapterId: Long)

    fun adapterScanFor(adapterId: Long, timeoutMs: Int)

    fun adapterScanIsActive(adapterId: Long): Boolean

    fun adapterScanGetResults(adapterId: Long): LongArray

    fun adapterGetPairedPeripherals(adapterId: Long): LongArray

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
}
