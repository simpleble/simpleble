import 'dart:async';
import 'dart:ffi';

import 'package:meta/meta.dart';

import 'errors.dart';
import 'internal/advanced.dart';
import 'internal/bindings.g.dart';
import 'internal/event_stream.dart';
import 'internal/events.dart';
import 'internal/native.dart';
import 'internal/registry.dart';
import 'internal/workers.dart';
import 'local.dart';
import 'peripheral.dart';

final _adapters = Registry<Adapter>();

final _release = Finalizer<int>((handle) => lifecycle.call(_releaseHandle, handle));

/// A Bluetooth adapter.
///
/// There is one Adapter object per native adapter object. Most backends keep a
/// single native object per adapter, so every enumeration returns the same
/// Adapter. On Windows each enumeration creates a new native object, and
/// listeners on one Adapter only hear about calls made through it.
///
/// Events are delivered on the isolate that uses SimpleDartBLE, in the order
/// they happened. While an adapter has listeners it stays alive, even if
/// nothing else references it. [close] releases it and its peripherals.
final class Adapter {
  Adapter._(this._handle, this._identity) {
    final events = Events.instance;
    final pointer = Pointer<Void>.fromAddress(_handle);
    final signal = Native.addressOf<NativeFunction<Void Function(Pointer<Void>, Pointer<Void>)>>(
      simpledartble_on_signal,
    );
    final found = Native.addressOf<NativeFunction<Void Function(Pointer<Void>, Pointer<Void>, Pointer<Void>)>>(
      simpledartble_on_peripheral,
    );
    _tokens = [for (final kind in _Event.values) events.register(_target, kind.index)];
    simpleble_adapter_set_callback_on_scan_start(pointer, signal, _tokens[_Event.scanStart.index]);
    simpleble_adapter_set_callback_on_scan_stop(pointer, signal, _tokens[_Event.scanStop.index]);
    simpleble_adapter_set_callback_on_scan_found(pointer, found, _tokens[_Event.scanFound.index]);
    simpleble_adapter_set_callback_on_scan_updated(pointer, found, _tokens[_Event.scanUpdated.index]);
    simpleble_adapter_set_callback_on_power_on(pointer, signal, _tokens[_Event.powerOn.index]);
    simpleble_adapter_set_callback_on_power_off(pointer, signal, _tokens[_Event.powerOff.index]);
    _release.attach(this, _handle, detach: this);
  }

  /// The adapter for an owned [handle].
  @internal
  static Adapter wrap(int handle) {
    final identity = simpleble_adapter_identity(Pointer.fromAddress(handle)).address;
    return _adapters.intern(identity, () => Adapter._(handle, identity), () => lifecycle.call(_releaseHandle, handle));
  }

  final int _handle;
  final int _identity;
  final _strand = Strand();
  late final _target = _Target(this);
  late final List<Pointer<Void>> _tokens;
  Future<void>? _closing;
  bool _scanning = false;

  late final _onScanStart = EventStream<void>(this);
  late final _onScanStop = EventStream<void>(this);
  late final _onScanFound = EventStream<Peripheral>(this);
  late final _onScanUpdated = EventStream<Peripheral>(this);
  late final _onPowerOn = EventStream<void>(this);
  late final _onPowerOff = EventStream<void>(this);

  /// Every adapter of every available backend.
  static Future<List<Adapter>> getAdapters() async {
    checkPlatform();
    // Claims the events first, so no handle is left over if another isolate has them.
    Events.instance;
    return [for (final handle in await lifecycle.call(_getAdapters, null)) wrap(handle)];
  }

  /// Whether Bluetooth is enabled on any adapter.
  static Future<bool> isBluetoothEnabled() async {
    checkPlatform();
    return lifecycle.call(_isBluetoothEnabled, null);
  }

  int get _live {
    if (_closing != null) throw NotInitializedException('The adapter was closed');
    return _handle;
  }

  String get identifier =>
      takeString(check((error) => simpleble_adapter_identifier(Pointer.fromAddress(_live), error)));

  String get address => takeString(check((error) => simpleble_adapter_address(Pointer.fromAddress(_live), error)));

  Future<bool> isPowered() async => _strand.call(_isPowered, _live);

  Future<void> powerOn() async => _strand.call(_powerOn, _live);

  Future<void> powerOff() async => _strand.call(_powerOff, _live);

  Stream<void> get onPowerOn => _onPowerOn.stream;

  Stream<void> get onPowerOff => _onPowerOff.stream;

  Future<void> scanStart() async {
    if (await _strand.call(_scanStart, _live)) _setScanning(true);
  }

