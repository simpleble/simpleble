package org.simplektble.examples.connect

import kotlinx.coroutines.runBlocking
import org.simplektble.examples.chooseAdapter
import org.simplektble.examples.describe
import org.simplektble.examples.scanAndChoose

fun main() = runBlocking {
    val adapter = chooseAdapter()
    val peripheral = scanAndChoose(adapter)

    println("Connecting to: ${peripheral.describe()}")
    peripheral.connect()

    println("Successfully connected. MTU: ${peripheral.mtu}")
    for (service in peripheral.services()) {
        println("Service: ${service.uuid}")
        for (characteristic in service.characteristics) {
            val capabilities = buildList {
                if (characteristic.canRead) add("read")
                if (characteristic.canWriteRequest) add("write request")
                if (characteristic.canWriteCommand) add("write command")
                if (characteristic.canNotify) add("notify")
                if (characteristic.canIndicate) add("indicate")
            }
            println("    Characteristic: ${characteristic.uuid} (${capabilities.joinToString()})")
            for (descriptor in characteristic.descriptors) {
                println("        Descriptor: ${descriptor.uuid}")
            }
        }
    }

    peripheral.disconnect()
}
