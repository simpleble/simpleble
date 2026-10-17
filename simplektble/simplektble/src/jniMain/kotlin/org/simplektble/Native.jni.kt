package org.simplektble

/** Loads the JNI library, which each platform ships differently. */
internal expect fun loadNativeLibrary()

internal actual object Native {
    init {
        loadNativeLibrary()
    }

    actual external fun version(): String

    actual external fun isBluetoothEnabled(): Boolean

    actual external fun getAdapters(): LongArray

    actual external fun adapterRegister(adapterId: Long, callback: AdapterCallback)

    actual external fun adapterIdentifier(adapterId: Long): String

    actual external fun adapterAddress(adapterId: Long): String

    actual external fun adapterScanStart(adapterId: Long)

    actual external fun adapterScanStop(adapterId: Long)

    actual external fun adapterScanFor(adapterId: Long, timeoutMs: Int)

    actual external fun adapterScanIsActive(adapterId: Long): Boolean

    actual external fun adapterScanGetResults(adapterId: Long): LongArray

    actual external fun adapterGetPairedPeripherals(adapterId: Long): LongArray

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
}
