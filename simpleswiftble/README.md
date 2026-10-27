<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo.png">
    <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo-dark.png" alt="SimpleBLE" width="360">
  </picture>
</p>

# SimpleSwiftBLE

**Build Bluetooth once. Ship it everywhere.**

SimpleSwiftBLE brings [SimpleBLE](https://github.com/simpleble/simpleble) to
Swift on iOS, iPadOS, macOS, and Mac Catalyst. Discover and connect Bluetooth
peripherals, read and write GATT values, subscribe to notifications, and host
local GATT services through one API, with `async`/`await` and `AsyncStream`
throughout.

## Why SimpleSwiftBLE

- **One cross-platform API:** Share Bluetooth code across your iOS, iPadOS, macOS, and Mac Catalyst apps, with the same API SimpleBLE offers in every other language.
- **Native underneath:** Use CoreBluetooth, and Dongl USB adapters on macOS, through one maintained stack.
- **Built for real products:** Act as a central or a peripheral on every supported platform.
- **Commercially supported:** Ship proprietary applications with commercial licensing and direct technical support from the team that builds SimpleBLE.

## Support and resources

- **Documentation:** The [usage guide](https://docs.simpleble.org/simpleswiftble/usage?utm_source=github&utm_medium=referral&utm_campaign=simpleswiftble_readme&utm_content=support) and [examples](https://github.com/simpleble/simpleble/tree/main/examples/simpleswiftble) cover every API.
- **Commercial support:** Visit [simpleble.org](https://simpleble.org/?utm_source=github&utm_medium=referral&utm_campaign=simpleswiftble_readme&utm_content=support) or [email us](mailto:contact@simpleble.org) about licensing and professional services.
- **Community:** Ask questions and share projects in [GitHub Discussions](https://github.com/simpleble/simpleble/discussions).
- **Dongl:** Prototype and ship with the [official SimpleBLE USB adapter](https://www.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simpleswiftble_readme&utm_content=support).
- **Contribute:** Send changes to [simpleble/simpleble](https://github.com/simpleble/simpleble/tree/main/simpleswiftble); the [development guide](https://github.com/simpleble/simpleble/blob/main/docs/extras/simpleswiftble-development.md) covers builds and tests.

## Get started

Use Swift 6.0 (Xcode 16) or newer. In Xcode, choose **File > Add Package
Dependencies** and enter `https://github.com/simpleble/simpleswiftble`, or add the
package to your `Package.swift`:

```swift
dependencies: [
    .package(url: "https://github.com/simpleble/simpleswiftble", from: "<version>")
],
targets: [
    .target(name: "MyApp", dependencies: [.product(name: "SimpleSwiftBLE", package: "simpleswiftble")])
]
```

The package includes SimpleBLE prebuilt for each platform, so only the Swift
layer compiles with your app.

Scan for five seconds and print what was found:

```swift
import SimpleSwiftBLE

let adapter = try Adapter.getAdapters().first!

let found = adapter.onScanFound
let printer = Task {
    for await peripheral in found {
        print("\(peripheral.identifier) [\(peripheral.address)] \(peripheral.rssi) dBm")
    }
}
try await adapter.scanFor(timeoutMs: 5000)
printer.cancel()
```

## Permissions

Add an `NSBluetoothAlwaysUsageDescription` entry to your app's `Info.plist`.
The system asks the user the first time your app uses Bluetooth. A sandboxed
macOS app also needs the Bluetooth capability
(`com.apple.security.device.bluetooth`).

## Scanning and events

Events are `AsyncStream`s. An adapter has `onPowerOn`, `onPowerOff`,
`onScanStart`, `onScanStop`, `onScanFound`, and `onScanUpdated`; a peripheral
has `onConnected` and `onDisconnected`. Each stream buffers every event from the
moment you get it, in the order it happened, so get it before the call that
produces the events.

Use `scanStart()` and `scanStop()` when your application controls scan timing,
and `scanGetResults()` for the peripherals seen by the last scan. You always get
the same `Peripheral` object for the same device, whether it came from a stream
or from a list, and it is `Identifiable` for SwiftUI lists.
`retrieveCachedPeripherals()` returns peripherals the system already knows,
without scanning.

Calls that wait on a device or the Bluetooth stack, such as `connect()`,
`read()`, and `scanFor()`, are `async` functions that never block Swift's
cooperative thread pool, so you can call them from the main actor. Data the
library already holds, such as a peripheral's `identifier`, `rssi`, or
`manufacturerData()`, is returned directly. The
[usage guide](https://docs.simpleble.org/simpleswiftble/usage?utm_source=github&utm_medium=referral&utm_campaign=simpleswiftble_readme&utm_content=scanning) covers
cancellation and SwiftUI.

## Connect and use GATT

Read the battery level of a peripheral and follow its notifications:

```swift
let batteryService: BluetoothUUID = "0000180f-0000-1000-8000-00805f9b34fb"
let batteryLevel: BluetoothUUID = "00002a19-0000-1000-8000-00805f9b34fb"

let peripheral = try adapter.scanGetResults().first { $0.identifier == "My Sensor" }!
try await peripheral.connect()

let level = try await peripheral.read(service: batteryService, characteristic: batteryLevel)
print("Battery: \(level[0])%")

for await value in try await peripheral.notify(service: batteryService, characteristic: batteryLevel) {
    print("Battery: \(value[0])%")
}
```

`services()` lists the services, characteristics, capabilities, and
descriptors of a connected peripheral, and before connecting, the advertised
services with their `data`. `read`, `writeRequest`, and `writeCommand` access
characteristic values, and `read` and `write` with a descriptor UUID access
descriptors. `notify` and `indicate` return a stream once subscribed, and
cancelling its task unsubscribes. When the peripheral disconnects, the stream
ends; subscribe again after reconnecting. Failures are thrown as
`SimpleBLEError`, whose `code` is SimpleBLE's error code.

## Host a GATT service

Create a local peripheral from an adapter, add services and characteristics,
then start advertising:

```swift
let host = try adapter.createLocalPeripheral()
try host.addAdvertisedService(batteryService)

let battery = try host.addService(batteryService)
    .addCharacteristic(batteryLevel, capabilities: [.read, .notify])
try await battery.setValue(Data([100]))

try await host.start()
```

`setValue()` updates the stored value and notifies subscribers. A local
characteristic has `onWrite`, `onSubscribed`, and `onUnsubscribed` streams, and
`setReadHandler` computes a value for each read. The local peripheral has
`onClientConnected` and `onClientDisconnected`, and
`setAdvertisementLocalName()` sets the advertised name. Configure services
before `start()`; call `stop()` before changing them.

## Use a Dongl

A [Dongl](https://docs.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simpleswiftble_readme&utm_content=dongl) is a USB Bluetooth adapter made for
SimpleBLE, available on macOS. Its backend is on by default; use the adapters
of the Dongl backend:

```swift
let adapter = try Backend.getDongl().getAdapters().first!
```

`Config.Dongl` holds the Dongl backend settings; set them before the first
call that reaches a backend. Pairing through a Dongl happens without the
operating system: set `setPasskeyRequestHandler`, `setPasskeyDisplayHandler`,
or `setNumericComparisonHandler` on the peripheral before `connect()`. The
handlers are `async`, so they can wait for the user.

## Platforms

| Platform | Architectures | Requirements |
| --- | --- | --- |
| iOS and iPadOS 15.8 or newer | arm64, and arm64 and x86_64 in the simulator | `NSBluetoothAlwaysUsageDescription` |
| Mac Catalyst 16.0 or newer | arm64, x86_64 | `NSBluetoothAlwaysUsageDescription` |
| macOS 15.0 or newer | arm64, x86_64 | `NSBluetoothAlwaysUsageDescription` |

## Versions

SimpleSwiftBLE follows SimpleBLE's release versions. Stable releases use
`X.Y.Z`, and development builds of the main branch are published as
`X.Y.Z-dev.N`; depend on one with `exact:`, or on the latest with
`branch: "main"`.

## License

SimpleSwiftBLE follows the [SimpleBLE license](https://github.com/simpleble/simpleble/blob/main/LICENSE.md),
the Business Source License 1.1 (BUSL-1.1). It is free for non-commercial use,
and commercial use needs a commercial license. We offer free commercial licenses
for small projects and early-stage companies: [email us](mailto:contact@simpleble.org)
or [leave us a message](https://www.simpleble.org/contact?utm_source=github&utm_medium=referral&utm_campaign=simpleswiftble_readme&utm_content=license) to discuss your use
case. Commercial licenses carry no royalties or revenue sharing; see the
[licensing page](https://docs.simpleble.org/licensing?utm_source=github&utm_medium=referral&utm_campaign=simpleswiftble_readme&utm_content=license) for terms.

---

**SimpleBLE** is a project powered by [The California Open Source Company](https://californiaopensource.com/?utm_source=github&utm_medium=referral&utm_campaign=simpleswiftble_readme&utm_content=footer).
