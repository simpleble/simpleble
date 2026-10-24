import 'dart:async';
import 'dart:ffi';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';
import 'package:meta/meta.dart';

import 'errors.dart';
import 'internal/advanced.dart';
import 'internal/bindings.g.dart';
import 'internal/event_stream.dart';
import 'internal/events.dart';
import 'internal/native.dart';
import 'internal/registry.dart';
import 'internal/workers.dart';
import 'service.dart';

final _hosts = Registry<LocalPeripheral>();
final _services = Registry<LocalService>();
final _characteristics = Registry<LocalCharacteristic>();

final _releasePeripheral = Finalizer<int>((handle) => lifecycle.call(_releasePeripheralHandle, handle));
final _releaseService = Finalizer<int>((handle) => lifecycle.call(_releaseServiceHandle, handle));
final _releaseCharacteristic = Finalizer<int>((handle) => lifecycle.call(_releaseCharacteristicHandle, handle));

final _signal = Native.addressOf<NativeFunction<Void Function(Pointer<Void>, Pointer<Void>)>>(simpledartble_on_signal);
final _text = Native.addressOf<NativeFunction<Void Function(Pointer<Void>, Pointer<Char>, Pointer<Void>)>>(
  simpledartble_on_text,
);
final _bytes = Native.addressOf<NativeFunction<Void Function(Pointer<Void>, Pointer<Uint8>, Size, Pointer<Void>)>>(
  simpledartble_on_bytes,
);

/// A host for GATT services of this program, advertised by its adapter.
///
/// Configure services and the advertisement before [start]. A started local
/// peripheral stays alive and keeps a Dart program running until it stops.
final class LocalPeripheral {
  LocalPeripheral._(this._handle, this._adapter) {
    final events = Events.instance;
    final pointer = Pointer<Void>.fromAddress(_handle);
    _tokens = [for (final kind in _HostEvent.values) events.register(_target, kind.index)];
    simpleble_local_peripheral_set_callback_on_client_connected(
      pointer,
      _text,
      _tokens[_HostEvent.clientConnected.index],
    );
    simpleble_local_peripheral_set_callback_on_client_disconnected(
      pointer,
      _text,
      _tokens[_HostEvent.clientDisconnected.index],
    );
    _releasePeripheral.attach(this, _handle, detach: this);
  }

  /// The local peripheral for an owned [handle] from the adapter with identity [adapter].
  @internal
  static LocalPeripheral create(int handle, int adapter) =>
      _hosts.intern(handle, () => LocalPeripheral._(handle, adapter), () {});

  int _handle;
  final int _adapter;
  final _strand = Strand();
  late final _target = _HostTarget(this);
  late final List<Pointer<Void>> _tokens;
  bool _started = false;

  late final _onClientConnected = EventStream<String>(this);
  late final _onClientDisconnected = EventStream<String>(this);

  int get _live {
    if (_handle == 0) throw NotInitializedException('The adapter of this local peripheral was closed');
    return _handle;
  }

  Pointer<Void> get _pointer => Pointer.fromAddress(_live);

  /// Adds a service UUID to the advertisement.
  void addAdvertisedService(String uuid) => using(
    (arena) =>
        check((error) => simpleble_local_peripheral_add_advertised_service(_pointer, nativeUuid(uuid, arena), error)),
  );

  /// Advertises [data] as the manufacturer data of [companyId].
  void setAdvertisedManufacturerData(int companyId, Uint8List data) {
    RangeError.checkValueInInterval(companyId, 0, 0xffff, 'companyId');
    using(
      (arena) => check(
        (error) => simpleble_local_peripheral_set_advertised_manufacturer_data(
          _pointer,
          companyId,
          nativeBytes(data, arena),
          data.length,
          error,
        ),
      ),
    );
  }

  /// Chooses whether centrals can connect. Local peripherals are connectable by default.
  void setConnectable(bool connectable) =>
      check((error) => simpleble_local_peripheral_set_connectable(_pointer, connectable, error));

  /// Advertises [name] as the local name on iOS, macOS and Linux, or the
  /// system's name for null. Other platforms throw
  /// [OperationNotSupportedException].
  void setAdvertisementLocalName(String? name) => setAdvertisementLocalNameOf(_pointer, name);

