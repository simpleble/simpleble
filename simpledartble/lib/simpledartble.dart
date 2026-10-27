/// SimpleBLE for Dart and Flutter.
///
/// Discover and connect Bluetooth peripherals, read and write GATT values,
/// subscribe to notifications, and host local GATT services through one API.
library;

import 'package:ffi/ffi.dart';

import 'src/internal/bindings.g.dart';

export 'src/adapter.dart' show Adapter;
export 'src/backend.dart' show Backend;
export 'src/config.dart';
export 'src/errors.dart';
export 'src/local.dart' show LocalCharacteristic, LocalPeripheral, LocalService;
export 'src/logging.dart';
export 'src/peripheral.dart' show AddressType, Peripheral;
export 'src/service.dart' show Characteristic, CharacteristicCapability, Descriptor, Service;

/// The SimpleBLE library under SimpleDartBLE.
abstract final class SimpleDartBle {
  /// The version of the native SimpleBLE library.
  static String get version => simpleble_get_version().cast<Utf8>().toDartString();
}
