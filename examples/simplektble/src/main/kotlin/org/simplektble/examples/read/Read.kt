package org.simplektble.examples.read

import kotlinx.coroutines.runBlocking
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

    val contents = peripheral.read(choice.service, choice.characteristic)
    println("Contents: ${contents.toHex()}")

    peripheral.disconnect()
}
