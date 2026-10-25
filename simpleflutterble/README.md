<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo.png">
    <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo-dark.png" alt="SimpleBLE" width="360">
  </picture>
</p>

# SimpleFlutterBLE

**Build Bluetooth once. Ship it everywhere.**

SimpleFlutterBLE brings [SimpleBLE](https://github.com/simpleble/simpleble) to
Flutter apps on Android, iOS, macOS, Windows, and Linux. Discover and connect
Bluetooth peripherals, read and write GATT values, subscribe to notifications,
and host local GATT services through one API, with futures and streams. It is
[SimpleDartBLE](https://pub.dev/packages/simpledartble)'s API, with the setup
Flutter apps need on Android.

## Why SimpleFlutterBLE

- **One cross-platform API:** Keep your Bluetooth code the same on Android, iOS, macOS, Windows, and Linux, and in Dart programs on the desktop through SimpleDartBLE.
- **Native underneath:** Use the Android Bluetooth stack, CoreBluetooth, WinRT, and BlueZ through one maintained stack.
- **Built for real products:** Act as a central or a peripheral on every supported platform.
- **Commercially supported:** Ship proprietary applications with commercial licensing and direct technical support from the team that builds SimpleBLE.

## Support and resources

- **Documentation:** The [usage guide](https://docs.simpleble.org/simpleflutterble/usage?utm_source=github&utm_medium=referral&utm_campaign=simpleflutterble_readme&utm_content=support) and [example app](https://github.com/simpleble/simpleble/tree/main/examples/simpleflutterble) cover every API.
- **Commercial support:** Visit [simpleble.org](https://simpleble.org/?utm_source=github&utm_medium=referral&utm_campaign=simpleflutterble_readme&utm_content=support) or [email us](mailto:contact@simpleble.org) about licensing and professional services.
- **Community:** Ask questions and share projects in [GitHub Discussions](https://github.com/simpleble/simpleble/discussions).
- **Dongl:** Prototype and ship with the [official SimpleBLE USB adapter](https://www.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simpleflutterble_readme&utm_content=support).
- **Contribute:** Send changes to [simpleble/simpleble](https://github.com/simpleble/simpleble/tree/main/simpleflutterble); the [development guide](https://github.com/simpleble/simpleble/blob/main/docs/extras/simpledartble-development.md) covers builds and tests.

## Get started

Use Flutter 3.38 or newer:

```sh
flutter pub add simpleflutterble
```

The package includes the native SimpleBLE library for every supported platform,
so nothing is compiled during your build.

Scan for five seconds and print what was found:

```dart
import 'package:simpleflutterble/simpleflutterble.dart';

Future<void> scan() async {
  final adapter = (await Adapter.getAdapters()).first;

  adapter.onScanFound.listen((peripheral) {
    print('${peripheral.identifier} [${peripheral.address}] ${peripheral.rssi} dBm');
  });
  await adapter.scanFor(const Duration(seconds: 5));
}
```

## Permissions

On Android, SimpleFlutterBLE's manifest declares `BLUETOOTH_SCAN`,
`BLUETOOTH_CONNECT`, and `BLUETOOTH_ADVERTISE`. Request them at runtime before
using an adapter, for example with
[permission_handler](https://pub.dev/packages/permission_handler), and set your
app's `minSdk` to 31.

On iOS and macOS, add an `NSBluetoothAlwaysUsageDescription` entry to your
app's `Info.plist`, and on macOS the `com.apple.security.device.bluetooth`
entitlement. The system asks the user the first time your app uses Bluetooth.
Set the deployment target to iOS 15.8 or macOS 15.0.

## Scanning and events

Events are broadcast streams. An adapter has `onScanStart`, `onScanStop`,
`onScanFound`, `onScanUpdated`, `onPowerOn`, and `onPowerOff`; a peripheral has
`onConnected` and `onDisconnected`. Events arrive in the order they happened.

Use `scanStart()` and `scanStop()` when your application controls scan timing,
and `scanGetResults()` for the peripherals seen by the last scan. An adapter
always gives you the same `Peripheral` object for the same device, whether it
came from an event or from a list.

Calls that talk to the Bluetooth stack return futures, run in the order they
were made, and never block your isolate. Data the library already holds, such
as a peripheral's `identifier`, `rssi`, or `manufacturerData()`, is returned
directly.

## Connect and use GATT

Read the battery level of a peripheral and follow its notifications:

```dart
const batteryService = '0000180f-0000-1000-8000-00805f9b34fb';
const batteryLevel = '00002a19-0000-1000-8000-00805f9b34fb';

final peripheral = adapter.scanGetResults().firstWhere((p) => p.identifier == 'My Sensor');

peripheral.onDisconnected.listen((_) => print('Disconnected'));
await peripheral.connect();

final level = await peripheral.read(batteryService, batteryLevel);
print('Battery: ${level[0]}%');

final levels = await peripheral.notify(batteryService, batteryLevel);
levels.listen((value) => print('Battery: ${value[0]}%'));
```

`services()` lists the services, characteristics, capabilities, and descriptors
of a connected peripheral. `read`, `writeRequest`, and `writeCommand` access
characteristic values; `readDescriptor` and `writeDescriptor` access
descriptors. `notify` and `indicate` return a stream of values once subscribed,
which ends on `unsubscribe` or a disconnection. Failures are
`SimpleDartBleException`s with SimpleBLE's error code in `code`.

## Host a GATT service

Create a local peripheral from an adapter, add services and characteristics,
then start advertising:

```dart
final host = adapter.createLocalPeripheral();
host.addAdvertisedService(batteryService);

final battery = host
    .addService(batteryService)
    .addCharacteristic(batteryLevel, {CharacteristicCapability.read, CharacteristicCapability.notify});
await battery.setValue(Uint8List.fromList([100]));

host.onClientConnected.listen((address) => print('$address connected'));
await host.start();
```

`setValue` updates the stored value and publishes it to subscribers.
Characteristics have `onWrite`, `onSubscribed`, and `onUnsubscribed`, and
`setReadHandler` computes a value for each read. Configure services before
`start()`; call `stop()` before changing them.

## Use a Dongl

A [Dongl](https://docs.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simpleflutterble_readme&utm_content=dongl) is a USB Bluetooth adapter made for
SimpleBLE. Its backend is on by default on Windows, macOS, and Linux; use the
adapters of the Dongl backend:

```dart
final dongl = await Backend.getDongl();
final adapter = (await dongl.getAdapters()).first;

// ...

await adapter.close();
```

`close()` stops the adapter's scan, disconnects its peripherals, and frees the
Dongl for other programs.

`Config` holds the Dongl, BlueZ, WinRT, and Android backend settings; set them
before the first call that reaches a backend. Pairing through a Dongl happens
without the operating system: set `setPasskeyRequestHandler`,
`setPasskeyDisplayHandler`, or `setNumericComparisonHandler` on the peripheral
before `connect()`.

## Platforms

| Platform | Architectures | Requirements |
| --- | --- | --- |
| Android 12 (API 31) or newer | arm64-v8a, armeabi-v7a, x86_64 | Runtime Bluetooth permissions |
| iOS 15.8 or newer | arm64, plus arm64 and x64 simulators | `NSBluetoothAlwaysUsageDescription` in `Info.plist` |
| macOS 15.0 or newer | x64, arm64 | `NSBluetoothAlwaysUsageDescription` and the Bluetooth entitlement |
| Windows 10 or newer | x64, arm64 | |
| Linux with glibc 2.28 or newer | x64, arm64 | BlueZ and the D-Bus library |

See [Bluetooth permissions](https://docs.simpleble.org/fundamentals/permissions?utm_source=github&utm_medium=referral&utm_campaign=simpleflutterble_readme&utm_content=platforms)
for the setup each platform needs.

## Versions

SimpleFlutterBLE follows SimpleBLE's release versions, `X.Y.Z`, and uses the
SimpleDartBLE of the same version.

## License

SimpleDartBLE follows the [SimpleBLE license](https://github.com/simpleble/simpleble/blob/main/LICENSE.md),
the Business Source License 1.1 (BUSL-1.1). It is free for non-commercial use,
and commercial use needs a commercial license. We offer free commercial licenses
for small projects and early-stage companies: [email us](mailto:contact@simpleble.org)
or [leave us a message](https://www.simpleble.org/contact?utm_source=github&utm_medium=referral&utm_campaign=simpleflutterble_readme&utm_content=license) to discuss your use
case. Commercial licenses carry no royalties or revenue sharing; see the
[licensing page](https://docs.simpleble.org/licensing?utm_source=github&utm_medium=referral&utm_campaign=simpleflutterble_readme&utm_content=license) for terms.

---

**SimpleBLE** is a project powered by [The California Open Source Company](https://californiaopensource.com/?utm_source=github&utm_medium=referral&utm_campaign=simpleflutterble_readme&utm_content=footer).
