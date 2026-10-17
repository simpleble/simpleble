<p align="center">
  <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo-dark.png#gh-light-mode-only" alt="SimpleBLE" width="360">
  <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo.png#gh-dark-mode-only" alt="SimpleBLE" width="360">
</p>

# SimpleKtBLE

SimpleKtBLE brings [SimpleBLE](https://github.com/simpleble/simpleble) to
Kotlin Multiplatform on Android, iOS, macOS, Windows, and Linux. Discover and
connect Bluetooth peripherals, read and write GATT values, follow
notifications, and host local GATT services from your shared Kotlin code, with
coroutines and flows throughout.

## Get started

Add SimpleKtBLE to the shared module of your project:

```kotlin
kotlin {
    sourceSets {
        commonMain.dependencies {
            implementation("org.simpleble:simplektble:<version>")
        }
    }
}
```

Each target's artifact includes the native SimpleBLE library it needs, so
nothing is compiled during your build.

Scan for five seconds and print what was found:

```kotlin
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.launch
import org.simplektble.Adapter

suspend fun scan() = coroutineScope {
    val adapter = Adapter.getAdapters().first()

    val found = launch(start = CoroutineStart.UNDISPATCHED) {
        adapter.onScanFound.collect { peripheral ->
            println("${peripheral.identifier} [${peripheral.address}] ${peripheral.rssi} dBm")
        }
    }
    adapter.scanFor(5000)
    found.cancel()
}
```

## Permissions

On Android, SimpleKtBLE declares the Bluetooth permissions it needs. Request
them at runtime before using an adapter:

```kotlin
import org.simplektble.SimpleKtBle
import org.simplektble.hasPermissions
import org.simplektble.requestPermissions

if (!SimpleKtBle.hasPermissions(activity)) {
    SimpleKtBle.requestPermissions(activity)
}
```

On iOS and macOS, add an `NSBluetoothAlwaysUsageDescription` entry to your
app's `Info.plist`. The system asks the user the first time your app uses
Bluetooth.

## Scanning and events

Events are flows. An adapter has `onScanStart`, `onScanStop`, `onScanActive`,
`onScanFound`, and `onScanUpdated`; a peripheral has `onConnected`,
`onDisconnected`, and `onConnectionActive`. Every event reaches each active
collector, in the order it happened.

Use `scanStart()` and `scanStop()` when your application controls scan timing,
and `scanGetResults()` for the peripherals seen by the last scan. An adapter
always gives you the same `Peripheral` object for the same device, whether it
came from a flow or from a list.

Calls that talk to the Bluetooth stack, such as `connect()` or `read()`, are
`suspend` functions that run on `Dispatchers.IO`. Data the library already
holds, such as a peripheral's `identifier`, `rssi`, or `manufacturerData()`, is
returned directly.

## Connect and use GATT

Read the battery level of a peripheral and follow its notifications:

```kotlin
val batteryService = BluetoothUUID("0000180f-0000-1000-8000-00805f9b34fb")
val batteryLevel = BluetoothUUID("00002a19-0000-1000-8000-00805f9b34fb")

val peripheral = adapter.scanGetResults().first { it.identifier == "My Sensor" }
peripheral.connect()

val level = peripheral.read(batteryService, batteryLevel)
println("Battery: ${level[0]}%")

peripheral.notify(batteryService, batteryLevel).collect { value ->
    println("Battery: ${value[0]}%")
}
```

`services()` lists the services, characteristics, capabilities, and
descriptors of a connected peripheral. `read`, `writeRequest`, and
`writeCommand` access characteristic values, and `read` and `write` with a
descriptor UUID access descriptors. `notify` and `indicate` return a flow that
subscribes when collected and unsubscribes when the collection ends. Failures
are thrown as `SimpleKtBleException`.

## Host a GATT service

Create a local peripheral from an adapter, add services and characteristics,
then start advertising:

```kotlin
val host = adapter.createLocalPeripheral()
host.addAdvertisedService(batteryService)

val battery = host.addService(batteryService).addCharacteristic(
    batteryLevel,
    LocalCharacteristicCapability.Read,
    LocalCharacteristicCapability.Notify
)
battery.value = byteArrayOf(100)

host.start()
```

Setting `value` updates the stored value and notifies subscribers. A local
characteristic has `onWrite`, `onSubscribed`, and `onUnsubscribed` flows, and
`setReadHandler` computes a value for each read. The local peripheral has
`onClientConnected` and `onClientDisconnected`. Configure services before
`start()`; call `stop()` before changing them.

## Use a Dongl

A [Dongl](https://docs.simpleble.org/dongl) is a USB Bluetooth adapter made for
SimpleBLE, available on macOS, Windows, and Linux. Its backend is on by
default; use the adapters of the Dongl backend:

```kotlin
val dongl = Backend.getDongl()
val adapter = dongl.getAdapters().first()
```

`Config` holds the Dongl, BlueZ, and WinRT backend settings; set them before
the first call that reaches a backend. Pairing through a Dongl happens without
the operating system: set `setPasskeyRequestHandler`,
`setPasskeyDisplayHandler`, or `setNumericComparisonHandler` on the peripheral
before `connect()`.

## Platforms

| Platform | Kotlin targets | Architectures |
| --- | --- | --- |
| Android 12 (API 31) or newer | `android` | arm64-v8a, armeabi-v7a, x86, x86_64 |
| iOS 15.8 or newer | `iosArm64`, `iosSimulatorArm64` | arm64 |
| macOS 13.0 or newer | `jvm`, `macosArm64` | x64 and arm64 on the JVM, arm64 native |
| Windows 10 or newer | `jvm` | x64 |
| Linux with glibc 2.28 or newer, BlueZ and D-Bus | `jvm` | x64, arm64 |

## Versions

SimpleKtBLE follows SimpleBLE's release versions.

## Documentation and support

- Read the [SimpleBLE documentation](https://docs.simpleble.org).
- Join the [Discord community](https://discord.gg/N9HqNEcvP3).
- Contribute in [simpleble/simpleble](https://github.com/simpleble/simpleble/tree/main/simplektble); see the [development guide](https://github.com/simpleble/simpleble/blob/main/docs/extras/simplektble-development.md).
- Visit [simpleble.org](https://simpleble.org/) for commercial licensing and professional services.

## License

SimpleKtBLE follows the [SimpleBLE license](https://github.com/simpleble/simpleble/blob/main/LICENSE.md),
the Business Source License 1.1 (BUSL-1.1).

---

**SimpleBLE** is a project powered by [The California Open Source Company](https://californiaopensource.com/).
