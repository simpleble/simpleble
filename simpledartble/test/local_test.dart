// Runs against SimpleBLE's PLAIN backend. Its adapter can host local GATT
// services, and lists a started host in its own scans as
// `Plain Adapter Peripheral`, so one program exercises both sides.
import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';

import 'package:simpledartble/simpledartble.dart';
import 'package:test/test.dart';

const serviceUuid = '00001234-0000-1000-8000-00805f9b34fb';
const valueUuid = '00005678-0000-1000-8000-00805f9b34fb';

Future<T> within<T>(Future<T> future) => future.timeout(const Duration(seconds: 5));

void main() {
  late Adapter adapter;
  late LocalPeripheral host;
  late LocalService service;
  late LocalCharacteristic value;
  late Peripheral peer;

  setUpAll(() async {
    adapter = (await Adapter.getAdapters()).single;
    host = adapter.createLocalPeripheral();
    service = host.addService(serviceUuid);
    value = service.addCharacteristic(valueUuid, {
      CharacteristicCapability.read,
      CharacteristicCapability.writeRequest,
      CharacteristicCapability.notify,
    });
    await value.setValue(Uint8List.fromList([0x00, 0xff]));
    await host.start();

    final found = adapter.onScanFound.firstWhere((peripheral) => peripheral.identifier == 'Plain Adapter Peripheral');
    await adapter.scanStart();
    peer = await within(found);
    await adapter.scanStop();
  });

  tearDownAll(() async {
    if (peer.isConnected) await peer.disconnect();
    await host.stop();
  });

  test('a local peripheral keeps one object per native object and converts its characteristics', () async {
    expect(host.services().single, same(service));
    expect(service.characteristics().single, same(value));
    expect(service.uuid, serviceUuid);
    expect(value.uuid, valueUuid);
    expect(value.capabilities, {
      CharacteristicCapability.read,
      CharacteristicCapability.writeRequest,
      CharacteristicCapability.notify,
    });
    expect(value.value, [0x00, 0xff]);

    expect(host.isStarted, isTrue);
    expect(await host.isAdvertising(), isTrue);
  });

  test('central operations reach the local events', () async {
    final connected = host.onClientConnected.first;
    await peer.connect();
    final address = await within(connected);
    expect(address, adapter.address);

    expect(await peer.read(serviceUuid, valueUuid), [0x00, 0xff]);

    final written = value.onWrite.first;
    await peer.writeRequest(serviceUuid, valueUuid, Uint8List.fromList([1, 2, 3]));
    expect(await within(written), [1, 2, 3]);

    final subscribed = value.onSubscribed.first;
    final values = StreamIterator(await peer.notify(serviceUuid, valueUuid));
    await within(subscribed);
    await value.setValue(Uint8List.fromList([7]));
    expect(await within(values.moveNext()), isTrue);
    expect(values.current, [7]);

    final unsubscribed = value.onUnsubscribed.first;
    await peer.unsubscribe(serviceUuid, valueUuid);
    await within(unsubscribed);

    final disconnected = host.onClientDisconnected.first;
    await peer.disconnect();
    expect(await within(disconnected), address);
  });

  test('read handlers answer reads with a value or a future, and a failing one with an empty value', () async {
    await peer.connect();

    await value.setReadHandler(() => utf8.encode('now'));
    expect(utf8.decode(await peer.read(serviceUuid, valueUuid)), 'now');

    await value.setReadHandler(() async {
      await Future<void>.delayed(const Duration(milliseconds: 50));
      return Uint8List.fromList([9]);
    });
    expect(await peer.read(serviceUuid, valueUuid), [9]);

    // The error is an uncaught error of the zone the handler was set in.
    final errors = <Object>[];
    await runZonedGuarded(
      () => value.setReadHandler(() => throw StateError('from handler')),
      (error, _) => errors.add(error),
    );
    expect(await peer.read(serviceUuid, valueUuid), isEmpty);
    expect(errors, [isA<StateError>()]);

    await value.setReadHandler(null);
    expect(await peer.read(serviceUuid, valueUuid), value.value);
    await peer.disconnect();
  });
}
