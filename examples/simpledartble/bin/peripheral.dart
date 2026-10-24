import 'dart:convert';
import 'dart:io';

import 'package:simpledartble/simpledartble.dart';

const serviceUuid = '12345678-1234-5678-1234-56789abcdef0';
const characteristicUuid = '12345678-1234-5678-1234-56789abcdef1';

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
  final host = adapter.createLocalPeripheral();

  // Add a service UUID to the advertisement.
  host.addAdvertisedService(serviceUuid);

  final service = host.addService(serviceUuid);
  final characteristic = service.addCharacteristic(characteristicUuid, CharacteristicCapability.values.toSet());
  await characteristic.setValue(utf8.encode('ready'));

  characteristic.onWrite.listen((value) async {
    print('Write: ${hex(value)}');
    // Echo the written value to subscribed clients.
    await characteristic.setValue(value);
  });
  characteristic.onSubscribed.listen((_) => print('Client subscribed.'));
  characteristic.onUnsubscribed.listen((_) => print('Client unsubscribed.'));

  host.onClientConnected.listen((address) => print('Client connected: $address'));
  host.onClientDisconnected.listen((address) => print('Client disconnected: $address'));

  await host.start();
  print('Advertising. Press Ctrl+C to stop.');

  await ProcessSignal.sigint.watch().first;
  await host.stop();
}
