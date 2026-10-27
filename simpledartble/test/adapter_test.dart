// Runs against SimpleBLE's PLAIN backend, which this package's pubspec selects
// for its own tests. PLAIN simulates one adapter and one peripheral that
// advertises periodically. Its state lasts for the whole process, so tests in
// this file run in order.
import 'dart:async';
import 'dart:typed_data';

import 'package:simpledartble/simpledartble.dart';
import 'package:test/test.dart';

Future<T> within<T>(Future<T> future) => future.timeout(const Duration(seconds: 5));

void main() {
  late Adapter adapter;

  setUpAll(() async => adapter = (await Adapter.getAdapters()).single);

  test('backends and adapters keep one object per native object', () async {
    final backends = await Backend.getBackends();
    expect(backends.map((backend) => backend.identifier), ['Plain']);
    expect((await Backend.getBackends()).single, same(backends.single));
    expect(await Backend.getNative(), same(backends.single));

    expect(adapter.identifier, 'Plain Adapter');
    expect(adapter.address, 'AA:BB:CC:DD:EE:FF');
    expect((await Adapter.getAdapters()).single, same(adapter));
    expect((await backends.single.getAdapters()).single, same(adapter));
    expect(await Adapter.isBluetoothEnabled(), isTrue);
    expect(await backends.single.isBluetoothEnabled(), isTrue);
  });

  test('scan events deliver the same Peripheral object as the scan results', () async {
    final events = <String>[];
    final subscriptions = [
      adapter.onScanStart.listen((_) => events.add('scanStart')),
      adapter.onScanStop.listen((_) => events.add('scanStop')),
    ];
    final found = adapter.onScanFound.first;
    final stopped = adapter.onScanStop.first;

    await adapter.scanStart();
    final peripheral = await within(found);
    expect(await adapter.scanIsActive(), isTrue);
    await adapter.scanStop();
    await within(stopped);
    for (final subscription in subscriptions) {
      await subscription.cancel();
    }

    expect(events, ['scanStart', 'scanStop']);
    expect(adapter.scanGetResults().single, same(peripheral));
    expect(await adapter.scanIsActive(), isFalse);

    expect(peripheral.identifier, 'Plain Peripheral');
    expect(peripheral.address, '11:22:33:44:55:66');
    expect(peripheral.addressType, AddressType.public);
    expect(peripheral.rssi, -60);
    expect(peripheral.txPower, 5);
    expect(peripheral.isConnectable, isTrue);
    expect(peripheral.manufacturerData(), {0x004c: Uint8List.fromList('test'.codeUnits)});
  });

  test('operations on one adapter run in call order', () async {
    final events = <String>[];
    final powerOns = StreamController<void>();
    final subscriptions = [
      adapter.onPowerOff.listen((_) => events.add('powerOff')),
      adapter.onPowerOn.listen((_) {
        events.add('powerOn');
        powerOns.add(null);
      }),
    ];

    // Not awaited one by one: the order of the calls alone decides the outcome.
    await Future.wait([adapter.powerOff(), adapter.powerOn(), adapter.powerOff(), adapter.powerOn()]);
    await within(powerOns.stream.take(2).drain<void>());
    for (final subscription in subscriptions) {
      await subscription.cancel();
    }

    expect(await adapter.isPowered(), isTrue);
    expect(events, ['powerOff', 'powerOn', 'powerOff', 'powerOn']);
  });
}
