package org.simplektble.examples.peripheral

import kotlinx.coroutines.awaitCancellation
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import org.simplektble.BluetoothUUID
import org.simplektble.LocalCharacteristicCapability
import org.simplektble.examples.chooseAdapter
import org.simplektble.examples.toHex

private val SERVICE_UUID = BluetoothUUID("12345678-1234-5678-1234-56789abcdef0")
private val CHARACTERISTIC_UUID = BluetoothUUID("12345678-1234-5678-1234-56789abcdef1")

fun main(): Unit = runBlocking {
    val adapter = chooseAdapter()
    val host = adapter.createLocalPeripheral()

    // Add a service UUID to the advertisement.
    host.addAdvertisedService(SERVICE_UUID)

    val characteristic = host.addService(SERVICE_UUID).addCharacteristic(
        CHARACTERISTIC_UUID,
        LocalCharacteristicCapability.Read,
        LocalCharacteristicCapability.WriteRequest,
        LocalCharacteristicCapability.WriteCommand,
        LocalCharacteristicCapability.Notify,
        LocalCharacteristicCapability.Indicate
    )
    characteristic.value = "ready".encodeToByteArray()

    launch {
        characteristic.onWrite.collect { value ->
            println("Write: ${value.toHex()}")
            // Echo the written value to subscribed clients.
            characteristic.value = value
        }
    }
    launch { characteristic.onSubscribed.collect { println("Client subscribed.") } }
    launch { characteristic.onUnsubscribed.collect { println("Client unsubscribed.") } }
    launch { host.onClientConnected.collect { println("Client connected: $it") } }
    launch { host.onClientDisconnected.collect { println("Client disconnected: $it") } }

    host.start()
    println("Advertising. Press Ctrl+C to stop.")
    awaitCancellation()
}