  /// Adds a primary GATT service.
  LocalService addService(String uuid) {
    final handle = using(
      (arena) => check((error) => simpleble_local_peripheral_add_service(_pointer, nativeUuid(uuid, arena), error)),
    );
    return LocalService._wrap(handle.address, this);
  }

  List<LocalService> services() => [
    for (final handle in readHandles(
      (list, error) => simpleble_local_peripheral_services(_pointer, list, error),
      simpleble_local_service_list_get,
      simpleble_local_service_list_release,
      simpleble_local_service_release_handle,
    ))
      LocalService._wrap(handle, this),
  ];

  void removeAllServices() => check((error) => simpleble_local_peripheral_remove_all_services(_pointer, error));

  /// Publishes the services and starts advertising.
  Future<void> start() async {
    await _strand.call(_start, _live);
    _setStarted(true);
  }

  /// Stops advertising and disconnects the clients.
  Future<void> stop() async {
    await _strand.call(_stop, _live);
    _setStarted(false);
  }

  bool get isStarted => check((error) => simpleble_local_peripheral_is_started(_pointer, error));

  Future<bool> isAdvertising() async => _strand.call(_isAdvertising, _live);

  /// The address of each client that connects, or a platform identifier.
  Stream<String> get onClientConnected => _onClientConnected.stream;

  /// The address of each client that disconnects, or a platform identifier.
  Stream<String> get onClientDisconnected => _onClientDisconnected.stream;

  void _setStarted(bool started) {
    if (started == _started) return;
    _started = started;
    if (started) {
      KeepAlive.hold();
      Retained.retain(this);
    } else {
      KeepAlive.release();
      Retained.release(this);
    }
  }

  void _onEvent(int kind, Object? value) {
    switch (_HostEvent.values[kind]) {
      case _HostEvent.clientConnected:
        _onClientConnected.add(value! as String);
      case _HostEvent.clientDisconnected:
        _onClientDisconnected.add(value! as String);
    }
  }

  /// Stops this local peripheral if it came from the adapter with identity [adapter] and is started.
  @internal
  Future<void> stopFrom(int adapter) async {
    if (adapter == _adapter && _handle != 0 && _started) await stop();
  }

  /// Releases this local peripheral if it came from the closing adapter with identity [adapter].
  @internal
  void releaseFrom(int adapter) {
    if (adapter != _adapter || _handle == 0) return;
    simpleble_local_peripheral_set_callback_on_client_connected(Pointer.fromAddress(_handle), nullptr, nullptr);
    simpleble_local_peripheral_set_callback_on_client_disconnected(Pointer.fromAddress(_handle), nullptr, nullptr);
    for (final token in _tokens) {
      Events.instance.unregister(token);
    }
    _setStarted(false);
    _releasePeripheral.detach(this);
    lifecycle.call(_releasePeripheralHandle, _handle);
    _hosts.forget(_handle, this);
    _handle = 0;
    _onClientConnected.close();
    _onClientDisconnected.close();
    Retained.forget(this);
  }

  /// Every local peripheral, service and characteristic that still stands for a native object.
  @internal
  static Iterable<Object> get registered => [..._hosts.objects, ..._services.objects, ..._characteristics.objects];
}

enum _HostEvent { clientConnected, clientDisconnected }

final class _HostTarget implements EventTarget {
  _HostTarget(this.host);

  final LocalPeripheral host;

  @override
  void onEvent(int kind, Object? value) => host._onEvent(kind, value);
}

/// A GATT service of a [LocalPeripheral].
final class LocalService {
  LocalService._(this._handle, this._identity, this._host) {
    _releaseService.attach(this, _handle, detach: this);
  }

  static LocalService _wrap(int handle, LocalPeripheral host) {
    final identity = simpleble_local_service_identity(Pointer.fromAddress(handle)).address;
    return _services.intern(
      identity,
      () => LocalService._(handle, identity, host),
      () => lifecycle.call(_releaseServiceHandle, handle),
    );
  }

  int _handle;
  final int _identity;
  final LocalPeripheral _host;

