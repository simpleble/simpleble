package org.simplektble.examples.dongl

import kotlinx.coroutines.runBlocking
import org.simplektble.Backend
import org.simplektble.examples.choose
import org.simplektble.examples.describe
import org.simplektble.examples.scanAndChoose

fun main() = runBlocking {
    val adapter = choose("a Dongl", "No Dongl found", Backend.getDongl().getAdapters()) { it.describe() }
    println("Selected Dongl: ${adapter.describe()}")

    val peripheral = scanAndChoose(adapter)

    // Pairing can start as soon as the link is up, so set the handlers before connecting.
    // The Dongl pairs without the operating system.
    peripheral.setPasskeyRequestHandler {
        print("Enter the passkey shown on the device (empty to reject): ")
        readln().ifBlank { null }
    }
    peripheral.setPasskeyDisplayHandler { passkey -> println("Enter $passkey on the device.") }
    peripheral.setNumericComparisonHandler { passkey ->
        print("Does the device show $passkey? [y/N] ")
        readln().trim() == "y"
    }

    println("Connecting to: ${peripheral.describe()}")
    peripheral.connect()

    println("Successfully connected, listing services...")
    for (service in peripheral.services()) {
        println("Service: ${service.uuid}")
        for (characteristic in service.characteristics) {
            println("    Characteristic: ${characteristic.uuid}")
        }
    }

    peripheral.disconnect()
}
