import 'dart:io';

import 'package:simpledartble/simpledartble.dart';

// Lists [items] and returns the one the user picks.
T choose<T>(String title, List<T> items, String Function(T item) describe) {
  print('Please select $title:');
  for (final (i, item) in items.indexed) {
    print('$i: ${describe(item)}');
  }
  stdout.write('Enter choice: ');
  final choice = int.tryParse(stdin.readLineSync() ?? '');
  if (choice == null || choice < 0 || choice >= items.length) throw StateError('Invalid choice');
  return items[choice];
}

String hex(List<int> data) => data.map((byte) => byte.toRadixString(16).padLeft(2, '0')).join();

Future<void> main() async {
  final adapters = await Adapter.getAdapters();
  if (adapters.isEmpty) throw StateError('No adapters found');

  final adapter = choose('an adapter', adapters, (adapter) => '${adapter.identifier} [${adapter.address}]');
  print('Selected adapter: ${adapter.identifier} [${adapter.address}]');

  final subscriptions = [
    adapter.onScanStart.listen((_) => print('Scan started.')),
    adapter.onScanStop.listen((_) => print('Scan complete.')),
    adapter.onScanFound.listen((peripheral) => print('Found ${peripheral.identifier} [${peripheral.address}]')),
  ];

  // Scan for 5 seconds
  await adapter.scanFor(const Duration(seconds: 5));
  for (final subscription in subscriptions) {
    await subscription.cancel();
  }

  final peripherals = adapter.scanGetResults();
  if (peripherals.isEmpty) throw StateError('No peripherals found');
  final peripheral = choose(
    'a peripheral',
    peripherals,
    (peripheral) => '${peripheral.identifier} [${peripheral.address}]',
  );

  print('Connecting to: ${peripheral.identifier} [${peripheral.address}]');
  await peripheral.connect();

  print('Successfully connected, listing services...');
  final pairs = [
    for (final service in peripheral.services())
      for (final characteristic in service.characteristics)
        (service: service.uuid, characteristic: characteristic.uuid),
  ];
  if (pairs.isEmpty) throw StateError('No characteristics found');
  final pair = choose('a service/characteristic pair', pairs, (pair) => '${pair.service} ${pair.characteristic}');

  final contents = await peripheral.read(pair.service, pair.characteristic);
  print('Contents: ${hex(contents)}');

  await peripheral.disconnect();
}
