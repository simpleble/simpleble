import 'dart:async';
import 'dart:ffi';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';
import 'package:meta/meta.dart';

import 'errors.dart';
import 'internal/bindings.g.dart';
import 'internal/event_stream.dart';
import 'internal/events.dart';
import 'internal/native.dart';
import 'internal/registry.dart';
import 'internal/workers.dart';
import 'service.dart';

/// How a peripheral's address was assigned.
enum AddressType {
  /// A public address, registered with the IEEE.
  public,

  /// A random address, static or private.
  random,

  /// The platform doesn't report the address type.
  unspecified,
}

final _peripherals = Registry<Peripheral>();

final _release = Finalizer<int>((handle) => lifecycle.call(_releaseHandle, handle));

final _signal = Native.addressOf<NativeFunction<Void Function(Pointer<Void>, Pointer<Void>)>>(simpledartble_on_signal);

final _value =
    Native.addressOf<
      NativeFunction<
        Void Function(Pointer<Void>, simpleble_uuid_t, simpleble_uuid_t, Pointer<Uint8>, Size, Pointer<Void>)
      >
    >(simpledartble_on_value);

/// A remote device.
///
/// There is one Peripheral object per native peripheral object, so an
/// adapter's events, scan results and peripheral lists return the same object
/// for the same device.
///
/// A peripheral with listeners, subscriptions or an open connection stays
/// alive, even if nothing else references it. An open connection keeps a Dart
/// program running until it ends.
final class Peripheral {
  Peripheral._(this._handle, this._identity, this._adapter) {
    final events = Events.instance;
    final pointer = Pointer<Void>.fromAddress(_handle);
    _tokens = [for (final kind in _Event.values) events.register(_target, kind.index)];
    simpleble_peripheral_set_callback_on_connected(pointer, _signal, _tokens[_Event.connected.index]);
    simpleble_peripheral_set_callback_on_disconnected(pointer, _signal, _tokens[_Event.disconnected.index]);
    _release.attach(this, _handle, detach: this);
  }

  /// The peripheral for an owned [handle] from the adapter with identity [adapter].
  @internal
  static Peripheral wrap(int handle, int adapter) {
    final identity = simpleble_peripheral_identity(Pointer.fromAddress(handle)).address;
    return _peripherals.intern(
      identity,
      () => Peripheral._(handle, identity, adapter),
      () => lifecycle.call(_releaseHandle, handle),
    );
  }

  int _handle;
  final int _identity;
  final int _adapter;
  final _strand = Strand();
  late final _target = _Target(this);
  late final List<Pointer<Void>> _tokens;
  bool _connected = false;

  // Subscriptions by _subscriptionKey().
  final _subscriptions = <String, _Subscription>{};
  final _passkeyRequest = HandlerSlot();
  final _passkeyDisplay = HandlerSlot();
  final _numericComparison = HandlerSlot();

  late final _onConnected = EventStream<void>(this);
  late final _onDisconnected = EventStream<void>(this);

  int get _live {
    if (_handle == 0) throw NotInitializedException('The adapter of this peripheral was closed');
    return _handle;
  }

  Pointer<Void> get _pointer => Pointer.fromAddress(_live);

  String get identifier => takeString(check((error) => simpleble_peripheral_identifier(_pointer, error)));

  String get address => takeString(check((error) => simpleble_peripheral_address(_pointer, error)));

  AddressType get addressType =>
      AddressType.values[check((error) => simpleble_peripheral_address_type(_pointer, error)).value];

  /// The last received signal strength in dBm, if any.
  int? get rssi => _optional(check((error) => simpleble_peripheral_rssi(_pointer, error)));

  /// The advertised transmit power in dBm, if any.
  int? get txPower => _optional(check((error) => simpleble_peripheral_tx_power(_pointer, error)));

  int get mtu => check((error) => simpleble_peripheral_mtu(_pointer, error));

  bool get isConnected => check((error) => simpleble_peripheral_is_connected(_pointer, error));

  bool get isConnectable => check((error) => simpleble_peripheral_is_connectable(_pointer, error));

  Future<bool> isPaired() async => _strand.call(_isPaired, _live);

  Future<void> unpair() async => _strand.call(_unpair, _live);