  Future<void> scanStop() async {
    await _strand.call(_scanStop, _live);
    _setScanning(false);
  }

  /// Scans for [duration], then stops. Other calls on this adapter wait for it.
  Future<void> scanFor(Duration duration) async {
    final milliseconds = duration.inMilliseconds;
    if (milliseconds < -0x80000000 || milliseconds > 0x7fffffff) {
      throw ArgumentError.value(duration, 'duration', 'Must fit in 32-bit milliseconds');
    }
    await _strand.call(_scanFor, (_live, milliseconds));
    _setScanning(false);
  }

  Future<bool> scanIsActive() async => _strand.call(_scanIsActive, _live);

  /// The peripherals seen by the current or last scan.
  List<Peripheral> scanGetResults() => _peripherals(_scanResults(_live));

  Future<List<Peripheral>> getPairedPeripherals() async => _peripherals(await _strand.call(_pairedPeripherals, _live));

  Future<List<Peripheral>> getConnectedPeripherals() async =>
      _peripherals(await _strand.call(_connectedPeripherals, _live));

  Stream<void> get onScanStart => _onScanStart.stream;

  Stream<void> get onScanStop => _onScanStop.stream;

  Stream<Peripheral> get onScanFound => _onScanFound.stream;

  Stream<Peripheral> get onScanUpdated => _onScanUpdated.stream;

  /// The peripherals CoreBluetooth already knows on iOS and macOS, by the
  /// identifiers [Peripheral.address] returned earlier, without scanning.
  /// Unknown identifiers are skipped, and a returned peripheral might be out of
  /// range. Other platforms throw [OperationNotSupportedException].
  Future<List<Peripheral>> retrieveCachedPeripherals(List<String> identifiers) async =>
      _peripherals(await _strand.call(readCachedPeripherals, (_live, List.of(identifiers))));

  /// A host for GATT services of this program, advertised by this adapter.
  LocalPeripheral createLocalPeripheral() => LocalPeripheral.create(
    check((error) => simpleble_adapter_create_local_peripheral(Pointer.fromAddress(_live), error)).address,
    _identity,
  );

  /// Stops this adapter's scan and local peripherals, disconnects the
  /// peripherals connected through it, and releases the adapter and every
  /// object that came from it. Once the future completes, a Dongl is free for
  /// other programs. Calls on the closed objects then throw
  /// [NotInitializedException], their streams end, and [getAdapters] returns
  /// a new Adapter for the device.
  Future<void> close() => _closing ??= _close();

  Future<void> _close() async {
    // NOTE: A Dongl whose port closes during a scan can stop answering until
    // it's replugged, so the scan and the connections end first. Failures
    // don't stop the release.
    try {
      if (await _strand.call(_scanIsActive, _handle)) await _strand.call(_scanStop, _handle);
    } catch (_) {}
    final peripherals = Peripheral.registered.toList();
    final local = LocalPeripheral.registered.toList();
    await Future.wait([
      for (final peripheral in peripherals)
        if (peripheral.isConnectedThrough(_identity)) peripheral.disconnect().catchError((_) {}),
      for (final host in local.whereType<LocalPeripheral>()) host.stopFrom(_identity).catchError((_) {}),
    ]);

    _clearCallbacks(Pointer.fromAddress(_handle));
    for (final token in _tokens) {
      Events.instance.unregister(token);
    }
    for (final peripheral in peripherals) {
      peripheral.releaseFrom(_identity);
    }
    for (final object in local) {
      switch (object) {
        case LocalPeripheral():
          object.releaseFrom(_identity);
        case LocalService():
          object.releaseFrom(_identity);
        case LocalCharacteristic():
          object.releaseFrom(_identity);
      }
    }
    _release.detach(this);
    await lifecycle.call(_releaseHandle, _handle);

    _setScanning(false);
    for (final stream in [_onScanStart, _onScanStop, _onPowerOn, _onPowerOff]) {
      stream.close();
    }
    _onScanFound.close();
    _onScanUpdated.close();
    Retained.forget(this);
    _adapters.forget(_identity, this);
  }

  List<Peripheral> _peripherals(List<int> handles) => [
    for (final handle in handles) Peripheral.wrap(handle, _identity),
  ];