  Pointer<Void> get _pointer {
    if (_handle == 0) throw NotInitializedException('The adapter of this local service was closed');
    return Pointer.fromAddress(_handle);
  }

  String get uuid => using((arena) {
    final uuid = arena<simpleble_uuid_t>();
    check((error) => simpleble_local_service_uuid(_pointer, uuid, error));
    return uuidOf(uuid.ref);
  });

  LocalCharacteristic addCharacteristic(String uuid, Set<CharacteristicCapability> capabilities) {
    final handle = using(
      (arena) => check(
        (error) => simpleble_local_service_add_characteristic(
          _pointer,
          nativeUuid(uuid, arena),
          capabilityFlags(capabilities),
          error,
        ),
      ),
    );
    return LocalCharacteristic._wrap(handle.address, _host);
  }

  List<LocalCharacteristic> characteristics() => [
    for (final handle in readHandles(
      (list, error) => simpleble_local_service_characteristics(_pointer, list, error),
      simpleble_local_characteristic_list_get,
      simpleble_local_characteristic_list_release,
      simpleble_local_characteristic_release_handle,
    ))
      LocalCharacteristic._wrap(handle, _host),
  ];

  /// Releases this service if it came from the closing adapter with identity [adapter].
  @internal
  void releaseFrom(int adapter) {
    if (adapter != _host._adapter || _handle == 0) return;
    _releaseService.detach(this);
    lifecycle.call(_releaseServiceHandle, _handle);
    _handle = 0;
    _services.forget(_identity, this);
  }
}

/// A characteristic of a [LocalService]. Reads return [value] unless a read
/// handler computes the value. While it has listeners it stays alive, even if
/// nothing else references it.
final class LocalCharacteristic {
  LocalCharacteristic._(this._handle, this._identity, this._host) {
    final events = Events.instance;
    final pointer = Pointer<Void>.fromAddress(_handle);
    _tokens = [for (final kind in _CharacteristicEvent.values) events.register(_target, kind.index)];
    simpleble_local_characteristic_set_callback_on_write(pointer, _bytes, _tokens[_CharacteristicEvent.write.index]);
    simpleble_local_characteristic_set_callback_on_subscribed(
      pointer,
      _signal,
      _tokens[_CharacteristicEvent.subscribed.index],
    );
    simpleble_local_characteristic_set_callback_on_unsubscribed(
      pointer,
      _signal,
      _tokens[_CharacteristicEvent.unsubscribed.index],
    );
    _releaseCharacteristic.attach(this, _handle, detach: this);
  }

  static LocalCharacteristic _wrap(int handle, LocalPeripheral host) {
    final identity = simpleble_local_characteristic_identity(Pointer.fromAddress(handle)).address;
    return _characteristics.intern(
      identity,
      () => LocalCharacteristic._(handle, identity, host),
      () => lifecycle.call(_releaseCharacteristicHandle, handle),
    );
  }

  int _handle;
  final int _identity;
  final LocalPeripheral _host;
  late final _target = _CharacteristicTarget(this);
  late final List<Pointer<Void>> _tokens;
  final _readHandler = HandlerSlot();

  late final _onWrite = EventStream<Uint8List>(this);
  late final _onSubscribed = EventStream<void>(this);
  late final _onUnsubscribed = EventStream<void>(this);

  int get _live {
    if (_handle == 0) throw NotInitializedException('The adapter of this local characteristic was closed');
    return _handle;
  }

  Pointer<Void> get _pointer => Pointer.fromAddress(_live);

  String get uuid => using((arena) {
    final uuid = arena<simpleble_uuid_t>();
    check((error) => simpleble_local_characteristic_uuid(_pointer, uuid, error));
    return uuidOf(uuid.ref);
  });

  Set<CharacteristicCapability> get capabilities =>
      capabilitiesOf(check((error) => simpleble_local_characteristic_capabilities(_pointer, error)));

  /// The stored value, a copy.
  Uint8List get value => using((arena) {
    final length = arena<Size>();
    final data = check((error) => simpleble_local_characteristic_value(_pointer, length, error));
    return takeBytes(data, length.value);
  });