  /// Advertised manufacturer data by company identifier. The values are copies.
  Map<int, Uint8List> manufacturerData() {
    final list = calloc<Pointer<Void>>();
    final entry = calloc<simpleble_manufacturer_data_t>();
    try {
      final count = check((error) => simpleble_peripheral_manufacturer_data(_pointer, list, error));
      final data = <int, Uint8List>{};
      for (var i = 0; i < count; i++) {
        check((error) => simpleble_manufacturer_data_list_get(list.value, i, entry, error));
        data[entry.ref.manufacturer_id] = copyBytes(entry.ref.data, entry.ref.data_length);
        simpleble_manufacturer_data_release(entry);
      }
      return data;
    } finally {
      simpleble_manufacturer_data_release(entry);
      simpleble_manufacturer_data_list_release(list.value);
      calloc.free(entry);
      calloc.free(list);
    }
  }

  /// The services of a connected peripheral, or the advertised ones before connecting.
  List<Service> services() {
    final list = calloc<Pointer<Void>>();
    final entry = calloc<simpleble_service_t>();
    try {
      final count = check((error) => simpleble_peripheral_services(_pointer, list, error));
      final services = <Service>[];
      for (var i = 0; i < count; i++) {
        check((error) => simpleble_service_list_get(list.value, i, entry, error));
        services.add(serviceOf(entry.ref));
        simpleble_service_release(entry);
      }
      return services;
    } finally {
      simpleble_service_release(entry);
      simpleble_service_list_release(list.value);
      calloc.free(entry);
      calloc.free(list);
    }
  }

  Future<void> connect() async {
    await _strand.call(_connect, _live);
    _setConnected(true);
  }

  /// Disconnects, which ends the subscriptions made before it.
  Future<void> disconnect() async {
    // NOTE: Calls run in call order, so this ends the subscriptions made
    // before it, and not those of calls made while it runs.
    final ended = {..._subscriptions};
    await _strand.call(_disconnect, _live);
    ended.forEach(_forget);
    _setConnected(false);
  }

  Stream<void> get onConnected => _onConnected.stream;

  Stream<void> get onDisconnected => _onDisconnected.stream;

  Future<Uint8List> read(String service, String characteristic) async {
    checkUuid(service, 'service');
    checkUuid(characteristic, 'characteristic');
    return _strand.call(_read, (_live, service, characteristic));
  }

  Future<void> writeRequest(String service, String characteristic, Uint8List data) async {
    checkUuid(service, 'service');
    checkUuid(characteristic, 'characteristic');
    return _strand.call(_writeRequest, (_live, service, characteristic, data));
  }

  Future<void> writeCommand(String service, String characteristic, Uint8List data) async {
    checkUuid(service, 'service');
    checkUuid(characteristic, 'characteristic');
    return _strand.call(_writeCommand, (_live, service, characteristic, data));
  }

  Future<Uint8List> readDescriptor(String service, String characteristic, String descriptor) async {
    checkUuid(service, 'service');
    checkUuid(characteristic, 'characteristic');
    checkUuid(descriptor, 'descriptor');
    return _strand.call(_readDescriptor, (_live, service, characteristic, descriptor));
  }

  Future<void> writeDescriptor(String service, String characteristic, String descriptor, Uint8List data) async {
    checkUuid(service, 'service');
    checkUuid(characteristic, 'characteristic');
    checkUuid(descriptor, 'descriptor');
    return _strand.call(_writeDescriptor, (_live, service, characteristic, descriptor, data));
  }

  /// Subscribes to notifications. The stream has each notified value until
  /// [unsubscribe], a disconnection, or another subscription to the same
  /// characteristic ends it. Cancelling it unsubscribes.
  Future<Stream<Uint8List>> notify(String service, String characteristic) =>
      _subscribe(_notify, service, characteristic);

  /// Subscribes to indications, like [notify] does to notifications.
  Future<Stream<Uint8List>> indicate(String service, String characteristic) =>
      _subscribe(_indicate, service, characteristic);

  /// Unsubscribes, which ends the subscription made before it.
  Future<void> unsubscribe(String service, String characteristic) async {
    checkUuid(service, 'service');
    checkUuid(characteristic, 'characteristic');
    final key = _subscriptionKey(service, characteristic);
    final ended = _subscriptions[key];
    await _strand.call(_unsubscribe, (_live, service, characteristic));
    if (ended != null) _forget(key, ended);
  }

