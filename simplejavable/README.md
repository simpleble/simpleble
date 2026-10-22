<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo.png">
    <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo-dark.png" alt="SimpleBLE" width="360">
  </picture>
</p>

# SimpleJavaBLE

**Build Bluetooth once. Ship it everywhere.**

SimpleJavaBLE brings [SimpleBLE](https://github.com/simpleble/simpleble) to Java
and the JVM on Windows, macOS, and Linux. Discover and connect Bluetooth
peripherals, read and write GATT values, subscribe to notifications, and host
local GATT services through one API.

## Why SimpleJavaBLE

- **One cross-platform API:** Keep your Bluetooth code the same on Windows, macOS, and Linux.
- **Native underneath:** Use WinRT, CoreBluetooth, and BlueZ through one maintained stack.
- **Built for real products:** Act as a central or a peripheral on every supported platform.
- **Commercially supported:** Ship proprietary applications with commercial licensing and direct technical support from the team that builds SimpleBLE.

## Support and resources

- **Documentation:** The [usage guide](https://docs.simpleble.org/simplejavable/usage?utm_source=github&utm_medium=referral&utm_campaign=simplejavable_readme&utm_content=support) and [examples](https://github.com/simpleble/simpleble/tree/main/examples/simplejavable) cover every API.
- **Commercial support:** Visit [simpleble.org](https://simpleble.org/?utm_source=github&utm_medium=referral&utm_campaign=simplejavable_readme&utm_content=support) or [email us](mailto:contact@simpleble.org) about licensing and professional services.
- **Community:** Ask questions and share projects in [GitHub Discussions](https://github.com/simpleble/simpleble/discussions).
- **Dongl:** Prototype and ship with the [official SimpleBLE USB adapter](https://www.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simplejavable_readme&utm_content=support).
- **Contribute:** Send changes to [simpleble/simpleble](https://github.com/simpleble/simpleble/tree/main/simplejavable); the [publishing guide](https://github.com/simpleble/simpleble/blob/main/docs/extras/simplejavable-publishing.md) covers builds and releases.

## Get started

Use Java 17 or newer, and add SimpleJavaBLE to your Gradle dependencies:

```kotlin
dependencies {
    implementation("org.simpleble:simplejavable:<version>")
}
```

The JAR includes the native SimpleBLE library for every supported platform, so
nothing is compiled during your build.

Scan for five seconds and print what was found:

```java
import org.simplejavable.Adapter;
import org.simplejavable.Peripheral;

public class Scan {
    public static void main(String[] args) throws Exception {
        Adapter adapter = Adapter.getAdapters().get(0);

        adapter.setEventListener(new Adapter.EventListener() {
            @Override
            public void onScanFound(Peripheral peripheral) {
                System.out.println(peripheral.getIdentifier() + " [" + peripheral.getAddress() + "] "
                        + peripheral.getRssi() + " dBm");
            }
        });
        adapter.scanFor(5000);
    }
}
```

## Scanning and events

An adapter's `Adapter.EventListener` receives `onPowerOn`, `onPowerOff`,
`onScanStart`, `onScanStop`, `onScanFound`, and `onScanUpdated`; a peripheral's
`Peripheral.EventListener` receives `onConnected` and `onDisconnected`.
Implement only the methods you need. Listeners run one at a time on
SimpleJavaBLE's callback thread; hand longer work to your own executor.

Use `scanStart()` and `scanStop()` when your application controls scan timing,
and `scanGetResults()` for the peripherals seen by the last scan.
`getPairedPeripherals()` and `getConnectedPeripherals()` list devices known to
the system. An adapter always gives you the same `Peripheral` object for the
same device, whether it came from an event or from a list. `powerOn()` and
`powerOff()` switch the adapter where the platform allows it, and `isPowered()`
reports its state.

## Connect and use GATT

Read the battery level of a peripheral and follow its notifications:

```java
BluetoothUUID batteryService = new BluetoothUUID("0000180f-0000-1000-8000-00805f9b34fb");
BluetoothUUID batteryLevel = new BluetoothUUID("00002a19-0000-1000-8000-00805f9b34fb");

Peripheral peripheral = adapter.scanGetResults().stream()
        .filter(p -> p.getIdentifier().equals("My Sensor"))
        .findFirst()
        .orElseThrow();

peripheral.setEventListener(new Peripheral.EventListener() {
    @Override
    public void onDisconnected() {
        System.out.println("Disconnected");
    }
});
peripheral.connect();

byte[] level = peripheral.read(batteryService, batteryLevel);
System.out.println("Battery: " + level[0] + "%");

peripheral.notify(batteryService, batteryLevel, value -> System.out.println("Battery: " + value[0] + "%"));
```

`services()` lists the services, characteristics, capabilities, and descriptors
of a connected peripheral. `read`, `writeRequest`, and `writeCommand` access
characteristic values, and `read` and `write` with a descriptor UUID access
descriptors. `notify` and `indicate` call your callback with each value, and
`unsubscribe` stops them. These calls and `connect()` also have `Async`
counterparts that return a `CompletableFuture`. Failures throw
`SimpleJavaBleException` or one of its subclasses, such as `NotConnectedException`,
with SimpleBLE's error code and message.

## Host a GATT service

Create a local peripheral from an adapter, add services and characteristics,
then start advertising:

```java
LocalPeripheral host = adapter.createLocalPeripheral();
host.addAdvertisedService(batteryService);

LocalCharacteristic battery = host.addService(batteryService).addCharacteristic(
        batteryLevel, LocalCharacteristicCapability.READ, LocalCharacteristicCapability.NOTIFY);
battery.setValue(new byte[] {100});

host.setEventListener(new LocalPeripheral.EventListener() {
    @Override
    public void onClientConnected(BluetoothAddress address) {
        System.out.println(address + " connected");
    }
});
host.start();
```

`setValue()` updates the stored value and publishes it to subscribers. A local
characteristic's `EventListener` receives `onWrite`, `onSubscribed`, and
`onUnsubscribed`, and `setReadHandler` computes a value for each read.
Configure services before `start()`; call `stop()` before changing them.

## Use a Dongl

A [Dongl](https://docs.simpleble.org/dongl?utm_source=github&utm_medium=referral&utm_campaign=simplejavable_readme&utm_content=dongl) is a USB Bluetooth adapter made for
SimpleBLE. Its backend is on by default; use the adapters of the Dongl backend:

```java
Adapter adapter = Backend.getDongl().getAdapters().get(0);
```

`Config` holds the Dongl, BlueZ, and WinRT backend settings; set them before the
first call that reaches a backend. Pairing through a Dongl happens without the
operating system: set `Advanced.Dongl.setPasskeyRequestCallback`,
`setPasskeyDisplayCallback`, or `setNumericComparisonCallback` for the
peripheral before `connect()`.

## Platforms

| Platform | Architectures | Requirements |
| --- | --- | --- |
| macOS 15.0 or newer | x64, arm64 | Bluetooth permission for the app that runs the JVM |
| Windows 10 or newer | x64 | |
| Linux | x64, arm64 | BlueZ and the D-Bus library |

See [Bluetooth permissions](https://docs.simpleble.org/fundamentals/permissions?utm_source=github&utm_medium=referral&utm_campaign=simplejavable_readme&utm_content=platforms)
for the setup each platform needs.

## Versions

SimpleJavaBLE follows SimpleBLE's release versions. Stable releases use
`X.Y.Z`, and development builds of the main branch are published as
`X.Y.Z-devN`.

## License

SimpleJavaBLE follows the [SimpleBLE license](https://github.com/simpleble/simpleble/blob/main/LICENSE.md),
the Business Source License 1.1 (BUSL-1.1). It is free for non-commercial use,
and commercial use needs a commercial license. We offer free commercial licenses
for small projects and early-stage companies: [email us](mailto:contact@simpleble.org)
or [leave us a message](https://www.simpleble.org/contact?utm_source=github&utm_medium=referral&utm_campaign=simplejavable_readme&utm_content=license) to discuss your use
case. Commercial licenses carry no royalties or revenue sharing; see the
[licensing page](https://docs.simpleble.org/licensing?utm_source=github&utm_medium=referral&utm_campaign=simplejavable_readme&utm_content=license) for terms.

---

**SimpleBLE** is a project powered by [The California Open Source Company](https://californiaopensource.com/?utm_source=github&utm_medium=referral&utm_campaign=simplejavable_readme&utm_content=footer).
