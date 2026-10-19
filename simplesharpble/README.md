![SimpleBLE](https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo-dark.png)

# SimpleSharpBLE

**Build Bluetooth once. Ship it everywhere.**

SimpleSharpBLE brings [SimpleBLE](https://github.com/simpleble/simpleble) to C#
and .NET on Windows, macOS, Linux, Android, iOS/iPadOS, and Mac Catalyst.
Discover and connect Bluetooth peripherals, read and write GATT values,
subscribe to notifications, and host local GATT services through one API, with
async methods and NativeAOT support included.

## Why SimpleSharpBLE

- **One cross-platform API:** Keep your Bluetooth apps, tools, and services the same on Windows, macOS, Linux, Android, iOS, and Mac Catalyst.
- **Native underneath:** Use WinRT, CoreBluetooth, BlueZ, and Android Bluetooth through one maintained stack.
- **Built for real products:** Act as a central or a peripheral on every supported platform.
- **Commercially supported:** Ship proprietary applications with commercial licensing and direct technical support from the team that builds SimpleBLE.

## Support and resources

- **Documentation:** The [usage guide](https://docs.simpleble.org/simplesharpble/usage?utm_source=github&utm_medium=referral&utm_campaign=simplesharpble_readme&utm_content=support) and [examples](https://github.com/simpleble/simpleble/tree/main/examples/simplesharpble) cover every API.
- **Commercial support:** Visit [simpleble.org](https://simpleble.org?utm_source=github&utm_medium=referral&utm_campaign=simplesharpble_readme&utm_content=support) or [email us](mailto:contact@simpleble.org) about licensing and professional services.
- **Community:** Ask questions and share projects in [GitHub Discussions](https://github.com/simpleble/simpleble/discussions).
- **Dongl:** Prototype and ship with the [official SimpleBLE USB adapter](https://www.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simplesharpble_readme&utm_content=support).
- **Contribute:** Send changes to [simpleble/simpleble](https://github.com/simpleble/simpleble/tree/main/simplesharpble); the [publishing guide](https://github.com/simpleble/simpleble/blob/main/docs/extras/simplesharpble-publishing.md) covers builds and releases.

## Get started

Use .NET 10 or newer:

```sh
dotnet add package SimpleSharpBLE
```

The package includes the native SimpleBLE library for every supported platform,
so nothing is compiled during installation.

Scan for five seconds and print what was found:

```csharp
using SimpleSharpBLE;

var adapter = Adapter.GetAdapters()[0];

adapter.ScanFound += (_, e) =>
{
    using var peripheral = e.Peripheral;
    Console.WriteLine($"{peripheral.Identifier} [{peripheral.Address}] {peripheral.Rssi} dBm");
};
adapter.ScanFor(TimeSpan.FromSeconds(5));
```

## Permissions

On Android, declare the Bluetooth permissions in your app's manifest and
request them at runtime before using an adapter. `BLUETOOTH_ADVERTISE` is only
needed to host a GATT service:

```xml
<uses-permission
    android:name="android.permission.BLUETOOTH_SCAN"
    android:usesPermissionFlags="neverForLocation" />
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT" />
<uses-permission android:name="android.permission.BLUETOOTH_ADVERTISE" />
```

Without `neverForLocation`, Android delivers scan results only to apps that also
hold `ACCESS_FINE_LOCATION`. On iOS/iPadOS and Mac Catalyst, add an
`NSBluetoothAlwaysUsageDescription` entry to your app's `Info.plist`; sandboxed
Mac Catalyst apps also need the `com.apple.security.device.bluetooth`
entitlement. The system asks the user the first time your app uses Bluetooth.

## Scanning and events

An adapter raises the `ScanStarted`, `ScanStopped`, `ScanFound`, `ScanUpdated`,
`PoweredOn`, and `PoweredOff` events; a peripheral raises `Connected` and
`Disconnected`. Remove a handler with `-=`. Handlers run on the .NET thread
pool, one at a time for each object, so hand longer work to your own tasks.

Use `ScanStart()` and `ScanStop()` when your application controls scan timing,
and `ScanGetResults()` for the peripherals seen by the last scan. Adapters and
peripherals, including the one each scan event carries, are `IDisposable`; the
[usage guide](https://docs.simpleble.org/simplesharpble/usage?utm_source=github&utm_medium=referral&utm_campaign=simplesharpble_readme&utm_content=scanning) covers ownership.

## Connect and use GATT

Read the battery level of a peripheral and follow its notifications:

```csharp
const string BatteryService = "0000180f-0000-1000-8000-00805f9b34fb";
const string BatteryLevel = "00002a19-0000-1000-8000-00805f9b34fb";

var peripheral = adapter.ScanGetResults().First(p => p.Identifier == "My Sensor");

peripheral.Disconnected += (_, _) => Console.WriteLine("Disconnected");
peripheral.Connect();

byte[] level = peripheral.Read(BatteryService, BatteryLevel);
Console.WriteLine($"Battery: {level[0]}%");

peripheral.Notify(BatteryService, BatteryLevel, value => Console.WriteLine($"Battery: {value[0]}%"));
```

`Services` lists the services, characteristics, capabilities, and descriptors
of a connected peripheral. `Read`, `WriteRequest`, and `WriteCommand` access
characteristic values; the `Read` and `Write` overloads that take a descriptor
UUID access descriptors. `Notify` and `Indicate` call your function with each
value as a `byte[]`, and `Unsubscribe` stops them. Failures throw
`BleException` with SimpleBLE's message and a `BleErrorCode`.

## Host a GATT service

Create a local peripheral from an adapter, add services and characteristics,
then start advertising:

```csharp
using SimpleSharpBLE.Local;

var host = adapter.CreateLocalPeripheral();
host.AddAdvertisedService(BatteryService);

var service = host.AddService(BatteryService);
var battery = service.AddCharacteristic(
    BatteryLevel, CharacteristicCapability.Read | CharacteristicCapability.Notify);
battery.Value = [100];

host.ClientConnected += (_, e) => Console.WriteLine($"{e.Address} connected");
host.Start();
```

Setting `Value` updates the stored value and publishes it to subscribers.
Characteristics raise `Written`, `Subscribed`, and `Unsubscribed`, and a
`ReadHandler` computes a value for each read. Configure services before
`Start()`; call `Stop()` before changing them.

## Use a Dongl

A [Dongl](https://docs.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simplesharpble_readme&utm_content=dongl) is a USB Bluetooth adapter made for
SimpleBLE. On Windows, macOS, and Linux its backend is on by default; use the
adapters of the Dongl backend:

```csharp
var adapter = Backend.GetDongl().Adapters[0];
```

`Config` holds the Dongl, BlueZ, WinRT, and Android backend settings; set them
before the first call that reaches a backend. Pairing through a Dongl happens
without the operating system: register a callback with
`SetPasskeyRequestCallback()`, `SetPasskeyDisplayCallback()`, or
`SetNumericComparisonCallback()` from `Advanced.Dongl` before `Connect()`.

## MAUI and mobile

On Android, initialize SimpleSharpBLE once on the main thread, after the
permissions are granted and Bluetooth is on:

```csharp
Java.Lang.JavaSystem.LoadLibrary("simpleble");
Advanced.Android.Initialize(Java.Interop.JniEnvironment.Runtime.InvocationPointer,
    Android.App.Application.Context.Handle);
```

On iOS/iPadOS and Mac Catalyst, set `SupportedOSPlatformVersion` to at least
15.8 and 16.0 respectively; the package links its native libraries into your
app. In MAUI, call the `Async` counterparts, such as `ScanForAsync`,
`ConnectAsync`, and `ReadAsync`, to keep the UI thread free, and update the UI
from events with `MainThread.BeginInvokeOnMainThread`. The
[usage guide](https://docs.simpleble.org/simplesharpble/usage?utm_source=github&utm_medium=referral&utm_campaign=simplesharpble_readme&utm_content=mobile) covers each
platform's setup, and the
[Android example](https://github.com/simpleble/simpleble/tree/main/examples/simplesharpble-android)
is a complete app.

## Platforms

| Platform | Architectures | Requirements |
| --- | --- | --- |
| macOS 14 or newer | arm64 | Bluetooth permission for the app that runs your code |
| Windows 10 or newer | x64, arm64 | |
| Linux with glibc | x64, arm64 | BlueZ and the D-Bus library |
| Android 12 or newer | arm64, x64 | Nearby devices permission |
| iOS/iPadOS 15.8 or newer | arm64, including the simulator | `NSBluetoothAlwaysUsageDescription` |
| Mac Catalyst 16.0 or newer | arm64 | `NSBluetoothAlwaysUsageDescription` |

See the [usage guide](https://docs.simpleble.org/simplesharpble/usage?utm_source=github&utm_medium=referral&utm_campaign=simplesharpble_readme&utm_content=platforms) for
Bluetooth permissions and platform details.

## Versions

SimpleSharpBLE follows SimpleBLE's release versions. `dotnet add package`
installs the latest stable release, and development builds of the main branch
are published as pre-releases:

```sh
dotnet add package SimpleSharpBLE --prerelease
```

## License

SimpleSharpBLE follows the [SimpleBLE license](https://github.com/simpleble/simpleble/blob/main/LICENSE.md), the Business
Source License 1.1 (BUSL-1.1). It is free for non-commercial use, and commercial use needs a commercial license. We offer
free commercial licenses for small projects and early-stage companies: [email us](mailto:contact@simpleble.org) or
[leave us a message](https://www.simpleble.org/contact?utm_source=github&utm_medium=referral&utm_campaign=simplesharpble_readme&utm_content=license)
to discuss your use case. Commercial licenses carry no royalties or revenue sharing; see the
[licensing page](https://docs.simpleble.org/licensing?utm_source=github&utm_medium=referral&utm_campaign=simplesharpble_readme&utm_content=license) for terms.

---

**SimpleBLE** is a project powered by [The California Open Source Company](https://californiaopensource.com?utm_source=github&utm_medium=referral&utm_campaign=simplesharpble_readme&utm_content=footer).