  /// Answers a Dongl's request for the passkey the peer shows, with six digits,
  /// or null to reject pairing. Set it before [connect], as pairing can start
  /// as soon as the link is up. The handler can return a future, such as of the
  /// user's input, and the Dongl waits for it.
  Future<void> setPasskeyRequestHandler(FutureOr<String?> Function()? handler) async {
    final peripheral = _live;
    return _passkeyRequest.set(
      handler == null
          ? null
          : (value) => (value! as Question).answerWith(handler, (String? passkey) {
              if (passkey != null && !RegExp(r'^[0-9]{6}$').hasMatch(passkey)) {
                throw ArgumentError.value(passkey, 'passkey', 'A passkey has six digits');
              }
              (value as Question).answerText(passkey);
            }),
      (token) => _strand.call(_setPasskeyRequest, (peripheral, token.address)),
    );
  }

  /// Shows a Dongl's six-digit passkey, which the user enters on the peer. Set it before [connect].
  Future<void> setPasskeyDisplayHandler(void Function(String passkey)? handler) async {
    final peripheral = _live;
    return _passkeyDisplay.set(
      handler == null ? null : (value) => handler(value! as String),
      (token) => _strand.call(_setPasskeyDisplay, (peripheral, token.address)),
    );
  }

  /// Answers whether the peer shows the same six-digit number as a Dongl,
  /// accepting or rejecting pairing. Set it before [connect]. The handler can
  /// return a future.
  Future<void> setNumericComparisonHandler(FutureOr<bool> Function(String passkey)? handler) async {
    final peripheral = _live;
    return _numericComparison.set(
      handler == null
          ? null
          : (value) {
              final question = value! as Question;
              question.answerWith(() => handler(question.argument!), question.answerFlag);
            },
      (token) => _strand.call(_setNumericComparison, (peripheral, token.address)),
    );
  }

  /// Secures a Dongl connection with LE Secure Connections. With [mitm], the
  /// peer must also be authenticated through the passkey or numeric comparison
  /// handlers. Pairing started by this call creates no bond, and the Dongl
  /// disconnects if it fails.
  Future<void> secureConnection({bool mitm = false}) async => _strand.call(_secureConnection, (_live, mitm));

  Future<Stream<Uint8List>> _subscribe(
    void Function((int, String, String, int, int) arguments) start,
    String service,
    String characteristic,
  ) async {
    checkUuid(service, 'service');
    checkUuid(characteristic, 'characteristic');
    final peripheral = _live;

    // The subscription is in place before the first value can arrive. A new
    // subscription to the same characteristic replaces the previous one.
    final key = _subscriptionKey(service, characteristic);
    final subscription = _Subscription(this, service, characteristic);
    _remember(key, subscription);
    try {
      await _strand.call(start, (peripheral, service, characteristic, _value.address, subscription.token.address));
    } catch (_) {
      _forget(key, subscription);
      rethrow;
    }
    subscription.settled = true;
    return subscription.values.stream;
  }

  void _remember(String key, _Subscription subscription) {
    final previous = _subscriptions[key];
    if (previous != null) {
      previous.end();
    } else {
      Retained.retain(this);
    }
    _subscriptions[key] = subscription;
  }

  // Forgets [subscription] unless a later one has replaced it.
  void _forget(String key, _Subscription subscription) {
    if (_subscriptions[key] != subscription) return;
    _subscriptions.remove(key);
    subscription.end();
    Retained.release(this);
  }

  // An open connection keeps the program running until it ends, and keeps this
  // peripheral alive to hear that it ended.
  //
  // NOTE: Some backends report a connection or disconnection only after the
  // call returns, so both the event and the call record it; the second is a
  // no-op.
  void _setConnected(bool connected) {
    if (connected == _connected) return;
    _connected = connected;
    if (connected) {
      KeepAlive.hold();
      Retained.retain(this);
    } else {
      KeepAlive.release();
      Retained.release(this);
    }
  }

  void _onEvent(int kind, Object? value) {
    switch (_Event.values[kind]) {
      case _Event.connected:
        _setConnected(true);
        _onConnected.add(null);
      case _Event.disconnected:
        // A disconnection ends the subscriptions made before it. One still
        // being made either started after it or fails, which forgets it.
        for (final MapEntry(:key, value: subscription) in {..._subscriptions}.entries) {
          if (subscription.settled) _forget(key, subscription);
        }
        _setConnected(false);
        _onDisconnected.add(null);
    }
  }

  /// Whether this peripheral came from the adapter with identity [adapter] and is connected.
  @internal
  bool isConnectedThrough(int adapter) => adapter == _adapter && _handle != 0 && _connected;

