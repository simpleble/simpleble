package org.simplektble.examples

import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.launch
import org.simplektble.Adapter
import org.simplektble.BluetoothUUID
import org.simplektble.Peripheral
import kotlin.system.exitProcess

fun Adapter.describe() = "$identifier [$address]"

fun Peripheral.describe() = "$identifier [$address]"

/** Lists `items` and returns the one the user picks. */
fun <T> choose(title: String, items: List<T>, describe: (T) -> String): T {
    if (items.isEmpty()) fail("No $title found")
    println("Please select $title:")
    items.forEachIndexed { index, item -> println("$index: ${describe(item)}") }
    print("Enter choice: ")
    val choice = readln().trim().toIntOrNull()
    return choice?.let(items::getOrNull) ?: fail("Invalid choice")
}

fun chooseAdapter(): Adapter {
    val adapter = choose("an adapter", Adapter.getAdapters()) { it.describe() }
    println("Selected adapter: ${adapter.describe()}")
    return adapter
}

/** Scans for five seconds, printing what it finds, and returns the peripheral the user picks. */
suspend fun scanAndChoose(adapter: Adapter): Peripheral = coroutineScope {
    val events = launch(start = CoroutineStart.UNDISPATCHED) {
        launch { adapter.onScanStart.collect { println("Scan started.") } }
        launch { adapter.onScanStop.collect { println("Scan complete.") } }
        launch { adapter.onScanFound.collect { println("Found ${it.describe()}") } }
    }
    adapter.scanFor(5000)
    events.cancel()
    choose("a peripheral", adapter.scanGetResults()) { it.describe() }
}

data class CharacteristicChoice(val service: BluetoothUUID, val characteristic: BluetoothUUID)

fun chooseCharacteristic(peripheral: Peripheral): CharacteristicChoice {
    val pairs = peripheral.services().flatMap { service ->
        service.characteristics.map { CharacteristicChoice(service.uuid, it.uuid) }
    }
    return choose("a service/characteristic pair", pairs) { "${it.service} ${it.characteristic}" }
}

fun ByteArray.toHex() = joinToString("") { (it.toInt() and 0xff).toString(16).padStart(2, '0') }

fun fail(message: String): Nothing {
    System.err.println(message)
    exitProcess(1)
}
