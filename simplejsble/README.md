<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo.png">
    <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo-dark.png" alt="SimpleBLE" width="360">
  </picture>
</p>

# SimpleJSBLE

**Build Bluetooth once. Ship it everywhere.**

SimpleJSBLE brings [SimpleBLE](https://github.com/simpleble/simpleble) to Node.js
and Electron on Windows, macOS, and Linux. Discover and connect Bluetooth
peripherals, read and write GATT values, subscribe to notifications, and host
local GATT services through one API, with TypeScript types included.

## Why SimpleJSBLE

- **One cross-platform API:** Keep your Bluetooth code the same in Node.js and Electron on Windows, macOS, and Linux.
- **Native underneath:** Use WinRT, CoreBluetooth, and BlueZ through one maintained stack.
- **Built for real products:** Act as a central or a peripheral on every supported platform.
- **Commercially supported:** Ship proprietary applications with commercial licensing and direct technical support from the team that builds SimpleBLE.

## Support and resources

- **Documentation:** The [usage guide](https://docs.simpleble.org/simplejsble/usage?utm_source=github&utm_medium=referral&utm_campaign=simplejsble_readme&utm_content=support) and [examples](https://github.com/simpleble/simpleble/tree/main/examples/simplejsble) cover every API.
- **Commercial support:** Visit [simpleble.org](https://simpleble.org/?utm_source=github&utm_medium=referral&utm_campaign=simplejsble_readme&utm_content=support) or [email us](mailto:contact@simpleble.org) about licensing and professional services.
- **Community:** Ask questions and share projects in [GitHub Discussions](https://github.com/simpleble/simpleble/discussions).
- **Dongl:** Prototype and ship with the [official SimpleBLE USB adapter](https://www.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simplejsble_readme&utm_content=support).
- **Contribute:** Send changes to [simpleble/simpleble](https://github.com/simpleble/simpleble/tree/main/simplejsble); the [publishing guide](https://github.com/simpleble/simpleble/blob/main/docs/extras/simplejsble-publishing.md) covers builds and releases.

## Get started

Use Node.js 22 or newer:

```sh
npm install simplejsble
```

The package includes the native addon for every supported platform, so nothing
is compiled or downloaded during installation.

Scan for five seconds and print what was found:

```js
import { Adapter } from 'simplejsble';

const [adapter] = await Adapter.getAdapters();

adapter.on('scanFound', peripheral => {
  console.log(`${peripheral.identifier()} [${peripheral.address()}] ${peripheral.rssi()} dBm`);
});
await adapter.scanFor(5000);
```

## Scanning and events

Adapters and peripherals are `EventEmitter`s. An adapter emits `scanStart`,
`scanStop`, `scanFound`, `scanUpdated`, `powerOn`, and `powerOff`; a peripheral
emits `connected` and `disconnected`. Events arrive on the JavaScript thread in
the order they happened.

Use `scanStart()` and `scanStop()` when your application controls scan timing,
and `scanGetResults()` for the peripherals seen by the last scan. An adapter
always gives you the same `Peripheral` object for the same device, whether it
came from an event or from a list.

Calls that talk to the Bluetooth stack return promises and run in the order
they were made. Data the library already holds, such as a peripheral's
`identifier()`, `rssi()`, or `manufacturerData()`, is returned directly. An
active scan or connection keeps Node.js running until it ends.

## Connect and use GATT

Read the battery level of a peripheral and follow its notifications:

```js
const BATTERY_SERVICE = '0000180f-0000-1000-8000-00805f9b34fb';
const BATTERY_LEVEL = '00002a19-0000-1000-8000-00805f9b34fb';

const peripheral = adapter.scanGetResults().find(p => p.identifier() === 'My Sensor');

peripheral.on('disconnected', () => console.log('Disconnected'));
await peripheral.connect();

const level = await peripheral.read(BATTERY_SERVICE, BATTERY_LEVEL);
console.log(`Battery: ${level[0]}%`);

await peripheral.notify(BATTERY_SERVICE, BATTERY_LEVEL, value => console.log(`Battery: ${value[0]}%`));
```

`services()` lists the services, characteristics, capabilities, and descriptors
of a connected peripheral. `read`, `writeRequest`, and `writeCommand` access
characteristic values; `readDescriptor` and `writeDescriptor` access
descriptors. `notify` and `indicate` call your function with each value as a
`Buffer`, and `unsubscribe` stops them. Failures are `SimpleBLEError`s with
SimpleBLE's error code in `error.code`.

## Host a GATT service

Create a local peripheral from an adapter, add services and characteristics,
then start advertising:

```js
const host = adapter.createLocalPeripheral();
host.addAdvertisedService(BATTERY_SERVICE);

const service = host.addService(BATTERY_SERVICE);
const battery = service.addCharacteristic(BATTERY_LEVEL, ['read', 'notify']);
await battery.setValue(Buffer.from([100]));

host.on('clientConnected', address => console.log(`${address} connected`));
await host.start();
```

`setValue` updates the stored value and publishes it to subscribers. Characteristics
emit `write`, `subscribed`, and `unsubscribed`, and `setReadHandler` computes a
value for each read. Configure services before `start()`; call `stop()` before
changing them.

## Use a Dongl

A [Dongl](https://docs.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simplejsble_readme&utm_content=dongl) is a USB Bluetooth adapter made for
SimpleBLE. Its backend is on by default; use the adapters of the Dongl backend:

```js
import { Backend } from 'simplejsble';

const dongl = await Backend.getDongl();
const [adapter] = await dongl.adapters();

// ...

await adapter.close();
```

`close()` stops the adapter's scan, disconnects its peripherals, and frees the
Dongl for other programs. Adapters also support `await using`.

`Config` holds the Dongl, BlueZ, and WinRT backend settings; set them before the
first call that reaches a backend. Pairing through a Dongl happens without the
operating system: set `setPasskeyRequestHandler`, `setPasskeyDisplayHandler`, or
`setNumericComparisonHandler` on the peripheral before `connect()`.

## Electron

Use SimpleJSBLE from the main process and pass plain data to renderers over
IPC. When packaging, unpack `*.node` files from the ASAR archive. Packaged macOS
apps need an `NSBluetoothAlwaysUsageDescription` entry in their `Info.plist`.

## Platforms

| Platform | Architectures | Requirements |
| --- | --- | --- |
| macOS 13.0 or newer | x64, arm64 | Bluetooth permission for the app that runs Node.js |
| Windows 10 or newer | x64, arm64 | |
| Linux with glibc 2.28 or newer | x64, arm64 | BlueZ and the D-Bus library |

See [Bluetooth permissions](https://docs.simpleble.org/fundamentals/permissions?utm_source=github&utm_medium=referral&utm_campaign=simplejsble_readme&utm_content=platforms)
for the setup each platform needs.

## Versions

SimpleJSBLE follows SimpleBLE's release versions. Stable releases are published
under the `latest` tag, and development builds of the main branch under `dev`:

```sh
npm install simplejsble@dev
```

## License

SimpleJSBLE follows the [SimpleBLE license](https://github.com/simpleble/simpleble/blob/main/LICENSE.md),
the Business Source License 1.1 (BUSL-1.1). It is free for non-commercial use,
and commercial use needs a commercial license. We offer free commercial licenses
for small projects and early-stage companies: [email us](mailto:contact@simpleble.org)
or [leave us a message](https://www.simpleble.org/contact?utm_source=github&utm_medium=referral&utm_campaign=simplejsble_readme&utm_content=license) to discuss your use
case. Commercial licenses carry no royalties or revenue sharing; see the
[licensing page](https://docs.simpleble.org/licensing?utm_source=github&utm_medium=referral&utm_campaign=simplejsble_readme&utm_content=license) for terms.

---

**SimpleBLE** is a project powered by [The California Open Source Company](https://californiaopensource.com/?utm_source=github&utm_medium=referral&utm_campaign=simplejsble_readme&utm_content=footer).
