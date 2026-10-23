package org.simplektble

public class Characteristic internal constructor(
    uuid: String,
    public val descriptors: List<Descriptor>,
    public val canRead: Boolean,
    public val canWriteRequest: Boolean,
    public val canWriteCommand: Boolean,
    public val canNotify: Boolean,
    public val canIndicate: Boolean
) {
    public val uuid: BluetoothUUID = BluetoothUUID(uuid)

    public val capabilities: Set<CharacteristicCapability> = CharacteristicCapability.entries.filterTo(linkedSetOf()) {
        when (it) {
            CharacteristicCapability.Read -> canRead
            CharacteristicCapability.WriteRequest -> canWriteRequest
            CharacteristicCapability.WriteCommand -> canWriteCommand
            CharacteristicCapability.Notify -> canNotify
            CharacteristicCapability.Indicate -> canIndicate
        }
    }
}