  // An active scan keeps the program running until it stops, and keeps this
  // adapter alive to hear that it stopped.
  //
  // NOTE: Some backends report a scan starting or stopping only after the call
  // returns, so both the event and the call record it; the second is a no-op.
  // Powering off ends a scan without a scan-stop callback on several backends,
  // so it counts as a stop too.
  void _setScanning(bool scanning) {
    if (scanning == _scanning) return;
    _scanning = scanning;
    if (scanning) {
      KeepAlive.hold();
      Retained.retain(this);
    } else {
      KeepAlive.release();
      Retained.release(this);
    }
  }

  void _onEvent(int kind, Object? value) {
    switch (_Event.values[kind]) {
      case _Event.scanStart:
        _setScanning(true);
        _onScanStart.add(null);
      case _Event.scanStop:
        _setScanning(false);
        _onScanStop.add(null);
      case _Event.scanFound:
        _onScanFound.add(Peripheral.wrap(value! as int, _identity));
      case _Event.scanUpdated:
        _onScanUpdated.add(Peripheral.wrap(value! as int, _identity));
      case _Event.powerOn:
        _onPowerOn.add(null);
      case _Event.powerOff:
        _setScanning(false);
        _onPowerOff.add(null);
    }
  }
}

enum _Event { scanStart, scanStop, scanFound, scanUpdated, powerOn, powerOff }

// Receives the adapter's events, and lives exactly as long as the adapter.
final class _Target implements EventTarget {
  _Target(this.adapter);

  final Adapter adapter;

  @override
  void onEvent(int kind, Object? value) => adapter._onEvent(kind, value);
}

void _clearCallbacks(Pointer<Void> adapter) {
  simpleble_adapter_set_callback_on_scan_start(adapter, nullptr, nullptr);
  simpleble_adapter_set_callback_on_scan_stop(adapter, nullptr, nullptr);
  simpleble_adapter_set_callback_on_scan_found(adapter, nullptr, nullptr);
  simpleble_adapter_set_callback_on_scan_updated(adapter, nullptr, nullptr);
  simpleble_adapter_set_callback_on_power_on(adapter, nullptr, nullptr);
  simpleble_adapter_set_callback_on_power_off(adapter, nullptr, nullptr);
}

// The functions below run on workers, so they take handle addresses.

void _releaseHandle(int adapter) => simpleble_adapter_release_handle(Pointer.fromAddress(adapter));

List<int> _getAdapters(void _) => readHandles(
  simpleble_adapter_get_adapters,
  simpleble_adapter_list_get,
  simpleble_adapter_list_release,
  simpleble_adapter_release_handle,
);

bool _isBluetoothEnabled(void _) => check(simpleble_adapter_is_bluetooth_enabled);

bool _isPowered(int adapter) => check((error) => simpleble_adapter_is_powered(Pointer.fromAddress(adapter), error));

void _powerOn(int adapter) => check((error) => simpleble_adapter_power_on(Pointer.fromAddress(adapter), error));

void _powerOff(int adapter) => check((error) => simpleble_adapter_power_off(Pointer.fromAddress(adapter), error));

// NOTE: With Bluetooth off, SimpleBLE logs a warning and doesn't scan, so only a
// scan that is active or may still report starting keeps the program running.
bool _scanStart(int adapter) {
  final pointer = Pointer<Void>.fromAddress(adapter);
  check((error) => simpleble_adapter_scan_start(pointer, error));
  return check((error) => simpleble_adapter_scan_is_active(pointer, error)) ||
      check((error) => simpleble_adapter_is_powered(pointer, error));
}

void _scanStop(int adapter) => check((error) => simpleble_adapter_scan_stop(Pointer.fromAddress(adapter), error));

void _scanFor((int, int) arguments) {
  final (adapter, milliseconds) = arguments;
  check((error) => simpleble_adapter_scan_for(Pointer.fromAddress(adapter), milliseconds, error));
}

bool _scanIsActive(int adapter) =>
    check((error) => simpleble_adapter_scan_is_active(Pointer.fromAddress(adapter), error));

List<int> _scanResults(int adapter) => _readPeripherals(adapter, simpleble_adapter_scan_get_results);

List<int> _pairedPeripherals(int adapter) => _readPeripherals(adapter, simpleble_adapter_get_paired_peripherals);

List<int> _connectedPeripherals(int adapter) => _readPeripherals(adapter, simpleble_adapter_get_connected_peripherals);

List<int> _readPeripherals(
  int adapter,
  int Function(Pointer<Void> adapter, Pointer<Pointer<Void>> list, ErrorVariable error) read,
) => readHandles(
  (list, error) => read(Pointer.fromAddress(adapter), list, error),
  simpleble_peripheral_list_get,
  simpleble_peripheral_list_release,
  simpleble_peripheral_release_handle,
);
