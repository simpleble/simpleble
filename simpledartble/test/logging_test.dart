// Runs against SimpleBLE's PLAIN backend, whose adapter logs a warning when a
// scan starts with Bluetooth off.
import 'dart:async';

import 'package:simpledartble/simpledartble.dart';
import 'package:test/test.dart';

void main() {
  late Adapter adapter;

  setUpAll(() async {
    adapter = (await Adapter.getAdapters()).single;
    await adapter.powerOff();
  });

  tearDownAll(() async {
    Logging.logToStdout();
    Logging.level = LogLevel.info;
    await adapter.powerOn();
  });

  test('log entries reach the callback, null discards them, and the level filters them', () async {
    final entries = <LogEntry>[];
    Logging.setCallback(entries.add);
    await adapter.scanStart();
    await Future<void>.delayed(const Duration(milliseconds: 100));
    Logging.setCallback(null);
    await adapter.scanStart();

    final filtered = <LogEntry>[];
    Logging.setCallback(filtered.add);
    Logging.level = LogLevel.error;
    expect(Logging.level, LogLevel.error);
    await adapter.scanStart();
    await Future<void>.delayed(const Duration(milliseconds: 100));

    final [entry] = entries;
    expect(entry.level, LogLevel.warn);
    expect(entry.module, 'SimpleBLE');
    expect(entry.message, 'Bluetooth is not enabled.');
    expect(entry.file, isNotEmpty);
    expect(entry.line, isPositive);
    expect(filtered, isEmpty);
  });
}