  /// Releases this peripheral if it came from the closing adapter with identity [adapter].
  @internal
  void releaseFrom(int adapter) {
    if (adapter != _adapter || _handle == 0) return;
    for (final MapEntry(:key, value: subscription) in {..._subscriptions}.entries) {
      _forget(key, subscription);
    }
    for (final slot in [_passkeyRequest, _passkeyDisplay, _numericComparison]) {
      slot.drop();
    }
    _clearCallbacks(Pointer.fromAddress(_handle));
    for (final token in _tokens) {
      Events.instance.unregister(token);
    }
    _setConnected(false);
    _release.detach(this);
    lifecycle.call(_releaseHandle, _handle);
    _handle = 0;
    _onConnected.close();
    _onDisconnected.close();
    Retained.forget(this);
    _peripherals.forget(_identity, this);
  }

  /// Every peripheral that still stands for a native object.
  @internal
  static Iterable<Peripheral> get registered => _peripherals.objects;
}

enum _Event { connected, disconnected }

// Receives the peripheral's events, and lives exactly as long as the peripheral.
final class _Target implements EventTarget {
  _Target(this.peripheral);

  final Peripheral peripheral;

  @override
  void onEvent(int kind, Object? value) => peripheral._onEvent(kind, value);
}

// One notification or indication subscription, and the stream of its values.
final class _Subscription implements EventTarget {
  _Subscription(this.peripheral, this.service, this.characteristic) {
    values = StreamController<Uint8List>(onCancel: _cancel);
    token = Events.instance.register(this, 0);
  }

  final Peripheral peripheral;
  final String service;
  final String characteristic;
  late final StreamController<Uint8List> values;
  late final Pointer<Void> token;

  // Whether the call that made it has succeeded.
  bool settled = false;

  @override
  void onEvent(int kind, Object? value) {
    if (!values.isClosed) values.add(value! as Uint8List);
  }

  void end() {
    Events.instance.unregister(token);
    values.close();
  }

  Future<void>? _cancel() {
    if (peripheral._subscriptions[_subscriptionKey(service, characteristic)] != this) return null;
    return peripheral.unsubscribe(service, characteristic);
  }
}

// NOTE: Some backends, like CoreBluetooth, match UUIDs regardless of case, and
// the others match only lowercase ones. Lowercase keys name the characteristic
// the backend uses either way.
String _subscriptionKey(String service, String characteristic) => '$service\n$characteristic'.toLowerCase();

int? _optional(int value) => value == -0x8000 ? null : value;

void _clearCallbacks(Pointer<Void> peripheral) {
  simpleble_peripheral_set_callback_on_connected(peripheral, nullptr, nullptr);
  simpleble_peripheral_set_callback_on_disconnected(peripheral, nullptr, nullptr);
}

// The functions below run on workers, so they take handle addresses.

void _releaseHandle(int handle) => simpleble_peripheral_release_handle(Pointer.fromAddress(handle));

bool _isPaired(int peripheral) =>
    check((error) => simpleble_peripheral_is_paired(Pointer.fromAddress(peripheral), error));

void _unpair(int peripheral) => check((error) => simpleble_peripheral_unpair(Pointer.fromAddress(peripheral), error));

void _connect(int peripheral) => check((error) => simpleble_peripheral_connect(Pointer.fromAddress(peripheral), error));

void _disconnect(int peripheral) =>
    check((error) => simpleble_peripheral_disconnect(Pointer.fromAddress(peripheral), error));

Uint8List _read((int, String, String) arguments) => using((arena) {
  final (peripheral, service, characteristic) = arguments;
  final length = arena<Size>();
  final data = check(
    (error) => simpleble_peripheral_read(
      Pointer.fromAddress(peripheral),
      nativeUuid(service, arena),
      nativeUuid(characteristic, arena),
      length,
      error,
    ),
  );
  return takeBytes(data, length.value);
});

void _writeRequest((int, String, String, Uint8List) arguments) => using((arena) {
  final (peripheral, service, characteristic, data) = arguments;
  check(
    (error) => simpleble_peripheral_write_request(
      Pointer.fromAddress(peripheral),
      nativeUuid(service, arena),
      nativeUuid(characteristic, arena),
      nativeBytes(data, arena),
      data.length,
      error,
    ),
  );
});

