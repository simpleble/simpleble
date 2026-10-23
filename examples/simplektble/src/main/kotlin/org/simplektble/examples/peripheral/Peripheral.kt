package org.simplektble.examples.peripheral

import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.awaitCancellation
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import org.simplektble.BluetoothUUID
import org.simplektble.CharacteristicCapability
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
        CharacteristicCapability.Read,
        CharacteristicCapability.WriteRequest,
        CharacteristicCapability.WriteCommand,
        CharacteristicCapability.Notify,
        CharacteristicCapability.Indicate
    )
    characteristic.setValue("ready".encodeToByteArray())

    launch(start = CoroutineStart.UNDISPATCHED) {
        characteristic.onWrite.collect { value ->
            println("Write: ${value.toHex()}")
            // Echo the written value to subscribed clients.
            characteristic.setValue(value)
        }
    }
    launch(start = CoroutineStart.UNDISPATCHED) {
        characteristic.onSubscribed.collect { println("Client subscribed.") }
    }
    launch(start = CoroutineStart.UNDISPATCHED) {
        characteristic.onUnsubscribed.collect { println("Client unsubscribed.") }
    }
    launch(start = CoroutineStart.UNDISPATCHED) {
        host.onClientConnected.collect { println("Client connected: $it") }
    }
    launch(start = CoroutineStart.UNDISPATCHED) {
        host.onClientDisconnected.collect { println("Client disconnected: $it") }
    }

    host.start()
    println("Advertising. Press Ctrl+C to stop.")
    awaitCancellation()
}
