import 'dart:async';
import 'dart:ffi';

import 'package:ffi/ffi.dart';

import 'internal/bindings.g.dart';
import 'internal/events.dart';

/// SimpleBLE's log levels, from [none] (off) to the most detailed, [verbose].
enum LogLevel { none, fatal, error, warn, info, debug, verbose }

/// One entry of SimpleBLE's log.
final class LogEntry {
  const LogEntry(this.level, this.module, this.file, this.line, this.function, this.message);

  final LogLevel level;

  /// The component that logged it, such as `SimpleBLE` or `SimpleBLE->SimpleBluez`.
  final String module;

  final String file;
  final int line;
  final String function;
  final String message;

  @override
  String toString() => '[${level.name}] $module: $message';
}

/// SimpleBLE's log, process-wide. By default, entries up to [LogLevel.info] go
/// to standard output.
abstract final class Logging {
  static _Receiver? _receiver;

  /// The most detailed level that is logged; [LogLevel.none] turns logging off.
  static LogLevel get level => LogLevel.values[simpleble_logging_get_level().value];
  static set level(LogLevel level) => simpleble_logging_set_level(simpleble_log_level_t.fromValue(level.index));

  /// Delivers each entry to [callback] on the isolate that uses SimpleDartBLE,
  /// in the order they were logged, instead of writing it out. Null discards
  /// entries. The callback runs in the zone this was called in.
  static void setCallback(void Function(LogEntry entry)? callback) {
    final previous = _receiver;
    if (callback == null) {
      simpleble_logging_set_callback(nullptr);
      _receiver = null;
    } else {
      final receiver = _Receiver(Zone.current.bindUnaryCallback(callback));
      receiver.token = Events.instance.register(receiver, 0);
      _receiver = receiver;
      simpledartble_log_to(receiver.token.address);
      simpleble_logging_set_callback(Native.addressOf<NativeFunction<_LogCallback>>(simpledartble_on_log).cast());
    }
    if (previous != null) Events.instance.unregister(previous.token);
  }

  /// Writes entries to standard output, the default.
  static void logToStdout() {
    simpleble_logging_log_default_stdout();
    _stop();
  }

  /// Appends entries to [path], or to a new timestamped file in the working directory.
  static void logToFile([String? path]) {
    if (path == null) {
      simpleble_logging_log_default_file();
    } else {
      using((arena) => simpleble_logging_log_default_file_path(path.toNativeUtf8(allocator: arena).cast()));
    }
    _stop();
  }

  static void _stop() {
    final receiver = _receiver;
    _receiver = null;
    if (receiver != null) Events.instance.unregister(receiver.token);
  }
}

typedef _LogCallback = Void Function(Int, Pointer<Char>, Pointer<Char>, Uint32, Pointer<Char>, Pointer<Char>);

final class _Receiver implements EventTarget {
  _Receiver(this.callback);

  final void Function(LogEntry entry) callback;
  Pointer<Void> token = nullptr;

  @override
  void onEvent(int kind, Object? value) {
    final [level, module, file, line, function, message] = value! as List<Object?>;
    String text(Object? value) => value as String? ?? '';
    callback(
      LogEntry(LogLevel.values[level! as int], text(module), text(file), line! as int, text(function), text(message)),
    );
  }
}