  /// Stores [value] and publishes it to subscribed clients.
  Future<void> setValue(Uint8List value) async => _host._strand.call(_setValue, (_live, value));

  /// Computes the value of each read, or stops doing so with null. The handler
  /// can return a future, and the read waits for it. A read the handler fails
  /// gets an empty value, and the error is unhandled.
  Future<void> setReadHandler(FutureOr<Uint8List> Function()? handler) async {
    final characteristic = _live;
    return _readHandler.set(
      handler == null
          ? null
          : (value) {
              final question = value! as Question;
              question.answerWith(handler, question.answerBytes);
            },
      (token) => _host._strand.call(_setReadHandler, (characteristic, token.address)),
    );
  }

  /// Each value a client writes.
  Stream<Uint8List> get onWrite => _onWrite.stream;

  Stream<void> get onSubscribed => _onSubscribed.stream;

  Stream<void> get onUnsubscribed => _onUnsubscribed.stream;

  void _onEvent(int kind, Object? value) {
    switch (_CharacteristicEvent.values[kind]) {
      case _CharacteristicEvent.write:
        _onWrite.add(value! as Uint8List);
      case _CharacteristicEvent.subscribed:
        _onSubscribed.add(null);
      case _CharacteristicEvent.unsubscribed:
        _onUnsubscribed.add(null);
    }
  }

  /// Releases this characteristic if it came from the closing adapter with identity [adapter].
  @internal
  void releaseFrom(int adapter) {
    if (adapter != _host._adapter || _handle == 0) return;
    final pointer = Pointer<Void>.fromAddress(_handle);
    simpleble_local_characteristic_set_callback_on_write(pointer, nullptr, nullptr);
    simpleble_local_characteristic_set_callback_on_subscribed(pointer, nullptr, nullptr);
    simpleble_local_characteristic_set_callback_on_unsubscribed(pointer, nullptr, nullptr);
    for (final token in _tokens) {
      Events.instance.unregister(token);
    }
    _readHandler.drop();
    _releaseCharacteristic.detach(this);
    lifecycle.call(_releaseCharacteristicHandle, _handle);
    _handle = 0;
    _onWrite.close();
    _onSubscribed.close();
    _onUnsubscribed.close();
    Retained.forget(this);
    _characteristics.forget(_identity, this);
  }
}

enum _CharacteristicEvent { write, subscribed, unsubscribed }

final class _CharacteristicTarget implements EventTarget {
  _CharacteristicTarget(this.characteristic);

  final LocalCharacteristic characteristic;

  @override
  void onEvent(int kind, Object? value) => characteristic._onEvent(kind, value);
}

// The functions below run on workers, so they take handle addresses.

void _releasePeripheralHandle(int handle) => simpleble_local_peripheral_release_handle(Pointer.fromAddress(handle));

void _releaseServiceHandle(int handle) => simpleble_local_service_release_handle(Pointer.fromAddress(handle));

void _releaseCharacteristicHandle(int handle) =>
    simpleble_local_characteristic_release_handle(Pointer.fromAddress(handle));

void _start(int host) => check((error) => simpleble_local_peripheral_start(Pointer.fromAddress(host), error));

void _stop(int host) => check((error) => simpleble_local_peripheral_stop(Pointer.fromAddress(host), error));

bool _isAdvertising(int host) =>
    check((error) => simpleble_local_peripheral_is_advertising(Pointer.fromAddress(host), error));

void _setValue((int, Uint8List) arguments) => using((arena) {
  final (characteristic, value) = arguments;
  check(
    (error) => simpleble_local_characteristic_set_value(
      Pointer.fromAddress(characteristic),
      nativeBytes(value, arena),
      value.length,
      error,
    ),
  );
});

// NOTE: The core holds a callback's lock while it runs, and a read waits for
// the isolate that answers it, so the handler is replaced on a worker.
void _setReadHandler((int, int) arguments) {
  final (characteristic, token) = arguments;
  simpleble_local_characteristic_set_callback_on_read(
    Pointer.fromAddress(characteristic),
    token == 0 ? nullptr : Native.addressOf(simpledartble_on_read),
    Pointer.fromAddress(token),
  );
}
