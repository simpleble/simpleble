package org.simplektble

public class Service internal constructor(
    uuid: String,
    /** The service data the peripheral advertised for this service, empty when it advertised none. */
    public val data: ByteArray,
    public val characteristics: List<Characteristic>
) {
    public val uuid: BluetoothUUID = BluetoothUUID(uuid)
}
