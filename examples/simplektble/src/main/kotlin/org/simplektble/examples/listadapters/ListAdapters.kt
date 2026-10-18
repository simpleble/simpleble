package org.simplektble.examples.listadapters

import org.simplektble.Adapter
import org.simplektble.examples.describe

fun main() {
    println("Bluetooth enabled: ${Adapter.isBluetoothEnabled()}")
    for (adapter in Adapter.getAdapters()) {
        println("Adapter: ${adapter.describe()}")
    }
}
