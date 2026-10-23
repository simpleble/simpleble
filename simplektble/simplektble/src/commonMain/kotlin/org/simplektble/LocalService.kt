package org.simplektble

import kotlinx.atomicfu.locks.SynchronizedObject
import kotlinx.atomicfu.locks.synchronized

public class LocalService internal constructor(private val instanceId: Long) {
    private val characteristicsLock = SynchronizedObject()
    private val characteristicsById = mutableMapOf<Long, LocalCharacteristic>()

    public val uuid: BluetoothUUID = BluetoothUUID(Native.localServiceUuid(instanceId))

    private fun characteristic(characteristicId: Long): LocalCharacteristic {
        return synchronized(characteristicsLock) {
            characteristicsById.getOrPut(characteristicId) { LocalCharacteristic(characteristicId) }
        }
    }

    @Throws(SimpleKtBleException::class)
    public fun characteristics(): List<LocalCharacteristic> {
        return Native.localServiceCharacteristics(instanceId).map(::characteristic)
    }

    @Throws(SimpleKtBleException::class)
    public fun addCharacteristic(
        uuid: BluetoothUUID,
        vararg capabilities: LocalCharacteristicCapability
    ): LocalCharacteristic {
        val mask = capabilities.fold(0) { result, capability -> result or capability.bit }
        return characteristic(Native.localServiceAddCharacteristic(instanceId, uuid.value, mask))
    }
}
