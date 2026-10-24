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

String? ask(String question) {
  stdout.write(question);
  return stdin.readLineSync();
}

Future<void> main() async {
  final dongl = await Backend.getDongl();
  final adapters = await dongl.getAdapters();
  if (adapters.isEmpty) throw StateError('No Dongl found');

  final adapter = choose('a Dongl', adapters, (adapter) => '${adapter.identifier} [${adapter.address}]');
  print('Selected Dongl: ${adapter.identifier} [${adapter.address}]');

  final found = adapter.onScanFound.listen(
    (peripheral) => print('Found ${peripheral.identifier} [${peripheral.address}]'),
  );

  // Scan for 5 seconds
  await adapter.scanFor(const Duration(seconds: 5));
  await found.cancel();

  final peripherals = adapter.scanGetResults();
  if (peripherals.isEmpty) throw StateError('No peripherals found');
  final peripheral = choose(
    'a peripheral',
    peripherals,
    (peripheral) => '${peripheral.identifier} [${peripheral.address}]',
  );

  // Pairing can start as soon as the link is up, so set the handlers before
  // connecting. The Dongl pairs without the operating system.
  await peripheral.setPasskeyRequestHandler(() {
    final passkey = ask('Enter the passkey shown on the device (empty to reject): ');
    return passkey == null || passkey.isEmpty ? null : passkey;
  });
  await peripheral.setPasskeyDisplayHandler((passkey) => print('Enter $passkey on the device.'));
  await peripheral.setNumericComparisonHandler((passkey) => ask('Does the device show $passkey? [y/N] ') == 'y');

  print('Connecting to: ${peripheral.identifier} [${peripheral.address}]');
  await peripheral.connect();

  print('Successfully connected, listing services...');
  for (final service in peripheral.services()) {
    print('Service: ${service.uuid}');
    for (final characteristic in service.characteristics) {
      print('    Characteristic: ${characteristic.uuid}');
    }
  }
  print('Paired: ${await peripheral.isPaired()}');

  await peripheral.disconnect();
  await adapter.close();
}
