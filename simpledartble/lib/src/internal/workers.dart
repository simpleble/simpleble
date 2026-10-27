import 'dart:async';
import 'dart:collection';
import 'dart:isolate';

/// Keeps a Dart program running while anything holds it: a pending call, an
/// active scan, an open connection or a started local peripheral. Listeners
/// and idle objects don't.
abstract final class KeepAlive {
  static int _holds = 0;
  static RawReceivePort? _port;

  static void hold() {
    if (_holds++ == 0) _port = RawReceivePort(null, 'SimpleDartBLE keep-alive');
  }

  static void release() {
    if (--_holds > 0) return;
    _port?.close();
    _port = null;
  }
}

/// Runs calls on one object one at a time, in call order, on the workers.
final class Strand {
  Future<void> _tail = Future<void>.value();

  /// Runs `function(argument)` on a worker after the calls before it.
  ///
  /// Both go to the worker, so [function] is a top-level or static function
  /// and [argument] holds plain values, such as handle addresses.
  Future<R> call<A, R>(R Function(A argument) function, A argument) => _run(() => function(argument));

  Future<R> _run<R>(R Function() function) {
    final result = _tail.then((_) => Workers.instance.run(function));
    _tail = result.then<void>((_) {}, onError: (_) {});
    return result;
  }
}

/// Backend enumeration and handle releases, which have to happen in order:
/// releasing a Dongl's adapter closes its serial port, which an enumeration
/// right after it would otherwise find in use.
final lifecycle = Strand();

/// Runs blocking SimpleCBLE calls on helper isolates, so the isolate using
/// SimpleDartBLE never waits on Bluetooth.
///
/// A connection attempt can block for seconds, so a busy worker never delays a
/// call of another object: when every worker is busy, another one starts. Two
/// idle workers stay around for the next calls.
final class Workers {
  Workers._();

  static final instance = Workers._();

  static const _keptIdle = 2;

  // Each worker's port, by worker index, or null while it starts or after it stopped.
  final _ports = <SendPort?>[];
  final _idle = <int>[];
  final _waiting = Queue<_Call<Object?>>();
  final _running = <int, _Call<Object?>>{};
  int _starting = 0;
  int _nextId = 0;
  late final _replies = RawReceivePort(_receive, 'SimpleDartBLE workers')..keepIsolateAlive = false;

  Future<R> run<R>(R Function() function) {
    final call = _Call<R>(_nextId++, function);
    KeepAlive.hold();
    _waiting.add(call);
    _dispatch();
    return call.completer.future;
  }

  void _dispatch() {
    while (_waiting.isNotEmpty && _idle.isNotEmpty) {
      final call = _waiting.removeFirst();
      call.worker = _idle.removeLast();
      _running[call.id] = call;
      _ports[call.worker]!.send((call.id, call.function));
    }
    while (_starting < _waiting.length) {
      _starting++;
      _ports.add(null);
      Isolate.spawn(_worker, (_replies.sendPort, _ports.length - 1), debugName: 'SimpleDartBLE worker');
    }
  }

  void _receive(Object? message) {
    switch (message) {
      case (int worker, SendPort port):
        _starting--;
        _ports[worker] = port;
        _idle.add(worker);
      case (int id, bool succeeded, Object? value, String? stackTrace):
        final call = _running.remove(id)!;
        _idle.add(call.worker);
        if (succeeded) {
          call.completer.complete(value);
        } else {
          call.completer.completeError(value!, StackTrace.fromString(stackTrace ?? ''));
        }
        KeepAlive.release();
    }
    _dispatch();
    while (_waiting.isEmpty && _idle.length > _keptIdle) {
      final worker = _idle.removeLast();
      _ports[worker]!.send(null);
      _ports[worker] = null;
    }
  }
}

final class _Call<R> {
  _Call(this.id, this.function);

  final int id;
  final R Function() function;
  final completer = Completer<R>();
  int worker = -1;
}

void _worker((SendPort, int) setup) {
  final (replies, worker) = setup;
  final port = RawReceivePort(null, 'SimpleDartBLE worker');
  port.handler = (Object? message) {
    if (message == null) {
      port.close();
      return;
    }
    final (int id, Object? Function() function) = message as (int, Object? Function());
    try {
      replies.send((id, true, function(), null));
    } catch (error, stackTrace) {
      try {
        replies.send((id, false, error, stackTrace.toString()));
      } catch (_) {
        replies.send((id, false, StateError('$error'), stackTrace.toString()));
      }
    }
  };
  replies.send((worker, port.sendPort));
}
