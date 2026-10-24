// Behavior of whole Dart programs, each case in a program of its own: what
// SimpleDartBLE keeps alive through garbage collection, which the program
// forces through the VM service, and how a program ends. The programs share a
// package of their own, as Windows can't replace the library this test process
// has loaded.
@TestOn('vm')
library;

import 'dart:io';

import 'package:test/test.dart';

final _package = Directory('.dart_tool/simpledartble_test');

/// Runs [body] as the main function of a program and returns its output.
Future<ProcessResult> _program(String body) async {
  if (!File('${_package.path}/pubspec.yaml').existsSync()) {
    Directory('${_package.path}/bin').createSync(recursive: true);
    File('${_package.path}/pubspec.yaml').writeAsStringSync('''
name: simpledartble_test
publish_to: none
environment:
  sdk: ^3.10.0
dependencies:
  simpledartble:
    path: ../..
  vm_service: any
hooks:
  user_defines:
    simpledartble:
      backend: plain
''');
    final get = await Process.run(Platform.resolvedExecutable, [
      'pub',
      'get',
      '--offline',
    ], workingDirectory: _package.path);
    if (get.exitCode != 0) fail('dart pub get failed: ${get.stderr}');
  }

  File('${_package.path}/bin/program.dart').writeAsStringSync('''
import 'dart:developer' as developer;
import 'dart:isolate' as isolates;

import 'package:simpledartble/simpledartble.dart';
import 'package:vm_service/vm_service_io.dart';

Future<void> collectGarbage() async {
  final service = await vmServiceConnectUri('\${(await developer.Service.getInfo()).serverWebSocketUri}');
  await service.getAllocationProfile(developer.Service.getIsolateId(isolates.Isolate.current)!, gc: true);
  await service.dispose();
}

Future<void> main() async {
$body
}
''');
  return Process.run(Platform.resolvedExecutable, [
    'run',
    '--enable-vm-service=0',
    '--disable-service-auth-codes',
    'bin/program.dart',
  ], workingDirectory: _package.path).timeout(const Duration(minutes: 10));
}

void main() {
  test('an adapter with listeners keeps receiving events after its last reference is dropped', () async {
    final result = await _program('''
  var powerOffs = 0;
  await () async {
    final adapter = (await Adapter.getAdapters()).single;
    adapter.onPowerOff.listen((_) => powerOffs++);
  }();
  await collectGarbage();

  final adapter = (await Adapter.getAdapters()).single;
  final poweredOff = adapter.onPowerOff.first.timeout(const Duration(seconds: 5));
  await adapter.powerOff();
  await poweredOff;
  await adapter.powerOn();
  print(powerOffs);
''');
    expect(result.exitCode, 0, reason: '${result.stderr}');
    // NOTE: The VM service announces itself on standard output first.
    expect('${result.stdout}'.trim().split('\n').last.trim(), '1');
    // NOTE: The program's package builds SimpleBLE the first time.
  }, timeout: const Timeout(Duration(minutes: 10)));

  test('a peripheral with subscriptions keeps receiving values after its last reference is dropped', () async {
    final result = await _program('''
  const battery = '0000180f-0000-1000-8000-00805f9b34fb';
  const level = '00002a19-0000-1000-8000-00805f9b34fb';
  final adapter = (await Adapter.getAdapters()).single;
  var values = 0;
  await () async {
    await adapter.scanFor(const Duration(milliseconds: 500));
    final peripheral = adapter.scanGetResults().single;
    await peripheral.connect();
    (await peripheral.notify(battery, level)).listen((_) => values++);
  }();
  await collectGarbage();

  values = 0;
  for (var i = 0; i < 30 && values == 0; i++) {
    await Future<void>.delayed(const Duration(milliseconds: 100));
  }
  await (await adapter.getConnectedPeripherals()).single.disconnect();
  print(values > 0);
''');
    expect(result.exitCode, 0, reason: '${result.stderr}');
    expect('${result.stdout}'.trim().split('\n').last.trim(), 'true');
  }, timeout: const Timeout(Duration(minutes: 10)));

  test('a program that ends with an error while callbacks keep coming exits without crashing', () async {
    // SimpleBLE disconnects the peripheral while the Dart VM shuts down.
    final result = await _program('''
  final adapter = (await Adapter.getAdapters()).single;
  await adapter.scanFor(const Duration(milliseconds: 500));
  final peripheral = adapter.scanGetResults().single;
  await peripheral.connect();
  throw StateError('The program ends here');
''');
    expect(result.exitCode, 255, reason: '${result.stderr}');
    expect('${result.stderr}', contains('The program ends here'));
    expect('${result.stderr}', isNot(contains('DumpStackTrace')));
  }, timeout: const Timeout(Duration(minutes: 10)));
}
