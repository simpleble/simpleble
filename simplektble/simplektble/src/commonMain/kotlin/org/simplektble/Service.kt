package org.simplektble

public class Service internal constructor(
    uuid: String,
    public val characteristics: List<Characteristic>
) {
    public val uuid: BluetoothUUID = BluetoothUUID(uuid)
}
