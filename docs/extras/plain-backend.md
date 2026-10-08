# PLAIN backend for binding development

This is contributor documentation for developing language bindings and testing
SimpleBLE without Bluetooth hardware. It is kept outside `content/docs` and is
not included in the published documentation site, its search index, or its
LLM reference text.

## Build

From the repository root:

```bash
cmake -S simpleble -B build_plain -DSIMPLEBLE_PLAIN=ON
cmake --build build_plain -j7
```

The flag replaces the native Bluetooth backend with the default simulated
environment. It is intended for repository development and binding tests.
Native advanced APIs require their corresponding platform backend.

Backend enumeration always exposes exactly one backend, `Plain`. Enabling
Dongl in configuration cannot add a hardware backend to this build.

## Default environment

`SIMPLEBLE_PLAIN=ON` runs the simulator with a default adapter and device. It
requires no Bluetooth hardware, permissions, native Bluetooth stack, or USB
device. The backend is named `Plain`, its adapter is `Plain Adapter`
(`AA:BB:CC:DD:EE:FF`), and its remote device is `Plain Peripheral`
(`11:22:33:44:55:66`).

The adapter and device keep their identity and state across enumeration calls.
Scanning discovers actual periodic advertisements; results are initially empty.
Wait for the scan-found callback, or scan for long enough to receive an
advertisement, such as `adapter.scan_for(500)`. Scan, connection, power, and value
callbacks run asynchronously on the adapter's callback thread.

The default device exposes two services. The short UUIDs below use the Bluetooth
base UUID, for example `fff1` means `0000fff1-0000-1000-8000-00805f9b34fb`.

| Service | Characteristic | Behavior |
| --- | --- | --- |
| Battery (`180f`) | Battery Level (`2a19`) | Read and notify; value is one byte, `100`. |
| Test (`fff0`) | Binary value (`fff1`) | Read, write request, write command, notify, and indicate. Initial bytes: `00 7f 80 ff`. Writes replace the stored value, including empty values. |
| Test (`fff0`) | Error (`fff2`) | Reads fail with ATT `0x08` (insufficient authorization); write requests fail with ATT `0x13` (value not allowed). |

The binary characteristic also has a writable User Description descriptor
(`2901`), initially `Plain value`. Subscribable characteristics have a Client
Characteristic Configuration descriptor (`2902`); its value reflects the current
connection's subscription. Notifications and indications publish the stored
characteristic value once per second while subscribed. Disconnecting removes
subscriptions. The default ATT MTU is 247, so `Peripheral::mtu()` returns a
244-byte payload limit.

Advertisements include both service UUIDs, binary service data `00 7f 80 ff` for
`fff0`, manufacturer data `test` for company `0x004c`, TX power 5 dBm, and RSSI
-60 dBm. Connections pair automatically: `is_paired()` becomes true and the device
appears in `get_paired_peripherals()`. Paired state survives disconnect until
`unpair()`. Authentication and encryption are not simulated.

Power changes update the adapter state and emit callbacks. Power-off stops
scanning; connections and local hosts continue running. Invalid services,
characteristics, descriptors, capabilities, and disconnected GATT operations
produce the normal SimpleBLE exceptions. Native handles are null and
platform-specific advanced APIs require their native backend.

## Local GATT loopback

`create_local_peripheral()` is supported by simulated adapters. A started local
peripheral appears in scans as `Plain Adapter Peripheral`, with its own unique
address. Central-side calls reach the local read, write, subscription, and client
connection callbacks, so a binding can test both sides of GATT in one process.

```cpp
auto adapter = SimpleBLE::Adapter::get_adapters().front();
auto local = adapter.create_local_peripheral();
const std::string service_uuid = "00001234-0000-1000-8000-00805f9b34fb";
const std::string value_uuid = "00005678-0000-1000-8000-00805f9b34fb";
auto service = local.add_service(service_uuid);
auto value = service.add_characteristic(value_uuid, {
    SimpleBLE::Local::CharacteristicCapability::READ,
    SimpleBLE::Local::CharacteristicCapability::WRITE_REQUEST,
    SimpleBLE::Local::CharacteristicCapability::NOTIFY,
});
value.set_value(SimpleBLE::ByteArray{0x00, 0xff});
local.start();

adapter.scan_for(500);
for (auto peer : adapter.scan_get_results()) {
    if (peer.identifier() != "Plain Adapter Peripheral") continue;
    peer.connect();
    auto bytes = peer.read(service_uuid, value_uuid); // 00 ff
    peer.disconnect();
}
local.stop();
```

Local GATT configuration is frozen while started. Stopping or releasing the local
host disconnects its clients; stopping and starting the same host preserves its
values. Updating a notifiable or indicatable local characteristic publishes its
new value to subscribed clients on a best-effort basis.
Local read and write handlers run on the simulated device's thread and must
return promptly; calling a blocking central GATT operation from them prevents
the device from handling that operation.

## Test isolation and custom simulations

Tests should disconnect clients, clear registered callbacks, stop local hosts,
and restore values they changed. State persists for the process lifetime. C++
tests needing custom devices or radio conditions can activate a
[simulation environment](../content/docs/simpleble/simulation/build-a-simulation.mdx).
Its adapters replace the default environment's adapters until deactivated;
the backend remains `Plain` throughout, including for existing backend handles.

## Android development

Consumer SimpleDroidBLE builds use the Android Bluetooth backend. The
repository's `plain` build uses the same Kotlin and JNI layers with the
simulator and can run on an emulator without Bluetooth hardware.

Run the binding integration tests on a connected emulator:

```bash
./simpledroidble/gradlew -p simpledroidble :simpledroidble:connectedPlainAndroidTest
```

To build the SimpleBLE Explorer example for an emulator:

```bash
cd examples/simpleble-android
./gradlew :app:assemblePlain
```

The example supports scanning, connections, service discovery, reads, writes,
notifications, and indications through the simulated backend. Verify native
scan, connection, reconnection, and GATT behavior on a physical phone using
the regular build.
