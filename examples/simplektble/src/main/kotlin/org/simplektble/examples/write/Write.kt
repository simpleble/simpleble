package org.simplektble.examples.write

import kotlinx.coroutines.runBlocking
import org.simplektble.examples.chooseAdapter
import org.simplektble.examples.chooseCharacteristic
import org.simplektble.examples.describe
import org.simplektble.examples.scanAndChoose

fun main() = runBlocking {
    val adapter = chooseAdapter()
    val peripheral = scanAndChoose(adapter)

    println("Connecting to: ${peripheral.describe()}")
    peripheral.connect()

    println("Successfully connected, listing services...")
    val choice = chooseCharacteristic(peripheral)

    print("Enter content to write: ")
    val content = readln()
    peripheral.writeRequest(choice.service, choice.characteristic, content.encodeToByteArray())

    peripheral.disconnect()
}
