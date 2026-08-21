package org.simpleble.android

/** Advertising data for a [LocalPeripheral]. */
data class LocalAdvertisement(
    val localName: String? = null,
    val serviceUuids: List<BluetoothUUID> = emptyList()
)
