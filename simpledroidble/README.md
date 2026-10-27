<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo.png">
    <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo-dark.png" alt="SimpleBLE" width="360">
  </picture>
</p>

# SimpleDroidBLE

**Build Bluetooth once. Ship it everywhere.**

SimpleDroidBLE brings [SimpleBLE](https://github.com/simpleble/simpleble) to
Kotlin on Android. Discover and connect Bluetooth peripherals, read and write
GATT values, follow notifications, and host local GATT services, with
coroutines and flows throughout.

To share Bluetooth code with iOS and desktop, use [SimpleKtBLE](https://docs.simpleble.org/simplektble/usage?utm_source=github&utm_medium=referral&utm_campaign=simpledroidble_readme&utm_content=intro#moving-from-simpledroidble).

## Why SimpleDroidBLE

- **Kotlin-first:** Call suspend functions and collect flows, safely from the main thread.
- **Native underneath:** Use Android's Bluetooth stack through the same core that runs SimpleBLE on every platform.
- **Built for real products:** Act as a central or a peripheral, with helpers for Android's runtime permissions.
- **Commercially supported:** Ship proprietary applications with commercial licensing and direct technical support from the team that builds SimpleBLE.

## Support and resources

- **Documentation:** The [usage guide](https://docs.simpleble.org/simpledroidble/usage?utm_source=github&utm_medium=referral&utm_campaign=simpledroidble_readme&utm_content=support) and [example app](https://github.com/simpleble/simpleble/tree/main/examples/simpleble-android) cover every API.
- **Commercial support:** Visit [simpleble.org](https://simpleble.org/?utm_source=github&utm_medium=referral&utm_campaign=simpledroidble_readme&utm_content=support) or [email us](mailto:contact@simpleble.org) about licensing and professional services.
- **Community:** Ask questions and share projects in [GitHub Discussions](https://github.com/simpleble/simpleble/discussions).
- **Contribute:** Send changes to [simpleble/simpleble](https://github.com/simpleble/simpleble/tree/main/simpledroidble); the [publishing guide](https://github.com/simpleble/simpleble/blob/main/docs/extras/simpledroidble-publishing.md) covers builds and releases.

## Get started

Add SimpleDroidBLE to your app module, with `minSdk = 31` or higher:

```kotlin
dependencies {
    implementation("org.simpleble:simpledroidble:<version>")
}
```

The artifact includes the native SimpleBLE library for every Android ABI, so
nothing is compiled during your build.

Scan for five seconds and print what was found:

```kotlin
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.launch
import org.simpleble.android.Adapter

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

SimpleDroidBLE declares the Bluetooth permissions it needs. Android delivers
scan results only to apps that also hold `ACCESS_FINE_LOCATION`, unless the app
states that it never derives location from them. If yours doesn't, say so in
your app's manifest:

```xml
<uses-permission
    android:name="android.permission.BLUETOOTH_SCAN"
    android:usesPermissionFlags="neverForLocation" />
```

Request the permissions at runtime before using an adapter:

```kotlin
import org.simpleble.android.SimpleDroidBle

if (!SimpleDroidBle.hasPermissions(activity)) {
    SimpleDroidBle.requestPermissions(activity)
}
```

`hasPermissions` and `requestPermissions` cover scanning and connecting. To
host a local peripheral, use `hasPeripheralPermissions` and
`requestPeripheralPermissions`, which cover advertising and client connections.
`requiredPermissions` and `requiredPeripheralPermissions` list the same
permissions for the Activity Result API.

## Scanning and events

Events are flows. An adapter has `onScanStart`, `onScanStop`, `onScanActive`,
`onScanFound`, and `onScanUpdated`; a peripheral has `onConnected`,
`onDisconnected`, and `onConnectionActive`.

Use `scanStart()` and `scanStop()` when your app controls scan timing, and
`scanGetResults()` for the peripherals seen by the last scan. An adapter always
gives you the same `Peripheral` object for the same device, whether it came
from a flow or from a list.

Calls that wait on a device or the Bluetooth stack, such as `connect()`,
`read()`, and `scanFor()`, are `suspend` functions that run on
`Dispatchers.IO`, so you can call them from the main thread. Data the library
already holds, such as a peripheral's `identifier`, `rssi`, or
`manufacturerData()`, is returned directly.

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
are thrown as `SimpleDroidBleException`.

## Host a GATT service

Create a local peripheral from an adapter, add services and characteristics,
then start advertising:

```kotlin
val host = adapter.createLocalPeripheral(context)
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
characteristic has `onWrite`, `onSubscribed`, and `onUnsubscribed` flows;
`setReadHandler` computes a value for each read, and `setWriteHandler` receives
every accepted write. The local peripheral has `onClientConnected` and
`onClientDisconnected`. Configure services before `start()`; call `stop()`
before changing them.

## Platforms

| Platform | Architectures | Requirements |
| --- | --- | --- |
| Android 12 (API 31) or newer | arm64-v8a, armeabi-v7a, x86, x86_64 | Runtime Bluetooth permissions |

## Versions

SimpleDroidBLE follows SimpleBLE's release versions. Stable releases use
`X.Y.Z`, and development builds of the main branch are published as
`X.Y.Z-dev.N`.

## License

SimpleDroidBLE follows the [SimpleBLE license](https://github.com/simpleble/simpleble/blob/main/LICENSE.md),
the Business Source License 1.1 (BUSL-1.1). It is free for non-commercial use,
and commercial use needs a commercial license. We offer free commercial licenses
for small projects and early-stage companies: [email us](mailto:contact@simpleble.org)
or [leave us a message](https://www.simpleble.org/contact?utm_source=github&utm_medium=referral&utm_campaign=simpledroidble_readme&utm_content=license) to discuss your use
case. Commercial licenses carry no royalties or revenue sharing; see the
[licensing page](https://docs.simpleble.org/licensing?utm_source=github&utm_medium=referral&utm_campaign=simpledroidble_readme&utm_content=license) for terms.

---

**SimpleBLE** is a project powered by [The California Open Source Company](https://californiaopensource.com/?utm_source=github&utm_medium=referral&utm_campaign=simpledroidble_readme&utm_content=footer).
