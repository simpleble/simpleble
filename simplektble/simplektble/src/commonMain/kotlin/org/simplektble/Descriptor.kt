package org.simplektble

public class Descriptor internal constructor(uuid: String) {
    public val uuid: BluetoothUUID = BluetoothUUID(uuid)
}
