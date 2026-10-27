import 'package:simpledartble/simpledartble.dart';

Future<void> main() async {
  print('Bluetooth enabled: ${await Adapter.isBluetoothEnabled()}');

  final adapters = await Adapter.getAdapters();
  if (adapters.isEmpty) print('No adapters found');
  for (final adapter in adapters) {
    print('Adapter: ${adapter.identifier} [${adapter.address}]');
  }
}
