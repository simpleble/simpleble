// How SimpleDartBLE keeps an isolate, and so a Dart program, running. Each case
// runs in an isolate of its own against SimpleBLE's PLAIN backend, and checks
// whether that isolate exits by itself.
import 'dart:async';
import 'dart:isolate';

import 'package:simpledartble/simpledartble.dart';
import 'package:test/test.dart';

const _hostService = '00001234-0000-1000-8000-00805f9b34fb';

typedef _Body = Future<void> Function(SendPort report, Future<void> signal);

/// A case in an isolate of its own. It reports through a port, and can wait
/// for a signal from the test, which doesn't keep it running.
final class _Isolate {
  _Isolate._();

  final _messages = ReceivePort();
  final _exit = ReceivePort();
  final _errors = ReceivePort();
  final _reports = StreamController<Object?>();
  late final _received = StreamIterator(_reports.stream);
  final _signal = Completer<SendPort>();
  late final Future<Object?> _ended;

  static Future<_Isolate> spawn(_Body body) async {
    final isolate = _Isolate._();
    isolate._messages.listen((message) {
      if (message is SendPort) {
        isolate._signal.complete(message);
      } else {
        isolate._reports.add(message);
      }
    });
    isolate._ended = Future.any([isolate._exit.first, isolate._errors.first]);
    await Isolate.spawn(
      _entry,
      (isolate._messages.sendPort, body),
      onExit: isolate._exit.sendPort,
      onError: isolate._errors.sendPort,
    );
    return isolate;
  }

  /// The next report.
  Future<Object?> next() async {
    if (!await _received.moveNext().timeout(const Duration(seconds: 10))) fail('The isolate ended');
    return _received.current;
  }

  Future<void> signal() async => (await _signal.future).send(null);

  /// Whether the isolate ends by itself within [time]. Fails if it failed.
  Future<bool> endsWithin(Duration time) async {
    final outcome = await _ended.timeout(time, onTimeout: () => 'running');
    if (outcome == 'running') return false;
    if (outcome != null) fail('The isolate failed: $outcome');
    _messages.close();
    _exit.close();
    _errors.close();
    return true;
  }
}

void _entry((SendPort, _Body) setup) {
  final (port, body) = setup;
  final signal = Completer<void>();
  final signals = RawReceivePort((_) => signal.complete(), 'signal')..keepIsolateAlive = false;
  port.send(signals.sendPort);
  body(port, signal.future);
}

Future<Peripheral> _plainPeripheral(Adapter adapter) async {
  await adapter.scanFor(const Duration(milliseconds: 500));
  return adapter.scanGetResults().firstWhere((peripheral) => peripheral.identifier == 'Plain Peripheral');
}

void main() {
  const settle = Duration(milliseconds: 500);
  const end = Duration(seconds: 10);

  test('listeners alone do not keep the program running', () async {
    final isolate = await _Isolate.spawn((report, signal) async {
      final adapter = (await Adapter.getAdapters()).single;
      adapter.onScanFound.listen((_) {});
    });
    expect(await isolate.endsWithin(end), isTrue);
  });

  test('an active scan, an open connection and a started local peripheral keep the program running', () async {
    final cases = <String, _Body>{
      'scan': (report, signal) async {
        final adapter = (await Adapter.getAdapters()).single;
        await adapter.scanStart();
        report.send('active');
        await signal;
        await adapter.scanStop();
      },
      'connection': (report, signal) async {
        final peripheral = await _plainPeripheral((await Adapter.getAdapters()).single);
        await peripheral.connect();
        report.send('active');
        await signal;
        await peripheral.disconnect();
      },
      'local peripheral': (report, signal) async {
        final host = (await Adapter.getAdapters()).single.createLocalPeripheral();
        host.addService(_hostService);
        await host.start();
        report.send('active');
        await signal;
        await host.stop();
      },
    };
    for (final MapEntry(key: name, value: body) in cases.entries) {
      final isolate = await _Isolate.spawn(body);
      expect(await isolate.next(), 'active', reason: name);
      expect(await isolate.endsWithin(settle), isFalse, reason: name);
      await isolate.signal();
      expect(await isolate.endsWithin(end), isTrue, reason: name);
    }
  });

  test('close() ends the scan, connections and local peripherals, and releases what came from the adapter', () async {
    // The isolate ends by itself only if close() ended all three.
    final isolate = await _Isolate.spawn((report, signal) async {
      final adapter = (await Adapter.getAdapters()).single;
      final host = adapter.createLocalPeripheral()..addService(_hostService);
      await host.start();
      final peripheral = await _plainPeripheral(adapter);
      await peripheral.connect();
      await adapter.scanStart();
      final scanFound = adapter.onScanFound.listen(null).asFuture<void>();
      await adapter.close();
      await scanFound;

      for (final call in [() => adapter.identifier, () => peripheral.isConnected, () => host.isStarted]) {
        try {
          call();
          report.send('a closed object answered');
        } on NotInitializedException {
          report.send('closed');
        }
      }
      final again = (await Adapter.getAdapters()).single;
      report.send(identical(again, adapter));
      report.send((await again.getConnectedPeripherals()).length);
    });
    expect([for (var i = 0; i < 5; i++) await isolate.next()], ['closed', 'closed', 'closed', false, 0]);
    expect(await isolate.endsWithin(end), isTrue);
  });
}
