<p align="center">
  <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo-dark.png#gh-light-mode-only" alt="SimpleBLE" width="360">
  <img src="https://raw.githubusercontent.com/simpleble/simpleble/main/docs/public/simpleBLE-logo.png#gh-dark-mode-only" alt="SimpleBLE" width="360">
</p>

# SimpleJavaBLE

SimpleJavaBLE brings [SimpleBLE](https://github.com/simpleble/simpleble) to Java
and the JVM on Windows, macOS, and Linux. Discover and connect Bluetooth
peripherals, read and write GATT values, subscribe to notifications, and host
local GATT services through one API.

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

An adapter's `Adapter.EventListener` receives `onScanStart`, `onScanStop`,
`onScanFound`, and `onScanUpdated`; a peripheral's `Peripheral.EventListener`
receives `onConnected` and `onDisconnected`. Implement only the methods you
need. Listeners run on SimpleBLE's threads; hand longer work to your own
executor.

Use `scanStart()` and `scanStop()` when your application controls scan timing,
and `scanGetResults()` for the peripherals seen by the last scan. An adapter
always gives you the same `Peripheral` object for the same device, whether it
came from an event or from a list.

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
counterparts that return a `CompletableFuture`. Failures throw a
`RuntimeException` with SimpleBLE's message.

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

A [Dongl](https://docs.simpleble.org/dongl) is a USB Bluetooth adapter made for
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
| macOS 13.0 or newer | x64, arm64 | Bluetooth permission for the app that runs the JVM |
| Windows 10 or newer | x64 | |
| Linux | x64, arm64 | BlueZ and the D-Bus library |

See the [usage guide](https://docs.simpleble.org/simplejavable/usage) for
Bluetooth permissions and platform details.

## Versions

SimpleJavaBLE follows SimpleBLE's release versions. Stable releases use
`X.Y.Z`, and development builds of the main branch are published as
`X.Y.Z-devN`.

## Documentation and support

- Read the [SimpleJavaBLE usage guide](https://docs.simpleble.org/simplejavable/usage).
- Browse the [examples](https://github.com/simpleble/simpleble/tree/main/examples/simplejavable).
- Join the [Discord community](https://discord.gg/N9HqNEcvP3).
- Contribute in [simpleble/simpleble](https://github.com/simpleble/simpleble/tree/main/simplejavable); see the [publishing guide](https://github.com/simpleble/simpleble/blob/main/docs/extras/simplejavable-publishing.md).
- Visit [simpleble.org](https://simpleble.org/) for commercial licensing and professional services.

## License

SimpleJavaBLE follows the [SimpleBLE license](https://github.com/simpleble/simpleble/blob/main/LICENSE.md),
the Business Source License 1.1 (BUSL-1.1). It is free for non-commercial use,
and commercial use needs a commercial license. We offer free commercial licenses
for small projects and early-stage companies: [email us](mailto:contact@simpleble.org)
or [leave us a message](https://www.simpleble.org/contact) to discuss your use
case. Commercial licenses carry no royalties or revenue sharing; see the
[licensing page](https://docs.simpleble.org/licensing) for terms.

---

**SimpleBLE** is a project powered by [The California Open Source Company](https://californiaopensource.com/).
