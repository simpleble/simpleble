import 'dart:async';
import 'dart:ffi';
import 'dart:isolate';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

import 'bindings.g.dart';
import 'registry.dart';

/// An object that receives SimpleBLE's callbacks.
abstract interface class EventTarget {
  /// Handles the callback registered as [kind]. [value] is what the callback
  /// passed: null, an owned peripheral handle's address, bytes, text, a list,
  /// such as a log entry or a peer's address and passkey, or a [Question].
  void onEvent(int kind, Object? value);
}

/// A native callback that waits for an answer, such as a read of a local
/// characteristic. It gets its fallback unless it's answered once.
final class Question {
  Question._(this._id, this.arguments);

  final int _id;
  bool _answered = false;

  /// The strings the callback passed, such as a peer's address or a passkey.
  final List<String> arguments;

  void answerBytes(Uint8List value) => _answer(() {
    using((arena) {
      final data = arena<Uint8>(value.isEmpty ? 1 : value.length);
      data.asTypedList(value.length).setAll(0, value);
      simpledartble_answer_bytes(_id, data, value.length);
    });
  });

  /// Answers with [value], or with the fallback for null.
  void answerText(String? value) => _answer(() {
    if (value == null) {
      simpledartble_answer_none(_id);
      return;
    }
    using((arena) => simpledartble_answer_text(_id, value.toNativeUtf8(allocator: arena).cast()));
  });

  /// Answers with a passkey of six digits, or with the fallback for null.
  void answerPasskey(String? passkey) {
    if (passkey != null && !RegExp(r'^[0-9]{6}$').hasMatch(passkey)) {
      throw ArgumentError.value(passkey, 'passkey', 'A passkey has six digits');
    }
    answerText(passkey);
  }

  void answerFlag(bool value) => _answer(() => simpledartble_answer_flag(_id, value));

  void fallback() => _answer(() => simpledartble_answer_none(_id));

  /// Answers with what [handler] returns, which may be a future. Without a
  /// handler, or when it fails, the question gets its fallback, and the error
  /// is unhandled like one in a stream listener.
  void answerWith<T>(FutureOr<T> Function()? handler, void Function(T value) answer) {
    if (handler == null) {
      fallback();
      return;
    }
    Future.sync(handler).then(
      (value) {
        try {
          answer(value);
        } catch (_) {
          fallback();
          rethrow;
        }
      },
      onError: (Object error, StackTrace stackTrace) {
        fallback();
        Zone.current.handleUncaughtError(error, stackTrace);
      },
    );
  }

  void _answer(void Function() call) {
    if (_answered) return;
    _answered = true;
    call();
  }
}

/// Delivers SimpleBLE's callbacks to the objects that registered them, on this
/// isolate and in the order they happened.
///
/// One isolate at a time receives the events, as callbacks belong to the
/// native objects that every isolate shares. After a Flutter hot restart, the
/// new isolate takes over.
final class Events implements Finalizable {
  Events._(this._port);

  static Events? _instance;

  static Events get instance => _instance ??= _claim();

  // NOTE: Native callbacks keep coming while the Dart VM shuts down, and
  // posting to it then crashes. The isolate group's shutdown runs this
  // finalizer, which ends the claim first.
  static final _release = NativeFinalizer(Native.addressOf(simpledartble_release));

  static Events _claim() {
    final port = RawReceivePort(null, 'SimpleDartBLE events')..keepIsolateAlive = false;
    simpledartble_initialize(NativeApi.postCObject.cast());
    final claim = simpledartble_claim(port.sendPort.nativePort);
    if (claim == 0) {
      port.close();
      throw StateError('Another isolate of this process is using SimpleDartBLE');
    }
    final events = Events._(port);
    port.handler = events._receive;
    _release.attach(events, Pointer.fromAddress(claim));
    return events;
  }

  // NOTE: Held for as long as the isolate runs, as it receives every event.
  // ignore: unused_field
  final RawReceivePort _port;
  final _targets = <int, (WeakReference<EventTarget>, int)>{};
  late final _collected = Finalizer<int>(_targets.remove);

  /// A token for a callback's userdata. Its events reach [target] as [kind]
  /// while the target exists.
  Pointer<Void> register(EventTarget target, int kind) {
    final token = simpledartble_token();
    _targets[token] = (WeakReference(target), kind);
    _collected.attach(target, token);
    return Pointer.fromAddress(token);
  }

  /// Stops delivering the events of [token].
  void unregister(Pointer<Void> token) => _targets.remove(token.address);

  void _receive(Object? message) {
    // NOTE: Another isolate's claim sends null to check this one still runs.
    if (message is! List) return;

    final token = message[0] as int;
    final Object? value = switch (message.length) {
      3 => Question._(message[1] as int, (message[2] as List).cast<String>()),
      2 => message[1],
      _ => null,
    };
    final registration = _targets[token];
    final target = registration?.$1.target;
    if (registration != null && target != null) {
      target.onEvent(registration.$2, value);
      return;
    }

    _targets.remove(token);
    switch (value) {
      case Question():
        value.fallback();
      // NOTE: Integers are only ever owned peripheral handles.
      case int():
        simpleble_peripheral_release_handle(Pointer.fromAddress(value));
    }
  }
}

/// The handler of one native callback that waits for an answer.
///
/// A new handler reaches the native object through `install`, in call order
/// with the object's other calls; the previous one answers until then. Handlers
/// stay alive while installed, even if their object doesn't.
final class HandlerSlot {
  _Handler? _current;

  /// Installs [receive], or clears the handler for null. [receive] gets what
  /// the callback passes, such as a [Question]. [install] registers the token,
  /// or a null pointer, with the native callback.
  /// The handler runs in the zone [set] was called in.
  Future<void> set(void Function(Object? value)? receive, Future<void> Function(Pointer<Void> token) install) async {
    final zone = Zone.current;
    final handler = receive == null ? null : _Handler((value) => zone.run(() => receive(value)));
    if (handler != null) {
      handler.token = Events.instance.register(handler, 0);
      Retained.retain(handler);
    }
    final previous = _current;
    _current = handler;
    try {
      await install(handler?.token ?? nullptr);
    } catch (_) {
      if (handler != null) _forget(handler);
      if (_current == handler) _current = previous;
      rethrow;
    }
    if (previous != null) _forget(previous);
  }

  /// Whether a handler is installed, or being installed.
  bool get isSet => _current != null;

  /// Forgets the handler of a native object that is gone.
  void drop() {
    final current = _current;
    _current = null;
    if (current != null) _forget(current);
  }

  void _forget(_Handler handler) {
    Events.instance.unregister(handler.token);
    Retained.release(handler);
  }
}

final class _Handler implements EventTarget {
  _Handler(this.receive);

  final void Function(Object? value) receive;
  Pointer<Void> token = nullptr;

  @override
  void onEvent(int kind, Object? value) => receive(value);
}
