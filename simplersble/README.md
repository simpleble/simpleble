<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo.png">
    <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo-dark.png" alt="SimpleBLE" width="360">
  </picture>
</p>

# SimpleRsBLE

**Build Bluetooth once. Ship it everywhere.**

SimpleRsBLE brings [SimpleBLE](https://github.com/simpleble/simpleble) to Rust
on Windows, macOS, and Linux. Discover and connect Bluetooth peripherals, read
and write GATT values, subscribe to notifications, and host local GATT services
through one API, with events delivered as async streams.

## Why SimpleRsBLE

- **One cross-platform API:** Keep your Bluetooth applications, tools, and services the same on Windows, macOS, and Linux.
- **Native underneath:** Use WinRT, CoreBluetooth, and BlueZ through one maintained stack.
- **Built for real products:** Act as a central or a peripheral on every supported platform.
- **Commercially supported:** Ship proprietary applications with commercial licensing and direct technical support from the team that builds SimpleBLE.

## Support and resources

- **Documentation:** The [usage guide](https://docs.simpleble.org/simplersble/usage?utm_source=github&utm_medium=referral&utm_campaign=simplersble_readme&utm_content=support) and [examples](https://github.com/simpleble/simpleble/tree/main/examples/simplersble) cover every API.
- **Commercial support:** Visit [simpleble.org](https://simpleble.org?utm_source=github&utm_medium=referral&utm_campaign=simplersble_readme&utm_content=support) or [email us](mailto:contact@simpleble.org) about licensing and professional services.
- **Community:** Ask questions and share projects in [GitHub Discussions](https://github.com/simpleble/simpleble/discussions).
- **Dongl:** Prototype and ship with the [official SimpleBLE USB adapter](https://www.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simplersble_readme&utm_content=support).
- **Contribute:** Send changes to [simpleble/simpleble](https://github.com/simpleble/simpleble/tree/main/simplersble).

## Get started

Use Rust 1.82 or newer and CMake 3.21 or newer:

```sh
cargo add simplersble futures tokio --features tokio/full
```

The crate compiles SimpleBLE with CMake during `cargo build` and links it
statically into your program. On Linux, the build also needs the D-Bus
development headers (`libdbus-1-dev` on Debian and Ubuntu).

Scan for five seconds and print what was found:

```rust
use std::time::Duration;

use futures::StreamExt;
use simplersble::{Adapter, ScanEvent};

#[tokio::main]
async fn main() {
    let adapter = Adapter::get_adapters().unwrap().remove(0);

    let mut events = adapter.on_scan_event();
    tokio::spawn(async move {
        while let Some(event) = events.next().await {
            if let ScanEvent::Found(peripheral) = event {
                println!(
                    "{} [{}] {} dBm",
                    peripheral.identifier().unwrap(),
                    peripheral.address().unwrap(),
                    peripheral.rssi().unwrap()
                );
            }
        }
    });
    adapter.scan_for(Duration::from_secs(5)).unwrap();
}
```

## Scanning and events

An adapter streams `ScanEvent::Start`, `Stop`, `Found`, and `Updated` from
`on_scan_event()`, and `PowerEvent::On` and `Off` from `on_power_event()`; a
peripheral streams `ConnectionEvent::Connected` and `Disconnected` from
`on_connection_event()`. Each method returns a `futures::Stream` of events that
you read in your own task, so SimpleBLE's threads never wait on your code.
A stream keeps every event until you read it. Drop a stream to stop listening.

Use `scan_start()` and `scan_stop()` when your application controls scan
timing, and `scan_get_results()` for the peripherals seen by the last scan.

## Connect and use GATT

Read the battery level of a peripheral and follow its notifications:

```rust
use simplersble::{ConnectionEvent, ValueChangedEvent};

let battery_service = String::from("0000180f-0000-1000-8000-00805f9b34fb");
let battery_level = String::from("00002a19-0000-1000-8000-00805f9b34fb");

let peripheral = adapter
    .scan_get_results()
    .unwrap()
    .into_iter()
    .find(|p| p.identifier().unwrap() == "My Sensor")
    .unwrap();

let mut connection = peripheral.on_connection_event();
tokio::spawn(async move {
    while let Some(event) = connection.next().await {
        if let ConnectionEvent::Disconnected = event {
            println!("Disconnected");
        }
    }
});
peripheral.connect().unwrap();

let level = peripheral.read(&battery_service, &battery_level).unwrap();
println!("Battery: {}%", level[0]);

let mut updates = peripheral.notify(&battery_service, &battery_level).unwrap();
tokio::spawn(async move {
    while let Some(ValueChangedEvent::ValueUpdated(value)) = updates.next().await {
        println!("Battery: {}%", value[0]);
    }
});
```

`services()` lists the services, characteristics, capabilities, and descriptors
of a connected peripheral. `read`, `write_request`, and `write_command` access
characteristic values; `descriptor_read` and `descriptor_write` access
descriptors. `notify` and `indicate` return a stream with each value as a
`Vec<u8>`, and `unsubscribe` ends it. Calls that can fail return
`Result<_, simplersble::Error>`, which carries SimpleBLE's message and works
with `?`; its `code()` returns an `ErrorCode`, such as
`ErrorCode::PeripheralNotConnected`.

## Host a GATT service

Create a local peripheral from an adapter, add services and characteristics,
then start advertising:

```rust
use simplersble::{CharacteristicCapability, ClientEvent};

let host = adapter.create_local_peripheral().unwrap();
host.add_advertised_service(&battery_service).unwrap();

let service = host.add_service(&battery_service).unwrap();
let battery = service
    .add_characteristic(
        &battery_level,
        &[CharacteristicCapability::Read, CharacteristicCapability::Notify],
    )
    .unwrap();
battery.set_value(&[100]).unwrap();

let mut clients = host.on_client_event();
tokio::spawn(async move {
    while let Some(event) = clients.next().await {
        if let ClientEvent::Connected(address) = event {
            println!("{} connected", address);
        }
    }
});
host.start().unwrap();
```

`set_value()` updates the stored value and publishes it to subscribers. A
characteristic's `on_event()` streams `LocalCharacteristicEvent::Written`,
`Subscribed`, and `Unsubscribed`, and `set_callback_on_read()` computes a value
for each read. Configure services before `start()`; call `stop()` before
changing them.

## Use a Dongl

A [Dongl](https://docs.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simplersble_readme&utm_content=dongl) is a USB Bluetooth adapter made for
SimpleBLE. Its backend is on by default; use the adapters of the Dongl backend:

```rust
let adapter = simplersble::Backend::get_dongl().unwrap().adapters().unwrap().remove(0);
```

`simplersble::config` holds the Dongl, BlueZ, and WinRT backend settings; set
them before the first call that reaches a backend. Pairing through a Dongl
happens without the operating system: register a callback with
`set_passkey_request_callback()`, `set_passkey_display_callback()`, or
`set_numeric_comparison_callback()` from `simplersble::advanced::dongl` before
`connect()`.

## Platforms

| Platform | Architectures | Requirements |
| --- | --- | --- |
| macOS 15.0 or newer | x64, arm64 | Xcode Command Line Tools and Bluetooth permission for the app that runs your program |
| Windows 10 or newer | x64, arm64 | Visual Studio 2019 16.11 or newer and Windows SDK 10.0.22621.0 or newer |
| Linux | x64, arm64 | BlueZ and the D-Bus development headers |

See [Bluetooth permissions](https://docs.simpleble.org/fundamentals/permissions?utm_source=github&utm_medium=referral&utm_campaign=simplersble_readme&utm_content=platforms)
for the setup each platform needs.

## Versions

SimpleRsBLE follows SimpleBLE's release versions. Cargo picks the latest stable
release, and development builds of the main branch are published as
pre-releases named `X.Y.Z-dev.N`. Pin one by its exact version to use it:

```sh
cargo add simplersble@=X.Y.Z-dev.N
```

## License

SimpleRsBLE follows the [SimpleBLE license](https://github.com/simpleble/simpleble/blob/main/LICENSE.md), the Business
Source License 1.1 (BUSL-1.1). It is free for non-commercial use, and commercial use needs a commercial license. We offer
free commercial licenses for small projects and early-stage companies: [email us](mailto:contact@simpleble.org) or
[leave us a message](https://www.simpleble.org/contact?utm_source=github&utm_medium=referral&utm_campaign=simplersble_readme&utm_content=license)
to discuss your use case. Commercial licenses carry no royalties or revenue sharing; see the
[licensing page](https://docs.simpleble.org/licensing?utm_source=github&utm_medium=referral&utm_campaign=simplersble_readme&utm_content=license) for terms.

---

**SimpleBLE** is a project powered by [The California Open Source Company](https://californiaopensource.com?utm_source=github&utm_medium=referral&utm_campaign=simplersble_readme&utm_content=footer).