void _writeCommand((int, String, String, Uint8List) arguments) => using((arena) {
  final (peripheral, service, characteristic, data) = arguments;
  check(
    (error) => simpleble_peripheral_write_command(
      Pointer.fromAddress(peripheral),
      nativeUuid(service, arena),
      nativeUuid(characteristic, arena),
      nativeBytes(data, arena),
      data.length,
      error,
    ),
  );
});

Uint8List _readDescriptor((int, String, String, String) arguments) => using((arena) {
  final (peripheral, service, characteristic, descriptor) = arguments;
  final length = arena<Size>();
  final data = check(
    (error) => simpleble_peripheral_read_descriptor(
      Pointer.fromAddress(peripheral),
      nativeUuid(service, arena),
      nativeUuid(characteristic, arena),
      nativeUuid(descriptor, arena),
      length,
      error,
    ),
  );
  return takeBytes(data, length.value);
});

void _writeDescriptor((int, String, String, String, Uint8List) arguments) => using((arena) {
  final (peripheral, service, characteristic, descriptor, data) = arguments;
  check(
    (error) => simpleble_peripheral_write_descriptor(
      Pointer.fromAddress(peripheral),
      nativeUuid(service, arena),
      nativeUuid(characteristic, arena),
      nativeUuid(descriptor, arena),
      nativeBytes(data, arena),
      data.length,
      error,
    ),
  );
});

typedef _ValueCallback =
    NativeFunction<
      Void Function(Pointer<Void>, simpleble_uuid_t, simpleble_uuid_t, Pointer<Uint8>, Size, Pointer<Void>)
    >;

void _notify((int, String, String, int, int) arguments) => using((arena) {
  final (peripheral, service, characteristic, callback, token) = arguments;
  check(
    (error) => simpleble_peripheral_notify(
      Pointer.fromAddress(peripheral),
      nativeUuid(service, arena),
      nativeUuid(characteristic, arena),
      Pointer<_ValueCallback>.fromAddress(callback),
      Pointer.fromAddress(token),
      error,
    ),
  );
});

void _indicate((int, String, String, int, int) arguments) => using((arena) {
  final (peripheral, service, characteristic, callback, token) = arguments;
  check(
    (error) => simpleble_peripheral_indicate(
      Pointer.fromAddress(peripheral),
      nativeUuid(service, arena),
      nativeUuid(characteristic, arena),
      Pointer<_ValueCallback>.fromAddress(callback),
      Pointer.fromAddress(token),
      error,
    ),
  );
});

void _unsubscribe((int, String, String) arguments) => using((arena) {
  final (peripheral, service, characteristic) = arguments;
  check(
    (error) => simpleble_peripheral_unsubscribe(
      Pointer.fromAddress(peripheral),
      nativeUuid(service, arena),
      nativeUuid(characteristic, arena),
      error,
    ),
  );
});

// NOTE: The core holds a callback's lock while it runs, and a pairing question
// waits for the isolate that answers it, so handlers are replaced on workers.
// Peripherals of other backends reject them with OPERATION_NOT_SUPPORTED.

void _setPasskeyRequest((int, int) arguments) {
  final (peripheral, token) = arguments;
  check(
    (error) => simpleble_advanced_dongl_set_passkey_request_callback(
      Pointer.fromAddress(peripheral),
      token == 0 ? nullptr : Native.addressOf(simpledartble_on_passkey_request),
      Pointer.fromAddress(token),
      error,
    ),
  );
}

void _setPasskeyDisplay((int, int) arguments) {
  final (peripheral, token) = arguments;
  check(
    (error) => simpleble_advanced_dongl_set_passkey_display_callback(
      Pointer.fromAddress(peripheral),
      token == 0 ? nullptr : Native.addressOf(simpledartble_on_text),
      Pointer.fromAddress(token),
      error,
    ),
  );
}

void _setNumericComparison((int, int) arguments) {
  final (peripheral, token) = arguments;
  check(
    (error) => simpleble_advanced_dongl_set_numeric_comparison_callback(
      Pointer.fromAddress(peripheral),
      token == 0 ? nullptr : Native.addressOf(simpledartble_on_numeric_comparison),
      Pointer.fromAddress(token),
      error,
    ),
  );
}

void _secureConnection((int, bool) arguments) {
  final (peripheral, mitm) = arguments;
  check((error) => simpleble_advanced_dongl_secure_connection(Pointer.fromAddress(peripheral), mitm, error));
}
