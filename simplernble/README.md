<p align="center">
  <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo-dark.png#gh-light-mode-only" alt="SimpleBLE" width="360">
  <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo.png#gh-dark-mode-only" alt="SimpleBLE" width="360">
</p>

# SimpleRnBLE

SimpleRnBLE brings [SimpleBLE](https://github.com/simpleble/simpleble) to React
Native apps on iOS and Android. Discover and connect Bluetooth peripherals, read
and write GATT values, subscribe to notifications, and host local GATT services
through one API, with TypeScript types included. It is a
[Nitro module](https://nitro.margelo.com), so calls go straight from JavaScript
to SimpleBLE's C++ core.

## Get started

Install SimpleRnBLE with Nitro Modules, then install the iOS pods:

```sh
npm install simplernble react-native-nitro-modules
cd ios && pod install
```

SimpleBLE comes prebuilt for iOS, and your app's Android build compiles it with
the NDK. In Expo, use a development build.

Declare Bluetooth usage with `NSBluetoothAlwaysUsageDescription` in your iOS
`Info.plist`. On Android, request `BLUETOOTH_SCAN` and `BLUETOOTH_CONNECT` at
runtime, plus `BLUETOOTH_ADVERTISE` to host services; SimpleRnBLE declares them
in its manifest.

Scan for five seconds and print what was found:

```ts
import { Adapter } from 'simplernble';

const [adapter] = await Adapter.getAdapters();

adapter.on('scanFound', peripheral => {
  console.log(peripheral.identifier(), peripheral.address(), peripheral.rssi());
});
await adapter.scanFor(5000);
```

## Scanning and events

Adapters and peripherals emit events, with `on()`, `once()`, and `off()`. An
adapter emits `scanStart`, `scanStop`, `scanFound`, `scanUpdated`, `powerOn`,
and `powerOff`; a peripheral emits `connected` and `disconnected`. Events arrive
on the JavaScript thread in the order they happened.

Use `scanStart()` and `scanStop()` when your app controls scan timing, and
`scanGetResults()` for the peripherals seen by the last scan. An adapter always
gives you the same `Peripheral` object for the same device, whether it came from
an event or from a list, so it works as a React key and in state.

Calls that talk to the Bluetooth stack return promises and run in the order
they were made, without blocking the JavaScript thread. Data the library already
holds, such as a peripheral's `identifier()`, `rssi()`, or `manufacturerData()`,
is returned directly.

## Connect and use GATT

Read the battery level of a peripheral and follow its notifications:

```ts
const BATTERY_SERVICE = '0000180f-0000-1000-8000-00805f9b34fb';
const BATTERY_LEVEL = '00002a19-0000-1000-8000-00805f9b34fb';

const peripheral = adapter.scanGetResults().find(p => p.identifier() === 'My Sensor')!;

peripheral.on('disconnected', () => console.log('Disconnected'));
await peripheral.connect();

const level = await peripheral.read(BATTERY_SERVICE, BATTERY_LEVEL);
console.log(`Battery: ${level[0]}%`);

await peripheral.notify(BATTERY_SERVICE, BATTERY_LEVEL, value => console.log(`Battery: ${value[0]}%`));
```

`services()` lists the services, characteristics, capabilities, and descriptors
of a connected peripheral. `read`, `writeRequest`, and `writeCommand` access
characteristic values; `readDescriptor` and `writeDescriptor` access
descriptors. Values are `Uint8Array`s. `notify` and `indicate` call your
function with each value, and `unsubscribe` stops them. Failures are
`SimpleBLEError`s with SimpleBLE's error code in `error.code`.

## Host a GATT service

Create a local peripheral from an adapter, add services and characteristics,
then start advertising:

```ts
const host = adapter.createLocalPeripheral();
host.addAdvertisedService(BATTERY_SERVICE);

const service = host.addService(BATTERY_SERVICE);
const battery = service.addCharacteristic(BATTERY_LEVEL, ['read', 'notify']);
await battery.setValue(Uint8Array.of(100));

host.on('clientConnected', address => console.log(`${address} connected`));
await host.start();
```

`setValue` updates the stored value and publishes it to subscribers. Characteristics
emit `write`, `subscribed`, and `unsubscribed`, and `setReadHandler` computes a
value for each read. Configure services before `start()`; call `stop()` before
changing them.

## Platforms

| Platform | Requirements |
| --- | --- |
| iOS 15.8 or newer | `NSBluetoothAlwaysUsageDescription` in `Info.plist` |
| Android 12 (API 31) or newer | Runtime Bluetooth permissions; the Android NDK to build |

SimpleRnBLE needs React Native 0.80 or newer with the New Architecture, and
`react-native-nitro-modules` 0.37. `Logging` routes SimpleBLE's log, and
`Config` holds the Android backend settings. See the
[usage guide](https://docs.simpleble.org/simplernble/usage) for permissions,
Expo, and platform details.

## Versions

SimpleRnBLE follows SimpleBLE's release versions. Stable releases are published
under the `latest` tag, and development builds of the main branch under `dev`:

```sh
npm install simplernble@dev
```

## Documentation and support

- Read the [SimpleRnBLE usage guide](https://docs.simpleble.org/simplernble/usage).
- Try the [example app](https://github.com/simpleble/simpleble/tree/main/examples/simplernble).
- Join the [Discord community](https://discord.gg/N9HqNEcvP3).
- Contribute in [simpleble/simpleble](https://github.com/simpleble/simpleble/tree/main/simplernble); see the [development and publishing guide](https://github.com/simpleble/simpleble/blob/main/docs/extras/simplernble-publishing.md).
- Visit [simpleble.org](https://simpleble.org/) for commercial licensing and professional services.

## License

SimpleRnBLE follows the [SimpleBLE license](https://github.com/simpleble/simpleble/blob/main/LICENSE.md),
the Business Source License 1.1 (BUSL-1.1). See the included [LICENSE](LICENSE.md)
for the complete terms.

---

**SimpleBLE** is a project powered by [The California Open Source Company](https://californiaopensource.com/).
