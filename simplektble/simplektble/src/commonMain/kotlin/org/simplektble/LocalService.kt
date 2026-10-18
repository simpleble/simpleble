package org.simplektble

import kotlinx.atomicfu.locks.SynchronizedObject
import kotlinx.atomicfu.locks.synchronized

public class LocalService internal constructor(private val instanceId: Long) {
    private val characteristicsLock = SynchronizedObject()
    private val configuredCharacteristics = mutableListOf<LocalCharacteristic>()

    public val uuid: BluetoothUUID = BluetoothUUID(Native.localServiceUuid(instanceId))

    public val characteristics: List<LocalCharacteristic>
        get() = synchronized(characteristicsLock) { configuredCharacteristics.toList() }

    @Throws(SimpleKtBleException::class, IllegalArgumentException::class)
    public fun addCharacteristic(
        uuid: BluetoothUUID,
        vararg capabilities: LocalCharacteristicCapability
    ): LocalCharacteristic {
        require(capabilities.isNotEmpty()) { "A local characteristic requires at least one capability." }
        val mask = capabilities.fold(0) { result, capability -> result or capability.bit }
        val characteristic = LocalCharacteristic(Native.localServiceAddCharacteristic(instanceId, uuid.value, mask))
        synchronized(characteristicsLock) { configuredCharacteristics += characteristic }
        return characteristic
    }
}
