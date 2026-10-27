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

String describe(Adapter adapter) => '${adapter.identifier} [${adapter.address}]';

String hex(List<int> data) => data.map((byte) => byte.toRadixString(16).padLeft(2, '0')).join();

Future<void> main() async {
  final adapters = await Adapter.getAdapters();
  if (adapters.isEmpty) throw StateError('No adapters found');

  final adapter = choose('an adapter', adapters, describe);
  print('Selected adapter: ${describe(adapter)}');

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

  print('The following peripherals were found:');
  for (final peripheral in adapter.scanGetResults()) {
    final connectable = peripheral.isConnectable ? 'Connectable' : 'Non-Connectable';
    print('${peripheral.identifier} [${peripheral.address}] - $connectable');
    print('    Address Type: ${peripheral.addressType.name}');
    print('    Tx Power: ${peripheral.txPower ?? 'unknown'} dBm');
    for (final MapEntry(key: companyId, value: data) in peripheral.manufacturerData().entries) {
      print('    Manufacturer ID: $companyId');
      print('    Manufacturer data: ${hex(data)}');
    }
    for (final service in peripheral.services()) {
      print('    Service UUID: ${service.uuid}');
      print('    Service data: ${hex(service.data)}');
    }
  }
}
