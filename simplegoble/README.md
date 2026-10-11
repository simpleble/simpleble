<p align="center">
  <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo-dark.png#gh-light-mode-only" alt="SimpleBLE" width="360">
  <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo.png#gh-dark-mode-only" alt="SimpleBLE" width="360">
</p>

# SimpleGoBLE

**Build Bluetooth once. Ship it everywhere.**

SimpleGoBLE brings [SimpleBLE](https://github.com/simpleble/simpleble) to Go on
Windows, macOS, and Linux. Enumerate Bluetooth backends and adapters, check power
state, control scanning, and receive power and scan callbacks through one API.

## Why SimpleGoBLE

- **One cross-platform API:** Keep your Bluetooth adapter logic consistent across desktop platforms.
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

For backend-specific enumeration, call `GetBackends()` and then
`backend.Adapters()`. Close every backend and adapter you obtain when finished;
each owns its native handle. Callbacks run serially on a Go worker and can call
adapter methods. Keep handlers short. Methods are safe for concurrent use.

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
