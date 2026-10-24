import 'dart:ffi';
import 'dart:typed_data';

import 'internal/bindings.g.dart';
import 'internal/native.dart';

/// What a characteristic supports.
enum CharacteristicCapability { read, writeRequest, writeCommand, notify, indicate }

/// A GATT descriptor of a peripheral's characteristic.
final class Descriptor {
  const Descriptor(this.uuid);

  final String uuid;

  @override
  String toString() => 'Descriptor($uuid)';
}

/// A GATT characteristic of a peripheral's service.
final class Characteristic {
  Characteristic(this.uuid, Set<CharacteristicCapability> capabilities, List<Descriptor> descriptors)
    : capabilities = Set.unmodifiable(capabilities),
      descriptors = List.unmodifiable(descriptors);

  final String uuid;
  final Set<CharacteristicCapability> capabilities;
  final List<Descriptor> descriptors;

  @override
  String toString() => 'Characteristic($uuid)';
}

/// A GATT service of a peripheral, or one it advertises.
final class Service {
  Service(this.uuid, this.data, List<Characteristic> characteristics)
    : characteristics = List.unmodifiable(characteristics);

  final String uuid;

  /// Advertised service data.
  final Uint8List data;

  final List<Characteristic> characteristics;

  @override
  String toString() => 'Service($uuid)';
}

/// Converts a SimpleCBLE service, which keeps its own memory.
Service serviceOf(simpleble_service_t service) => Service(
  uuidOf(service.uuid),
  copyBytes(service.data, service.data_length),
  [for (var i = 0; i < service.characteristic_count; i++) _characteristicOf(service.characteristics[i])],
);

Characteristic _characteristicOf(simpleble_characteristic_t characteristic) => Characteristic(
  uuidOf(characteristic.uuid),
  {
    if (characteristic.can_read) CharacteristicCapability.read,
    if (characteristic.can_write_request) CharacteristicCapability.writeRequest,
    if (characteristic.can_write_command) CharacteristicCapability.writeCommand,
    if (characteristic.can_notify) CharacteristicCapability.notify,
    if (characteristic.can_indicate) CharacteristicCapability.indicate,
  },
  [for (var i = 0; i < characteristic.descriptor_count; i++) Descriptor(uuidOf(characteristic.descriptors[i].uuid))],
);

/// SimpleCBLE's capability flags for [capabilities].
int capabilityFlags(Set<CharacteristicCapability> capabilities) => capabilities.fold(
  0,
  (flags, capability) =>
      flags |
      switch (capability) {
        CharacteristicCapability.read =>
          simpleble_local_characteristic_capability_t.SIMPLEBLE_LOCAL_CHARACTERISTIC_READ,
        CharacteristicCapability.writeRequest =>
          simpleble_local_characteristic_capability_t.SIMPLEBLE_LOCAL_CHARACTERISTIC_WRITE_REQUEST,
        CharacteristicCapability.writeCommand =>
          simpleble_local_characteristic_capability_t.SIMPLEBLE_LOCAL_CHARACTERISTIC_WRITE_COMMAND,
        CharacteristicCapability.notify =>
          simpleble_local_characteristic_capability_t.SIMPLEBLE_LOCAL_CHARACTERISTIC_NOTIFY,
        CharacteristicCapability.indicate =>
          simpleble_local_characteristic_capability_t.SIMPLEBLE_LOCAL_CHARACTERISTIC_INDICATE,
      }.value,
);

/// The capabilities in SimpleCBLE's capability [flags].
Set<CharacteristicCapability> capabilitiesOf(int flags) => {
  for (final capability in CharacteristicCapability.values)
    if (flags & capabilityFlags({capability}) != 0) capability,
};
