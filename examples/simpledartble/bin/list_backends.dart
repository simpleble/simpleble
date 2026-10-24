import 'package:simpledartble/simpledartble.dart';

Future<void> main() async {
  for (final backend in await Backend.getBackends()) {
    print('Backend: ${backend.identifier}');
    for (final adapter in await backend.getAdapters()) {
      print('    Adapter: ${adapter.identifier} [${adapter.address}]');
    }
  }
}
