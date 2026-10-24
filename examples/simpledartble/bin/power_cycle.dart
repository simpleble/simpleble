import 'package:simpledartble/simpledartble.dart';

Future<void> main() async {
  final adapters = await Adapter.getAdapters();
  if (adapters.isEmpty) print('No adapters found');

  for (final adapter in adapters) {
    print('Adapter: ${adapter.identifier} [${adapter.address}]');

    print('Powering off adapter');
    await adapter.powerOff();
    print('Adapter powered: ${await adapter.isPowered()}');

    await Future<void>.delayed(const Duration(seconds: 1));

    print('Powering on adapter');
    await adapter.powerOn();
    print('Adapter powered: ${await adapter.isPowered()}');
  }
}
