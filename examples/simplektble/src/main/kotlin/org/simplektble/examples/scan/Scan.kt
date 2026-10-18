package org.simplektble.examples.scan

import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import org.simplektble.examples.chooseAdapter
import org.simplektble.examples.describe
import org.simplektble.examples.toHex

fun main() = runBlocking {
    val adapter = chooseAdapter()

    val events = launch(start = CoroutineStart.UNDISPATCHED) {
        launch(start = CoroutineStart.UNDISPATCHED) { adapter.onScanStart.collect { println("Scan started.") } }
        launch(start = CoroutineStart.UNDISPATCHED) { adapter.onScanStop.collect { println("Scan complete.") } }
        launch(start = CoroutineStart.UNDISPATCHED) {
            adapter.onScanFound.collect { println("Found ${it.describe()} ${it.rssi} dBm") }
        }
        launch(start = CoroutineStart.UNDISPATCHED) {
            adapter.onScanUpdated.collect { println("Updated ${it.describe()} ${it.rssi} dBm") }
        }
    }

    // Scan for 5 seconds
    adapter.scanFor(5000)
    events.cancel()

    println("The following peripherals were found:")
    for (peripheral in adapter.scanGetResults()) {
        println("${peripheral.describe()} ${peripheral.rssi} dBm, connectable: ${peripheral.isConnectable}")
        for ((companyId, data) in peripheral.manufacturerData()) {
            println("    Manufacturer ${companyId.toString(16).padStart(4, '0')}: ${data.toHex()}")
        }
    }
}
