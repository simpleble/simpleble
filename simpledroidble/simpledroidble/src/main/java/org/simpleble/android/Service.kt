package org.simpleble.android

class Service(
    uuid: String,
    val characteristics: List<Characteristic>
) {
    val uuid = BluetoothUUID(uuid)
}
