// Runs against SimpleBLE's PLAIN backend, using the GATT layout of its default
// device (docs/extras/plain-backend.md).
import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';

import 'package:simpledartble/simpledartble.dart';
import 'package:test/test.dart';

String uuid(String short) => '0000$short-0000-1000-8000-00805f9b34fb';

final battery = uuid('180f');
final batteryLevel = uuid('2a19');
final testService = uuid('fff0');
final value = uuid('fff1');
final error = uuid('fff2');
final description = uuid('2901');
final cccd = uuid('2902');

Future<T> within<T>(Future<T> future) => future.timeout(const Duration(seconds: 5));

Uint8List bytes(List<int> values) => Uint8List.fromList(values);

void main() {
  late Adapter adapter;
  late Peripheral peripheral;

  setUpAll(() async {
    adapter = (await Adapter.getAdapters()).single;
    final found = adapter.onScanFound.first;
    await adapter.scanStart();
    peripheral = await within(found);
    await adapter.scanStop();
  });

  tearDownAll(() async {
    if (peripheral.isConnected) await peripheral.disconnect();
  });

  test('services() before connecting converts the advertised services and their data', () {
    final services = peripheral.services();
    expect(services.firstWhere((service) => service.uuid == testService).data, [0x00, 0x7f, 0x80, 0xff]);
    expect(services.map((service) => service.uuid), contains(battery));
  });

  test('connect() and disconnect() reach the device and emit events', () async {
    final events = <String>[];
    final subscriptions = [
      peripheral.onConnected.listen((_) => events.add('connected')),
      peripheral.onDisconnected.listen((_) => events.add('disconnected')),
    ];

    final connected = peripheral.onConnected.first;
    await peripheral.connect();
    await within(connected);
    expect(peripheral.isConnected, isTrue);
    expect(await adapter.getConnectedPeripherals(), [same(peripheral)]);

    final disconnected = peripheral.onDisconnected.first;
    await peripheral.disconnect();
    await within(disconnected);
    expect(peripheral.isConnected, isFalse);
    for (final subscription in subscriptions) {
      await subscription.cancel();
    }

    expect(events, ['connected', 'disconnected']);
  });

  test('services() converts the discovered GATT tree', () async {
    await peripheral.connect();
    final services = peripheral.services();

    final binary = services
        .firstWhere((service) => service.uuid == testService)
        .characteristics
        .firstWhere((characteristic) => characteristic.uuid == value);
    expect(binary.capabilities, CharacteristicCapability.values.toSet());
    expect(binary.descriptors.map((descriptor) => descriptor.uuid), unorderedEquals([description, cccd]));

    final level = services
        .firstWhere((service) => service.uuid == battery)
        .characteristics
        .firstWhere((characteristic) => characteristic.uuid == batteryLevel);
    expect(level.capabilities, {CharacteristicCapability.read, CharacteristicCapability.notify});
  });

  test('values and descriptors round-trip, including empty ones', () async {
    final original = await peripheral.read(testService, value);
    final text = await peripheral.readDescriptor(testService, value, description);
    expect(original, [0x00, 0x7f, 0x80, 0xff]);
    expect(utf8.decode(text), 'Plain value');

    await peripheral.writeRequest(testService, value, bytes([1, 2, 3]));
    expect(await peripheral.read(testService, value), [1, 2, 3]);
    await peripheral.writeCommand(testService, value, bytes([]));
    expect(await peripheral.read(testService, value), isEmpty);
    await peripheral.writeDescriptor(testService, value, description, utf8.encode('changed'));
    expect(utf8.decode(await peripheral.readDescriptor(testService, value, description)), 'changed');

    await peripheral.writeRequest(testService, value, original);
    await peripheral.writeDescriptor(testService, value, description, text);
  });

  test('GATT failures become SimpleDartBleException with the SimpleBLE error code', () async {
    await expectLater(
      peripheral.read(testService, error),
      throwsA(isA<OperationFailedException>().having((e) => e.code, 'code', ErrorCode.operationFailed)),
    );
  });

  test('notify() and indicate() deliver values until unsubscribe() ends the stream', () async {
    // The CCCD tells notify (1) and indicate (2) apart.
    for (final (subscribe, flag) in [(peripheral.notify, 1), (peripheral.indicate, 2)]) {
      final values = StreamIterator(await subscribe(testService, value));
      expect(await peripheral.readDescriptor(testService, value, cccd), [flag, 0]);

      expect(await within(values.moveNext()), isTrue);
      expect(values.current, [0x00, 0x7f, 0x80, 0xff]);

      await peripheral.unsubscribe(testService, value);
      expect(await peripheral.readDescriptor(testService, value, cccd), [0, 0]);
      while (await within(values.moveNext())) {}
    }
  });

  test('cancelling a subscription unsubscribes, and a new subscription or a disconnection ends the stream', () async {
    await within((await peripheral.notify(testService, value)).first);
    expect(await peripheral.readDescriptor(testService, value, cccd), [0, 0]);

    final replaced = await peripheral.notify(testService, value);
    final current = await peripheral.notify(testService, value);
    expect(await within(replaced.toList()), isEmpty);

    final ended = current.toList();
    await peripheral.disconnect();
    await within(ended);
    await peripheral.connect();
  });

  test('subscriptions follow call order when calls are not awaited one by one', () async {
    // Each sequence ends with a subscription, which must keep receiving values.
    final sequences = <void Function()>[
      () => peripheral.unsubscribe(testService, value),
      () {
        peripheral.disconnect();
        peripheral.connect();
      },
    ];
    for (final sequence in sequences) {
      (await peripheral.notify(testService, value)).listen(null);
      sequence();

      final values = await peripheral.notify(testService, value);
      await within(values.first);
    }
  });

  test('unpair() reaches the device', () async {
    expect(await peripheral.isPaired(), isTrue);
    await peripheral.unpair();
    expect(await peripheral.isPaired(), isFalse);
  });
}
