<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo.png">
    <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo-dark.png" alt="SimpleBLE" width="360">
  </picture>
</p>

# SimpleGoBLE

**Build Bluetooth once. Ship it everywhere.**

SimpleGoBLE brings [SimpleBLE](https://github.com/simpleble/simpleble) to Go on
Windows, macOS, and Linux. Discover and connect Bluetooth peripherals, read and
write GATT values, subscribe to notifications, and host local GATT services
through one API, without cgo.

## Why SimpleGoBLE

- **One cross-platform API:** Keep your Bluetooth client and hosting logic the same on Windows, macOS, and Linux.
- **Native underneath:** Use WinRT, CoreBluetooth, and BlueZ through one maintained stack.
- **Built for real products:** Act as a central or a peripheral on every supported platform.
- **Commercially supported:** Ship proprietary applications with commercial licensing and direct technical support from the team that builds SimpleBLE.

## Support and resources

- **Documentation:** The [usage guide](https://docs.simpleble.org/simplegoble/usage?utm_source=github&utm_medium=referral&utm_campaign=simplegoble_readme&utm_content=support), [API reference](https://pkg.go.dev/github.com/simpleble/simplegoble), and [examples](https://github.com/simpleble/simpleble/tree/main/examples/simplegoble) cover every API.
- **Commercial support:** Visit [simpleble.org](https://simpleble.org/?utm_source=github&utm_medium=referral&utm_campaign=simplegoble_readme&utm_content=support) or [email us](mailto:contact@simpleble.org) about licensing and professional services.
- **Community:** Ask questions and share projects in [GitHub Discussions](https://github.com/simpleble/simpleble/discussions).
- **Dongl:** Prototype and ship with the [official SimpleBLE USB adapter](https://www.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simplegoble_readme&utm_content=support).
- **Contribute:** Send changes to [simpleble/simpleble](https://github.com/simpleble/simpleble/tree/main/simplegoble); the [publishing guide](https://github.com/simpleble/simpleble/blob/main/docs/extras/simplegoble-publishing.md) covers builds and releases.

## Get started

Use Go 1.25 or newer:

```sh
go get github.com/simpleble/simplegoble
```

The module embeds the native SimpleBLE library for every supported platform and
loads it without cgo, so no C compiler is needed.

Scan for five seconds and print what was found:

```go
package main

import (
    "fmt"
    "log"
    "time"

    ble "github.com/simpleble/simplegoble"
)

func main() {
    adapters, err := ble.GetAdapters()
    if err != nil {
        log.Fatal(err)
    }
    adapter := adapters[0]

    adapter.SetCallbackOnScanFound(func(peripheral *ble.Peripheral) {
        defer peripheral.Close()
        identifier, _ := peripheral.Identifier()
        address, _ := peripheral.Address()
        rssi, _ := peripheral.RSSI()
        fmt.Printf("%s [%s] %d dBm\n", identifier, address, rssi)
    })
    if err := adapter.ScanFor(5 * time.Second); err != nil {
        log.Fatal(err)
    }
}
```

## Scanning and events

An adapter takes callbacks through `SetCallbackOnScanStart`,
`SetCallbackOnScanStop`, `SetCallbackOnScanFound`, `SetCallbackOnScanUpdated`,
`SetCallbackOnPowerOn`, and `SetCallbackOnPowerOff`; a peripheral takes
`SetCallbackOnConnected` and `SetCallbackOnDisconnected`. Pass `nil` to clear a
callback. Callbacks run one at a time on each handle's goroutine, so keep them
short. Each peripheral passed to a callback is a handle you close when finished.

Use `ScanStart` and `ScanStop` when your application controls scan timing, and
`ScanGetResults` for the peripherals seen by the last scan.

## Connect and use GATT

Read the battery level of a peripheral and follow its notifications:

```go
const (
    batteryService = "0000180f-0000-1000-8000-00805f9b34fb"
    batteryLevel   = "00002a19-0000-1000-8000-00805f9b34fb"
)

func followBattery(peripheral *ble.Peripheral) error {
    peripheral.SetCallbackOnDisconnected(func() { fmt.Println("Disconnected") })
    if err := peripheral.Connect(); err != nil {
        return err
    }

    level, err := peripheral.Read(batteryService, batteryLevel)
    if err != nil {
        return err
    }
    fmt.Printf("Battery: %d%%\n", level[0])

    return peripheral.Notify(batteryService, batteryLevel, func(value []byte) {
        fmt.Printf("Battery: %d%%\n", value[0])
    })
}
```

`Services` lists the services, characteristics, capabilities, and descriptors
of a connected peripheral. `Read`, `WriteRequest`, and `WriteCommand` access
characteristic values; `ReadDescriptor` and `WriteDescriptor` access
descriptors. `Notify` and `Indicate` call your function with each value as a
byte slice, and `Unsubscribe` stops them. Failures are `*ble.Error` values with
SimpleBLE's error code in `Code`.

## Host a GATT service

Create a local peripheral from an adapter, add services and characteristics,
then start advertising:

```go
host, err := adapter.CreateLocalPeripheral()
if err != nil {
    return err
}
defer host.Close()
host.AddAdvertisedService(batteryService)

service, err := host.AddService(batteryService)
if err != nil {
    return err
}
battery, err := service.AddCharacteristic(batteryLevel, ble.CapabilityRead|ble.CapabilityNotify)
if err != nil {
    return err
}
battery.SetValue([]byte{100})

host.SetCallbackOnClientConnected(func(address string) { fmt.Println(address, "connected") })
return host.Start()
```

`SetValue` updates the stored value and publishes it to subscribers.
Characteristics take `SetCallbackOnWrite`, `SetCallbackOnSubscribed`, and
`SetCallbackOnUnsubscribed`, and `SetCallbackOnRead` computes a value for each
read. Configure services before `Start`; call `Stop` before changing them.

## Use a Dongl

A [Dongl](https://docs.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simplegoble_readme&utm_content=dongl) is a USB Bluetooth adapter made for
SimpleBLE. Its backend is on by default; use the adapters of the Dongl backend:

```go
dongl, err := ble.GetDonglBackend()
if err != nil {
    return err
}
adapters, err := dongl.Adapters()
```

`ble.Config` holds the Dongl, BlueZ, and WinRT backend settings; set them
before the first call that reaches a backend. Pairing through a Dongl happens
without the operating system: register `SetCallbackOnPasskeyRequest`,
`SetCallbackOnPasskeyDisplay`, or `SetCallbackOnNumericComparison` on the
peripheral before `Connect`.

## Platforms

| Platform | Architectures | Requirements |
| --- | --- | --- |
| macOS 13.0 or newer | x64, arm64 | Bluetooth permission for the app that runs your program |
| Windows 10 or newer | x64, arm64 | |
| Linux | x64, arm64 | BlueZ and the D-Bus library |

See [Bluetooth permissions](https://docs.simpleble.org/fundamentals/permissions?utm_source=github&utm_medium=referral&utm_campaign=simplegoble_readme&utm_content=platforms)
for the setup each platform needs.

## Versions

SimpleGoBLE follows SimpleBLE's release versions. `go get` picks the latest
stable release, `vX.Y.Z`, and development builds of the main branch are tagged
`vX.Y.Z-devN`:

```sh
go get github.com/simpleble/simplegoble@main
```

## License

SimpleGoBLE follows the [SimpleBLE license](https://github.com/simpleble/simpleble/blob/main/LICENSE.md),
the Business Source License 1.1 (BUSL-1.1). It is free for non-commercial use,
and commercial use needs a commercial license. We offer free commercial licenses
for small projects and early-stage companies: [email us](mailto:contact@simpleble.org)
or [leave us a message](https://www.simpleble.org/contact?utm_source=github&utm_medium=referral&utm_campaign=simplegoble_readme&utm_content=license) to discuss your use
case. Commercial licenses carry no royalties or revenue sharing; see the
[licensing page](https://docs.simpleble.org/licensing?utm_source=github&utm_medium=referral&utm_campaign=simplegoble_readme&utm_content=license) for terms.

---

**SimpleBLE** is a project powered by [The California Open Source Company](https://californiaopensource.com/?utm_source=github&utm_medium=referral&utm_campaign=simplegoble_readme&utm_content=footer).
