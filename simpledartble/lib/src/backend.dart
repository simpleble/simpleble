import 'dart:ffi';

import 'adapter.dart';
import 'internal/bindings.g.dart';
import 'internal/events.dart';
import 'internal/native.dart';
import 'internal/registry.dart';
import 'internal/workers.dart';

// NOTE: SimpleBLE has one backend per identifier.
final _backends = Registry<Backend>();

final _release = Finalizer<int>((handle) => lifecycle.call(_releaseHandle, handle));

/// A Bluetooth stack SimpleBLE can use, such as the operating system's or a Dongl.
///
/// There is one Backend object per backend.
final class Backend {
  Backend._(this._handle, this.identifier) {
    _release.attach(this, _handle);
  }

  static Backend _wrap(int handle) {
    final identifier = takeString(check((error) => simpleble_backend_identifier(Pointer.fromAddress(handle), error)));
    return _backends.intern(
      identifier,
      () => Backend._(handle, identifier),
      () => lifecycle.call(_releaseHandle, handle),
    );
  }

  final int _handle;

  final String identifier;

  /// Every available backend.
  static Future<List<Backend>> getBackends() async {
    checkPlatform();
    return [for (final handle in await lifecycle.call(_getBackends, null)) _wrap(handle)];
  }

  /// The operating system's Bluetooth backend.
  static Future<Backend> getNative() async {
    checkPlatform();
    return _wrap(await lifecycle.call(_getNative, null));
  }

  /// The Dongl backend, regardless of `Config.dongl.useDonglBackend`.
  static Future<Backend> getDongl() async {
    checkPlatform();
    return _wrap(await lifecycle.call(_getDongl, null));
  }

  static Future<Backend> getSimulator() async {
    checkPlatform();
    return _wrap(await lifecycle.call(_getSimulator, null));
  }

  Future<bool> isBluetoothEnabled() => lifecycle.call(_isBluetoothEnabled, _handle);

  Future<List<Adapter>> getAdapters() async {
    // Claims the events first, so no handle is left over if another isolate has them.
    Events.instance;
    return [for (final handle in await lifecycle.call(_getAdapters, _handle)) Adapter.wrap(handle)];
  }

  @override
  String toString() => 'Backend($identifier)';
}

// The functions below run on workers, so they take handle addresses.

void _releaseHandle(int backend) => simpleble_backend_release_handle(Pointer.fromAddress(backend));

List<int> _getBackends(void _) => readHandles(
  simpleble_backend_get_backends,
  simpleble_backend_list_get,
  simpleble_backend_list_release,
  simpleble_backend_release_handle,
);

int _getNative(void _) => check(simpleble_backend_get_native).address;

int _getDongl(void _) => check(simpleble_backend_get_dongl).address;

int _getSimulator(void _) => check(simpleble_backend_get_simulator).address;

bool _isBluetoothEnabled(int backend) =>
    check((error) => simpleble_backend_is_bluetooth_enabled(Pointer.fromAddress(backend), error));

List<int> _getAdapters(int backend) => readHandles(
  (list, error) => simpleble_backend_get_adapters(Pointer.fromAddress(backend), list, error),
  simpleble_adapter_list_get,
  simpleble_adapter_list_release,
  simpleble_adapter_release_handle,
);
