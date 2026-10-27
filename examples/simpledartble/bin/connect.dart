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
  for (final service in peripheral.services()) {
    print('Service: ${service.uuid}');
    for (final characteristic in service.characteristics) {
      print('    Characteristic: ${characteristic.uuid}');
      print('    Capabilities: ${characteristic.capabilities.map((capability) => capability.name).join(' ')}');
    }
  }

  await peripheral.disconnect();
}
