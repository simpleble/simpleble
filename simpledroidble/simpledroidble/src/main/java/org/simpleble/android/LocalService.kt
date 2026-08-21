package org.simpleble.android

class LocalService internal constructor(private val instanceId: Long) {
    private val configuredCharacteristics = mutableListOf<LocalCharacteristic>()

    val uuid: BluetoothUUID = BluetoothUUID(nativeUuid(instanceId))

    val characteristics: List<LocalCharacteristic>
        get() = synchronized(configuredCharacteristics) { configuredCharacteristics.toList() }

    fun addCharacteristic(
        uuid: BluetoothUUID,
        vararg capabilities: LocalCharacteristicCapability
    ): LocalCharacteristic {
        require(capabilities.isNotEmpty()) { "A local characteristic requires at least one capability." }
        val mask = capabilities.fold(0) { result, capability -> result or capability.bit }
        val characteristic = LocalCharacteristic(nativeAddCharacteristic(instanceId, uuid.value, mask))
        synchronized(configuredCharacteristics) { configuredCharacteristics += characteristic }
        return characteristic
    }

    private external fun nativeUuid(serviceId: Long): String
    private external fun nativeAddCharacteristic(serviceId: Long, uuid: String, capabilities: Int): Long
}
