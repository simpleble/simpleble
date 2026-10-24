/// One Dart object per native object, keyed by the value SimpleCBLE identifies
/// its handles with, or by a backend's identifier.
final class Registry<T extends Object> {
  final _objects = <Object, WeakReference<T>>{};
  late final _finalizer = Finalizer<Object>((identity) {
    if (_objects[identity]?.target == null) _objects.remove(identity);
  });

  /// The object for [identity], created if there is none. [discard] releases
  /// the new handle when an object exists, as an unused handle would otherwise
  /// keep the native object, and a Dongl's serial port, until collected.
  T intern(Object identity, T Function() create, void Function() discard) {
    final existing = _objects[identity]?.target;
    if (existing != null) {
      discard();
      return existing;
    }

    final object = create();
    _objects[identity] = WeakReference(object);
    _finalizer.attach(object, identity);
    return object;
  }

  /// Every object that still stands for a native object.
  Iterable<T> get objects => _objects.values.map((reference) => reference.target).nonNulls;

  /// Forgets [object], which no longer stands for a native object, so the next
  /// object for [identity] is a new one.
  void forget(Object identity, T object) {
    if (_objects[identity]?.target == object) _objects.remove(identity);
  }
}

/// Objects kept alive while they have listeners or subscriptions, so they keep
/// working after the caller drops its own reference.
abstract final class Retained {
  static final _counts = <Object, int>{};

  static void retain(Object object) => _counts[object] = (_counts[object] ?? 0) + 1;

  static void release(Object object) {
    final count = (_counts[object] ?? 0) - 1;
    if (count > 0) {
      _counts[object] = count;
    } else {
      _counts.remove(object);
    }
  }

  static void forget(Object object) => _counts.remove(object);
}
