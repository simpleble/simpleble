<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo.png">
    <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo-dark.png" alt="SimpleBLE" width="360">
  </picture>
</p>

# SimpleKtBLE

**Build Bluetooth once. Ship it everywhere.**

SimpleKtBLE brings [SimpleBLE](https://github.com/simpleble/simpleble) to
Kotlin Multiplatform on Android, iOS, macOS, Windows, and Linux. Discover and
connect Bluetooth peripherals, read and write GATT values, follow
notifications, and host local GATT services from your shared Kotlin code, with
coroutines and flows throughout.

## Why SimpleKtBLE

- **One cross-platform API:** Keep your Bluetooth code in shared Kotlin for Android, iOS, macOS, Windows, and Linux.
- **Native underneath:** Use Android Bluetooth, CoreBluetooth, WinRT, and BlueZ through one maintained stack.
- **Built for real products:** Act as a central or a peripheral on every supported platform.
- **Commercially supported:** Ship proprietary applications with commercial licensing and direct technical support from the team that builds SimpleBLE.

## Support and resources

- **Documentation:** The [usage guide](https://docs.simpleble.org/simplektble/usage?utm_source=github&utm_medium=referral&utm_campaign=simplektble_readme&utm_content=support) and [examples](https://github.com/simpleble/simpleble/tree/main/examples/simplektble) cover every API.
- **Commercial support:** Visit [simpleble.org](https://simpleble.org/?utm_source=github&utm_medium=referral&utm_campaign=simplektble_readme&utm_content=support) or [email us](mailto:contact@simpleble.org) about licensing and professional services.
- **Community:** Ask questions and share projects in [GitHub Discussions](https://github.com/simpleble/simpleble/discussions).
- **Dongl:** Prototype and ship with the [official SimpleBLE USB adapter](https://www.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simplektble_readme&utm_content=support).
- **Contribute:** Send changes to [simpleble/simpleble](https://github.com/simpleble/simpleble/tree/main/simplektble); the [development guide](https://github.com/simpleble/simpleble/blob/main/docs/extras/simplektble-development.md) covers builds and tests.

## Get started

Use Kotlin 2.3 or newer, and add SimpleKtBLE to the shared module of your project:

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

On Android, SimpleKtBLE declares the Bluetooth permissions it needs. Android
delivers scan results only to apps that also hold `ACCESS_FINE_LOCATION`,
unless the app states that it never derives location from them. If yours
doesn't, say so in your app's manifest:

```xml
<uses-permission
    android:name="android.permission.BLUETOOTH_SCAN"
    android:usesPermissionFlags="neverForLocation" />
```

Request the permissions at runtime before using an adapter:

```kotlin
import org.simplektble.SimpleKtBle
import org.simplektble.hasPermissions
import org.simplektble.requestPermissions

if (!SimpleKtBle.hasPermissions(activity)) {
    SimpleKtBle.requestPermissions(activity)
}
```

`hasPermissions` and `requestPermissions` cover scanning and connecting. To
host a local peripheral, use `hasPeripheralPermissions` and
`requestPeripheralPermissions`, which cover advertising and client connections.

On iOS and macOS, add an `NSBluetoothAlwaysUsageDescription` entry to your
app's `Info.plist`. The system asks the user the first time your app uses
Bluetooth.

## Scanning and events

Events are flows. An adapter has `onPowerOn`, `onPowerOff`, `onScanStart`,
`onScanStop`, `onScanActive`, `onScanFound`, and `onScanUpdated`; a peripheral
has `onConnected`, `onDisconnected`, and `onConnectionActive`. Every event
reaches each active collector, in the order it happened.

Use `scanStart()` and `scanStop()` when your application controls scan timing,
and `scanGetResults()` for the peripherals seen by the last scan. An adapter
always gives you the same `Peripheral` object for the same device, whether it
came from a flow or from a list. On iOS and macOS,
`retrieveCachedPeripherals()` returns peripherals the system already knows,
without scanning.

Calls that wait on a device or the Bluetooth stack, such as `connect()`,
`read()`, and `scanFor()`, are `suspend` functions that run on
`Dispatchers.IO`, so you can call them from the main thread. Data the library
already holds, such as a peripheral's `identifier`, `rssi`, or
`manufacturerData()`, is returned directly. The
[usage guide](https://docs.simpleble.org/simplektble/usage?utm_source=github&utm_medium=referral&utm_campaign=simplektble_readme&utm_content=scanning) covers
cancellation.

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
descriptors of a connected peripheral, and before connecting, the advertised
services with their `data`. `read`, `writeRequest`, and `writeCommand` access
characteristic values, and `read` and `write` with a descriptor UUID access
descriptors. `notify` and `indicate` return a flow that subscribes when
collected and unsubscribes when the collection ends. When the peripheral
disconnects, the flow completes; collect it again after reconnecting. Failures
are thrown as `SimpleKtBleException`, whose `code` is SimpleBLE's `ErrorCode`.

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
battery.setValue(byteArrayOf(100))

host.start()
```

`setValue()` updates the stored value and notifies subscribers. A local
characteristic has `onWrite`, `onSubscribed`, and `onUnsubscribed` flows, and
`setReadHandler` computes a value for each read. The local peripheral has
`onClientConnected` and `onClientDisconnected`. On iOS, macOS, and Linux,
`setAdvertisementLocalName()` sets the advertised name. Configure services
before `start()`; call `stop()` before changing them.

## Use a Dongl

A [Dongl](https://docs.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simplektble_readme&utm_content=dongl) is a USB Bluetooth adapter made for
SimpleBLE, available on macOS, Windows, and Linux. Its backend is on by
default; use the adapters of the Dongl backend:

```kotlin
val dongl = Backend.getDongl()
val adapter = dongl.getAdapters().first()
```

`Config` holds the Android, Dongl, BlueZ, and WinRT backend settings; set them
before the first call that reaches a backend. Pairing through a Dongl happens
without the operating system: set `setPasskeyRequestHandler`,
`setPasskeyDisplayHandler`, or `setNumericComparisonHandler` on the peripheral
before `connect()`.

## Platforms

| Platform | Kotlin targets | Architectures |
| --- | --- | --- |
| Android 12 (API 31) or newer | `android` | arm64-v8a, armeabi-v7a, x86, x86_64 |
| iOS 15.8 or newer | `iosArm64`, `iosSimulatorArm64` | arm64 |
| macOS 15.0 or newer | `jvm`, `macosArm64` | x64 and arm64 on the JVM, arm64 native |
| Windows 10 or newer | `jvm` | x64 |
| Linux with BlueZ and D-Bus | `jvm` | x64, arm64 |

## Versions

SimpleKtBLE follows SimpleBLE's release versions. Stable releases use
`X.Y.Z`, and development builds of the main branch are published as
`X.Y.Z-devN`.

## License

SimpleKtBLE follows the [SimpleBLE license](https://github.com/simpleble/simpleble/blob/main/LICENSE.md),
the Business Source License 1.1 (BUSL-1.1). It is free for non-commercial use,
and commercial use needs a commercial license. We offer free commercial licenses
for small projects and early-stage companies: [email us](mailto:contact@simpleble.org)
or [leave us a message](https://www.simpleble.org/contact?utm_source=github&utm_medium=referral&utm_campaign=simplektble_readme&utm_content=license) to discuss your use
case. Commercial licenses carry no royalties or revenue sharing; see the
[licensing page](https://docs.simpleble.org/licensing?utm_source=github&utm_medium=referral&utm_campaign=simplektble_readme&utm_content=license) for terms.

---

**SimpleBLE** is a project powered by [The California Open Source Company](https://californiaopensource.com/?utm_source=github&utm_medium=referral&utm_campaign=simplektble_readme&utm_content=footer).
