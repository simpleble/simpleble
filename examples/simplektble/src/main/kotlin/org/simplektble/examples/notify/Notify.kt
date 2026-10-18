package org.simplektble.examples.notify

import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withTimeoutOrNull
import org.simplektble.examples.chooseAdapter
import org.simplektble.examples.chooseCharacteristic
import org.simplektble.examples.describe
import org.simplektble.examples.scanAndChoose
import org.simplektble.examples.toHex

fun main() = runBlocking {
    val adapter = chooseAdapter()
    val peripheral = scanAndChoose(adapter)

    println("Connecting to: ${peripheral.describe()}")
    peripheral.connect()

    println("Successfully connected, listing services...")
    val choice = chooseCharacteristic(peripheral)

    // Collecting subscribes, and the subscription ends when the collection does, after 5 seconds here.
    withTimeoutOrNull(5000) {
        peripheral.notify(choice.service, choice.characteristic).collect { data ->
            println("Notification: ${data.toHex()}")
        }
    }

    peripheral.disconnect()
}
