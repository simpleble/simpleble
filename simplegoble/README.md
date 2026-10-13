<p align="center">
  <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo-dark.png#gh-light-mode-only" alt="SimpleBLE" width="360">
  <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo.png#gh-dark-mode-only" alt="SimpleBLE" width="360">
</p>

# SimpleGoBLE

**Build Bluetooth once. Ship it everywhere.**

SimpleGoBLE brings [SimpleBLE](https://github.com/simpleble/simpleble) to Go on
Windows, macOS, and Linux. Discover and connect Bluetooth peripherals, read and
write GATT values, subscribe to notifications, and host local GATT services through one API.

## Why SimpleGoBLE

- **One cross-platform API:** Keep your Bluetooth client and hosting logic consistent across desktop platforms.
- **Native underneath:** Use WinRT, CoreBluetooth, and BlueZ through SimpleBLE.

## Get started

Use Go 1.25 or newer:

```sh
go get github.com/simpleble/simplegoble
```

List your Bluetooth adapters:

```go
package main

import (
    "fmt"
    "log"

    ble "github.com/simpleble/simplegoble"
)

func main() {
    if err := run(); err != nil {
        log.Fatal(err)
    }
}

func run() error {
    adapters, err := ble.GetAdapters()
    if err != nil {
        return err
    }
    defer func() {
        for _, adapter := range adapters {
            _ = adapter.Close()
        }
    }()

    for _, adapter := range adapters {
        name, err := adapter.Identifier()
        if err != nil {
            return err
        }
        address, err := adapter.Address()
        if err != nil {
            return err
        }
        powered, err := adapter.IsPowered()
        if err != nil {
            return err
        }
        fmt.Printf("%s (%s), powered: %t\n", name, address, powered)
    }
    return nil
}
```

Build your application:

```sh
go build ./...
```

You can also run the included adapter listing example directly:

```sh
go run github.com/simpleble/simplegoble/examples/list_adapters
```

## Scan control and callbacks

Register callbacks on an adapter, then scan for a fixed duration:

```go
package main

import (
    "fmt"
    "time"

    ble "github.com/simpleble/simplegoble"
)

func scan(adapter *ble.Adapter) error {
    if err := adapter.SetCallbackOnScanStart(func() {
        fmt.Println("Scan started")
    }); err != nil {
        return err
    }
    if err := adapter.SetCallbackOnScanStop(func() {
        fmt.Println("Scan stopped")
    }); err != nil {
        return err
    }
    return adapter.ScanFor(5 * time.Second)
}
```

`ScanFor` waits for the requested duration while other goroutines remain runnable.
Use `ScanStart` and `ScanStop` when your application controls scan timing.
Power callbacks use `SetCallbackOnPowerOn` and `SetCallbackOnPowerOff`.
Pass `nil` to a callback setter to clear it.

After scanning, call `adapter.ScanGetResults()` to get peripherals. Use
`SetCallbackOnScanFound` and `SetCallbackOnScanUpdated` for live discovery;
close each peripheral handle passed to your handler when finished.

## Connect and use GATT

Read the battery level from a peripheral discovered during scanning:

```go
package main

import (
    "fmt"

    ble "github.com/simpleble/simplegoble"
)

func readBattery(peripheral *ble.Peripheral) error {
    if err := peripheral.Connect(); err != nil {
        return err
    }
    defer peripheral.Disconnect()

    value, err := peripheral.Read("180f", "2a19")
    if err != nil {
        return err
    }
    fmt.Printf("Battery value: %X\n", value)
    return nil
}
```

Use `Services()` to inspect characteristics, their capabilities, and descriptors.
`Read`, `WriteRequest`, and `WriteCommand` access characteristic values;
`ReadDescriptor` and `WriteDescriptor` access descriptors. UUIDs accept short
4/8-digit Bluetooth identifiers or full 128-bit UUIDs.

`Notify` and `Indicate` deliver Go-owned byte slices to your handler.
`Unsubscribe` stops a subscription. `ManufacturerData` returns advertisement
payloads indexed by company ID.

Run the scan and read examples:

```sh
go run github.com/simpleble/simplegoble/examples/scan
go run github.com/simpleble/simplegoble/examples/read ADDRESS 180f 2a19
```

For backend-specific enumeration, call `GetBackends()` and then
`backend.Adapters()`. Close every backend, adapter, and peripheral handle when
finished. Peripheral handles remain usable after the adapter closes. `Close`
releases a handle; call `Disconnect` to end a connection. Client callbacks run serially
on each wrapper's Go worker and can call its methods. Keep handlers short.
Methods are safe for concurrent use.

## Host a GATT service

Create a local peripheral from an adapter, add services and characteristics,
then start advertising:

```go
func hostBattery(adapter *ble.Adapter, done <-chan struct{}) error {
    host, err := adapter.CreateLocalPeripheral()
    if err != nil {
        return err
    }
    defer host.Close()
    if err := host.AddAdvertisedService("180f"); err != nil {
        return err
    }
    service, err := host.AddService("180f")
    if err != nil {
        return err
    }
    defer service.Close()
    battery, err := service.AddCharacteristic("2a19", ble.CapabilityRead|ble.CapabilityNotify)
    if err != nil {
        return err
    }
    defer battery.Close()
    if err := battery.SetValue([]byte{100}); err != nil {
        return err
    }
    if err := host.Start(); err != nil {
        return err
    }
    defer host.Stop()
    <-done
    return nil
}
```

`SetValue` updates the stored characteristic value and publishes it to subscribers.
Observe writes with `SetCallbackOnWrite` and clients with
`SetCallbackOnClientConnected` and `SetCallbackOnClientDisconnected`.
Configure services before `Start`; call `Stop` before changing the GATT layout.
Close each local handle when finished. Hosting support follows the native backend.

Run the echo server:

```sh
go run github.com/simpleble/simplegoble/examples/peripheral
```

See the [usage guide](https://docs.simpleble.org/simplegoble/usage) for dynamic
reads, subscription callbacks, and handle ownership.

## Use a Dongl

A [Dongl](https://docs.simpleble.org/dongl) is a USB Bluetooth adapter made for
SimpleBLE. Enable its backend before any other SimpleGoBLE call, then use the
adapters of the backend whose identifier is `Dongl`:

```go
func donglAdapters() ([]*ble.Adapter, error) {
    if err := ble.Config.Dongl.SetUseDonglBackend(true); err != nil {
        return nil, err
    }
    backends, err := ble.GetBackends()
    if err != nil {
        return nil, err
    }
    defer func() {
        for _, backend := range backends {
            _ = backend.Close()
        }
    }()
    for _, backend := range backends {
        name, err := backend.Identifier()
        if err != nil {
            return nil, err
        }
        if name == "Dongl" {
            return backend.Adapters()
        }
    }
    return nil, nil
}
```

`ble.Config` also holds the BlueZ and WinRT backend settings. Pairing through a
Dongl happens without the operating system: register
`SetCallbackOnPasskeyRequest`, `SetCallbackOnPasskeyDisplay`, or
`SetCallbackOnNumericComparison` on the peripheral before `Connect`.

```sh
go run github.com/simpleble/simplegoble/examples/dongl [ADDRESS]
```

## Platforms

SimpleGoBLE supports Windows, macOS, and Linux on amd64 and arm64.
See the [usage guide](https://docs.simpleble.org/simplegoble/usage) for Bluetooth
permissions and platform requirements.

## Versions

SimpleGoBLE follows SimpleBLE's release versions. Pin a stable release or use
the latest development build:

```sh
go get github.com/simpleble/simplegoble@v1.2.1
go get github.com/simpleble/simplegoble@main
```

Stable tags use `vX.Y.Z`; development tags use `vX.Y.Z-devN`. Each module includes
`VERSION` and `SOURCE_REVISION` to identify its native build and source commit.
Go selects stable releases for `@latest`.

## Documentation and support

- Browse the [Go API reference](https://pkg.go.dev/github.com/simpleble/simplegoble).
- Read the [SimpleGoBLE usage guide](https://docs.simpleble.org/simplegoble/usage).
- Join the [Discord community](https://discord.gg/N9HqNEcvP3).
- Browse the [runnable examples](https://github.com/simpleble/simpleble/tree/main/examples/simplegoble).
- Contribute bindings and tests in [simpleble/simpleble](https://github.com/simpleble/simpleble/tree/main/simplegoble); see the [development and publishing guide](https://github.com/simpleble/simpleble/blob/main/docs/extras/simplegoble-publishing.md).
- Visit [simpleble.org](https://simpleble.org/) for commercial licensing and professional services.

## License

SimpleGoBLE follows the [SimpleBLE license](https://github.com/simpleble/simpleble/blob/main/LICENSE.md),
the Business Source License 1.1 (BUSL-1.1). See the included [LICENSE](LICENSE)
for the complete terms.

---

**SimpleBLE** is a project powered by [The California Open Source Company](https://californiaopensource.com/).
