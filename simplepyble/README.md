<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo.png">
    <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo-dark.png" alt="SimpleBLE" width="360">
  </picture>
</p>

# SimplePyBLE

**Build Bluetooth once. Ship it everywhere.**

SimplePyBLE brings [SimpleBLE](https://github.com/simpleble/simpleble) to Python
on Windows, macOS, and Linux. Discover and connect Bluetooth peripherals, read
and write GATT values, subscribe to notifications, and host local GATT services
through one API, with type hints and an asyncio module included.

## Why SimplePyBLE

- **One cross-platform API:** Keep your Bluetooth scripts, tools, and services the same on Windows, macOS, and Linux.
- **Native underneath:** Use WinRT, CoreBluetooth, and BlueZ through one maintained stack.
- **Built for real products:** Act as a central or a peripheral on every supported platform.
- **Commercially supported:** Ship proprietary applications with commercial licensing and direct technical support from the team that builds SimpleBLE.

## Support and resources

- **Documentation:** The [usage guide](https://docs.simpleble.org/simplepyble/usage?utm_source=github&utm_medium=referral&utm_campaign=simplepyble_readme&utm_content=support) and [examples](https://github.com/simpleble/simpleble/tree/main/examples/simplepyble) cover every API.
- **Commercial support:** Visit [simpleble.org](https://simpleble.org?utm_source=github&utm_medium=referral&utm_campaign=simplepyble_readme&utm_content=support) or [email us](mailto:contact@simpleble.org) about licensing and professional services.
- **Community:** Ask questions and share projects in [GitHub Discussions](https://github.com/simpleble/simpleble/discussions).
- **Dongl:** Prototype and ship with the [official SimpleBLE USB adapter](https://www.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simplepyble_readme&utm_content=support).
- **Contribute:** Send changes to [simpleble/simpleble](https://github.com/simpleble/simpleble/tree/main/simplepyble).

## Get started

Use Python 3.10 or newer:

```sh
pip install simplepyble
```

The wheels include the native SimpleBLE library for every supported platform,
so nothing is compiled during installation.

Scan for five seconds and print what was found:

```python
import simplepyble

adapter = simplepyble.Adapter.get_adapters()[0]

adapter.set_callback_on_scan_found(
    lambda peripheral: print(f"{peripheral.identifier()} [{peripheral.address()}] {peripheral.rssi()} dBm")
)
adapter.scan_for(5000)
```

## Scanning and events

An adapter takes callbacks for `scan_start`, `scan_stop`, `scan_found`,
`scan_updated`, `power_on`, and `power_off` through
`set_callback_on_scan_found()` and its siblings; a peripheral takes
`set_callback_on_connected()` and `set_callback_on_disconnected()`. Pass `None`
to clear a callback. Callbacks run on SimpleBLE's threads, so hand longer work
to your own.

Use `scan_start()` and `scan_stop()` when your application controls scan
timing, and `scan_get_results()` for the peripherals seen by the last scan.

## Connect and use GATT

Read the battery level of a peripheral and follow its notifications:

```python
BATTERY_SERVICE = "0000180f-0000-1000-8000-00805f9b34fb"
BATTERY_LEVEL = "00002a19-0000-1000-8000-00805f9b34fb"

peripheral = next(p for p in adapter.scan_get_results() if p.identifier() == "My Sensor")

peripheral.set_callback_on_disconnected(lambda: print("Disconnected"))
peripheral.connect()

level = peripheral.read(BATTERY_SERVICE, BATTERY_LEVEL)
print(f"Battery: {level[0]}%")

peripheral.notify(BATTERY_SERVICE, BATTERY_LEVEL, lambda value: print(f"Battery: {value[0]}%"))
```

`services()` lists the services, characteristics, capabilities, and descriptors
of a connected peripheral. `read`, `write_request`, and `write_command` access
characteristic values; `descriptor_read` and `descriptor_write` access
descriptors. `notify` and `indicate` call your function with each value as
`bytes`, and `unsubscribe` stops them. Failures raise `RuntimeError` with
SimpleBLE's message.

## Host a GATT service

Create a local peripheral from an adapter, add services and characteristics,
then start advertising:

```python
from simplepyble import LocalCharacteristicCapability

host = adapter.create_local_peripheral()
host.add_advertised_service(BATTERY_SERVICE)

service = host.add_service(BATTERY_SERVICE)
battery = service.add_characteristic(
    BATTERY_LEVEL, [LocalCharacteristicCapability.READ, LocalCharacteristicCapability.NOTIFY]
)
battery.set_value(bytes([100]))

host.set_callback_on_client_connected(lambda address: print(f"{address} connected"))
host.start()
```

`set_value()` updates the stored value and publishes it to subscribers.
Characteristics take `set_callback_on_write()`, `set_callback_on_subscribed()`,
and `set_callback_on_unsubscribed()`, and `set_callback_on_read()` computes a
value for each read. Configure services before `start()`; call `stop()` before
changing them.

## Use a Dongl

A [Dongl](https://docs.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simplepyble_readme&utm_content=dongl) is a USB Bluetooth adapter made for
SimpleBLE. Its backend is on by default; use the adapters of the Dongl backend:

```python
adapter = simplepyble.Backend.get_dongl().adapters()[0]
```

`simplepyble.config` holds the Dongl, BlueZ, and WinRT backend settings; set
them before the first call that reaches a backend. Pairing through a Dongl
happens without the operating system: register a callback with
`set_passkey_request_callback()`, `set_passkey_display_callback()`, or
`set_numeric_comparison_callback()` from `simplepyble.advanced.dongl` before
`connect()`. On Linux, set `config.simplebluez.interactive_pairing` and
register the same callbacks for an adapter in `simplepyble.advanced.linux`;
they also receive the peer's address.

## Asyncio

`simplepyble.aio` has the same API for `asyncio`. Calls that wait on the
Bluetooth stack are coroutines, and callbacks can be functions or coroutines;
they run on the event loop that registered them.

```python
import asyncio
from simplepyble.aio import Adapter

async def main():
    adapter = Adapter.get_adapters()[0]

    adapter.set_callback_on_scan_found(
        lambda peripheral: print(f"{peripheral.identifier()} [{peripheral.address()}] {peripheral.rssi()} dBm")
    )
    async with adapter:
        await adapter.scan_for(5000)

asyncio.run(main())
```

Leaving `async with` on an adapter, peripheral, or local peripheral clears its
callbacks and stops its scan, subscriptions, or advertising.

## Platforms

| Platform | Architectures | Requirements |
| --- | --- | --- |
| macOS 15.0 or newer | x64, arm64 | Bluetooth permission for the app that runs Python |
| Windows 10 or newer | x64, x86, arm64 | |
| Linux | x64, x86, arm64 | BlueZ and the D-Bus library |

See [Bluetooth permissions](https://docs.simpleble.org/fundamentals/permissions?utm_source=github&utm_medium=referral&utm_campaign=simplepyble_readme&utm_content=platforms)
for the setup each platform needs.

## Versions

SimplePyBLE follows SimpleBLE's release versions. pip installs the latest
stable release, and development builds of the main branch are published as
pre-releases:

```sh
pip install --pre simplepyble
```

## License

SimplePyBLE follows the [SimpleBLE license](https://github.com/simpleble/simpleble/blob/main/LICENSE.md),
the Business Source License 1.1 (BUSL-1.1). It is free for non-commercial use,
and commercial use needs a commercial license. We offer free commercial licenses
for small projects and early-stage companies: [email us](mailto:contact@simpleble.org)
or [leave us a message](https://www.simpleble.org/contact?utm_source=github&utm_medium=referral&utm_campaign=simplepyble_readme&utm_content=license)
to discuss your use case. Commercial licenses carry no royalties or revenue
sharing; see the [licensing page](https://docs.simpleble.org/licensing?utm_source=github&utm_medium=referral&utm_campaign=simplepyble_readme&utm_content=license) for
terms.

---

**SimpleBLE** is a project powered by [The California Open Source Company](https://californiaopensource.com?utm_source=github&utm_medium=referral&utm_campaign=simplepyble_readme&utm_content=footer).
