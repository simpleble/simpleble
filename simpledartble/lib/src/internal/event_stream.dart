import 'dart:async';

import 'registry.dart';

/// The broadcast stream of one native event. While it has listeners, its
/// owner stays alive.
final class EventStream<T> {
  EventStream(Object owner)
    : _controller = StreamController<T>.broadcast(
        onListen: () => Retained.retain(owner),
        onCancel: () => Retained.release(owner),
      );

  final StreamController<T> _controller;

  Stream<T> get stream => _controller.stream;

  void add(T event) {
    if (!_controller.isClosed) _controller.add(event);
  }

  /// Ends the stream, for an object that no longer stands for a native object.
  void close() => _controller.close();
}
