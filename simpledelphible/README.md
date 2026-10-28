<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo.png">
    <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo-dark.png" alt="SimpleBLE" width="360">
  </picture>
</p>

# SimpleDelphiBLE

**Build Bluetooth once. Ship it everywhere.**

SimpleDelphiBLE brings [SimpleBLE](https://github.com/simpleble/simpleble) to
Delphi and Free Pascal on Windows, macOS, and Linux. Discover and connect
Bluetooth peripherals, read and write GATT values, subscribe to notifications,
and host local GATT services through one API, with events that reach your VCL,
FMX, or Lazarus forms on the main thread.

## Why SimpleDelphiBLE

- **One cross-platform API:** Keep your Bluetooth client and hosting logic the same on Windows, macOS, and Linux, in Delphi and in Lazarus.
- **Native underneath:** Use WinRT, CoreBluetooth, and BlueZ through one maintained stack.
- **Built for real products:** Act as a central or a peripheral on every supported platform.
- **Commercially supported:** Ship proprietary applications with commercial licensing and direct technical support from the team that builds SimpleBLE.

## Support and resources

- **Documentation:** The [usage guide](https://docs.simpleble.org/simpledelphible/usage?utm_source=github&utm_medium=referral&utm_campaign=simpledelphible_readme&utm_content=support) and [examples](https://github.com/simpleble/simpleble/tree/main/examples/simpledelphible) cover every API.
- **Commercial support:** Visit [simpleble.org](https://simpleble.org/?utm_source=github&utm_medium=referral&utm_campaign=simpledelphible_readme&utm_content=support) or [email us](mailto:contact@simpleble.org) about licensing and professional services.
- **Community:** Ask questions and share projects in [GitHub Discussions](https://github.com/simpleble/simpleble/discussions).
- **Dongl:** Prototype and ship with the [official SimpleBLE USB adapter](https://www.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simpledelphible_readme&utm_content=support).
- **Contribute:** Send changes to [simpleble/simpleble](https://github.com/simpleble/simpleble/tree/main/simpledelphible); the [development guide](https://github.com/simpleble/simpleble/blob/main/docs/extras/simpledelphible-development.md) covers builds and tests.

## Get started

Use Delphi 11 or newer, or Free Pascal 3.2.2 or newer. Install with
[Boss](https://github.com/HashLoad/boss):

```sh
boss install github.com/simpleble/simpledelphible
```

or download a [release](https://github.com/simpleble/simpledelphible/releases)
and add its `source` folder to your search path. Both include the native
SimpleBLE libraries for every supported platform, prebuilt in `lib`; ship the
ones for your target next to your executable.

Scan for five seconds and print what was found:

```pascal
program scan;

{$APPTYPE CONSOLE}

uses
  SimpleDelphiBLE;

type
  TPrinter = class
    procedure ScanFound(Sender: TBleAdapter; Peripheral: TBlePeripheral);
  end;

procedure TPrinter.ScanFound(Sender: TBleAdapter; Peripheral: TBlePeripheral);
begin
  WriteLn(Peripheral.Identifier, ' [', Peripheral.Address, '] ', Peripheral.Rssi, ' dBm');
end;

var
  Adapter: TBleAdapter;
  Printer: TPrinter;

begin
  Adapter := TBleAdapter.GetAdapters[0];
  Printer := TPrinter.Create;
  Adapter.OnScanFound := Printer.ScanFound;
  Adapter.ScanFor(5000);
end.
```

## Scanning and events

An adapter has the events `OnScanStart`, `OnScanStop`, `OnScanFound`,
`OnScanUpdated`, `OnPowerOn`, and `OnPowerOff`; a peripheral has `OnConnected`
and `OnDisconnected`. In VCL, FMX, and Lazarus applications, handlers run on
the main thread, so they can update your forms directly. In console programs,
they run in order on SimpleDelphiBLE's event thread.

Use `ScanStart` and `ScanStop` when your application controls scan timing, and
`ScanGetResults` for the peripherals seen by the last scan. An adapter always
gives you the same `TBlePeripheral` object for the same device, whether it came
from an event or from a list. SimpleDelphiBLE owns adapters and peripherals, so
you never free them.

## Connect and use GATT

Read the battery level of a peripheral and follow its notifications:

```pascal
const
  BatteryService = '0000180f-0000-1000-8000-00805f9b34fb';
  BatteryLevel = '00002a19-0000-1000-8000-00805f9b34fb';

procedure TBatteryMonitor.Follow(Peripheral: TBlePeripheral);
begin
  Peripheral.OnDisconnected := PeripheralDisconnected;
  Peripheral.Connect;
  WriteLn('Battery: ', Peripheral.Read(BatteryService, BatteryLevel)[0], '%');
  Peripheral.Notify(BatteryService, BatteryLevel, BatteryChanged);
end;

procedure TBatteryMonitor.BatteryChanged(Sender: TBlePeripheral; const Service, Characteristic: string;
  const Data: TBytes);
begin
  WriteLn('Battery: ', Data[0], '%');
end;
```

`Services` lists the services, characteristics, capabilities, and descriptors
of a connected peripheral. `Read`, `WriteRequest`, and `WriteCommand` access
characteristic values; `ReadDescriptor` and `WriteDescriptor` access
descriptors. `Notify` and `Indicate` call your handler with each value as
`TBytes`, and `Unsubscribe` stops them. Calls like `Connect` and `Read` wait for
the device, so GUI applications make them from a worker thread. Failures raise
an `EBleError` subclass, such as `EBleNotConnected`, with SimpleBLE's error code
in `Code`.

## Host a GATT service

Create a local peripheral from an adapter, add services and characteristics,
then start advertising:

```pascal
Host := Adapter.CreateLocalPeripheral;
Host.AddAdvertisedService(BatteryService);

Battery := Host.AddService(BatteryService).AddCharacteristic(BatteryLevel, [lcRead, lcNotify]);
Battery.SetValue(TBytes.Create(100));

Host.OnClientConnected := ClientConnected;
Host.Start;
```

`SetValue` updates the stored value and publishes it to subscribers.
Characteristics have the events `OnWrite`, `OnSubscribed`, and
`OnUnsubscribed`, and `OnReadRequest` supplies a value for each read. Configure
services before `Start`; call `Stop` before changing them. Your application
owns the local peripheral: stop it, then free it.

## Use a Dongl

A [Dongl](https://docs.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simpledelphible_readme&utm_content=dongl) is a USB Bluetooth adapter made for
SimpleBLE. Its backend is on by default; use the adapters of the Dongl backend:

```pascal
Adapter := TBleBackend.GetDongl.GetAdapters[0];
```

`TBleConfig` holds the Dongl, BlueZ, and WinRT backend settings; set them
before the first call that reaches a backend. Pairing through a Dongl happens
without the operating system: assign `OnPasskeyRequest`, `OnPasskeyDisplay`, or
`OnNumericComparison` on the peripheral before `Connect`.

## Free Pascal and Lazarus

Compile in Delphi mode with Unicode strings, and on macOS and Linux list
`cthreads` first in your program's `uses` clause:

```pascal
{$MODE DELPHIUNICODE}

uses
  {$IFDEF UNIX}cthreads,{$ENDIF}
  SimpleDelphiBLE;
```

In Lazarus, use the `SimpleDelphiBLEPackage.lpk` runtime package that comes
with Boss and with each release. Open it with **Package > Open Package File
(.lpk)** and choose **Use > Add to Project**, or build it with
`lazbuild SimpleDelphiBLEPackage.lpk` and add `SimpleDelphiBLEPackage` as a
**New Requirement** in the Project Inspector. The package supplies the units
and the library path for your target, so your project needs no search paths or
linker options.

## Platforms

| Platform | Architectures | Compilers | Requirements |
| --- | --- | --- | --- |
| Windows 10 or newer | x86, x64 | Delphi, Free Pascal | |
| macOS 15.0 or newer | x64, arm64 | Delphi, Free Pascal | Bluetooth permission for your app |
| Linux | x64 | Delphi Enterprise or Architect, Free Pascal | BlueZ and the D-Bus library |
| Linux | arm64 | Free Pascal | BlueZ and the D-Bus library |

See [Bluetooth permissions](https://docs.simpleble.org/fundamentals/permissions?utm_source=github&utm_medium=referral&utm_campaign=simpledelphible_readme&utm_content=platforms)
for the setup each platform needs.

## Versions

SimpleDelphiBLE follows SimpleBLE's release versions. Stable releases use
`X.Y.Z`, and development builds of the main branch are published as
`X.Y.Z-dev.N`.

## License

SimpleDelphiBLE follows the [SimpleBLE license](https://github.com/simpleble/simpleble/blob/main/LICENSE.md),
the Business Source License 1.1 (BUSL-1.1). It is free for non-commercial use,
and commercial use needs a commercial license. We offer free commercial licenses
for small projects and early-stage companies: [email us](mailto:contact@simpleble.org)
or [leave us a message](https://www.simpleble.org/contact?utm_source=github&utm_medium=referral&utm_campaign=simpledelphible_readme&utm_content=license) to discuss your use
case. Commercial licenses carry no royalties or revenue sharing; see the
[licensing page](https://docs.simpleble.org/licensing?utm_source=github&utm_medium=referral&utm_campaign=simpledelphible_readme&utm_content=license) for terms.

---

**SimpleBLE** is a project powered by [The California Open Source Company](https://californiaopensource.com/?utm_source=github&utm_medium=referral&utm_campaign=simpledelphible_readme&utm_content=footer).
