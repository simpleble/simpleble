// Scans for five seconds with the first adapter and prints the peripherals it
// finds. The repository's examples/simpledartble folder has an example for each
// operation: https://github.com/simpleble/simpleble/tree/main/examples/simpledartble
import 'package:simpledartble/simpledartble.dart';

Future<void> main() async {
  final adapter = (await Adapter.getAdapters()).first;
  print('Using ${adapter.identifier} [${adapter.address}]');

  adapter.onScanFound.listen((peripheral) {
    print('Found ${peripheral.identifier} [${peripheral.address}] ${peripheral.rssi} dBm');
  });
  await adapter.scanFor(const Duration(seconds: 5));
}
