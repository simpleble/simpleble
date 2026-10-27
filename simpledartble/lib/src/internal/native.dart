import 'dart:convert';
import 'dart:ffi';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

import '../errors.dart';
import 'bindings.g.dart';

// Helpers for calling SimpleCBLE. They work on any isolate, so the functions
// that run on workers use them too.

typedef ErrorVariable = Pointer<Pointer<simpleble_error>>;

/// Calls a SimpleCBLE function with an error variable and throws the error it stores.
T check<T>(T Function(ErrorVariable error) call) {
  final error = calloc<Pointer<simpleble_error>>();
  try {
    final result = call(error);
    if (error.value != nullptr) {
      final code = ErrorCode.values[simpleble_error_code(error.value).value];
      throw SimpleDartBleException.of(code, simpleble_error_message(error.value).cast<Utf8>().toDartString());
    }
    return result;
  } finally {
    simpleble_error_release(error);
    calloc.free(error);
  }
}

/// Throws unless SimpleBLE can reach the platform. In Android apps,
/// SimpleFltBLE loads the native library as the app starts.
void checkPlatform() {
  if (!simpledartble_platform_ready()) {
    throw StateError('Android apps use SimpleDartBLE through the simplefltble package');
  }
}

/// Converts a string SimpleCBLE allocated and frees it.
String takeString(Pointer<Char> value) {
  try {
    return value.cast<Utf8>().toDartString();
  } finally {
    simpleble_free(value.cast());
  }
}

/// Copies bytes SimpleCBLE returned or lent, which may be a null pointer when empty.
Uint8List copyBytes(Pointer<Uint8> data, int length) =>
    length == 0 ? Uint8List(0) : Uint8List.fromList(data.asTypedList(length));

/// Reads a SimpleCBLE list into the addresses of its owned handles, then releases the list.
List<int> readHandles(
  int Function(Pointer<Pointer<Void>> list, ErrorVariable error) read,
  Pointer<Void> Function(Pointer<Void> list, int index, ErrorVariable error) get,
  void Function(Pointer<Void> list) releaseList,
  void Function(Pointer<Void> handle) releaseHandle,
) {
  final list = calloc<Pointer<Void>>();
  final handles = <int>[];
  try {
    final count = check((error) => read(list, error));
    for (var i = 0; i < count; i++) {
      handles.add(check((error) => get(list.value, i, error)).address);
    }
    return handles;
  } catch (_) {
    for (final handle in handles) {
      releaseHandle(Pointer.fromAddress(handle));
    }
    rethrow;
  } finally {
    releaseList(list.value);
    calloc.free(list);
  }
}

// SIMPLEBLE_UUID_STR_LEN: a UUID string and its terminator.
const _uuidCapacity = 37;

/// Checks that [value] fits a SimpleCBLE UUID. UUIDs are passed on unchanged.
void checkUuid(String value, String name) {
  if (utf8.encode(value).length >= _uuidCapacity) {
    throw ArgumentError.value(value, name, 'A UUID has at most ${_uuidCapacity - 1} characters');
  }
}

/// A SimpleCBLE UUID for [value], allocated in [arena].
simpleble_uuid_t nativeUuid(String value, Arena arena) {
  checkUuid(value, 'uuid');
  final uuid = arena<simpleble_uuid_t>();
  final bytes = value.toNativeUtf8(allocator: arena);
  for (var i = 0; i < bytes.length; i++) {
    uuid.ref.value[i] = bytes.cast<Char>()[i];
  }
  uuid.ref.value[bytes.length] = 0;
  return uuid.ref;
}

/// The string of a SimpleCBLE UUID.
String uuidOf(simpleble_uuid_t uuid) {
  final bytes = <int>[];
  for (var i = 0; i < _uuidCapacity && uuid.value[i] != 0; i++) {
    bytes.add(uuid.value[i]);
  }
  return utf8.decode(bytes);
}

/// A copy of [value] in [arena]. SimpleCBLE takes a non-null pointer even for no bytes.
Pointer<Uint8> nativeBytes(Uint8List value, Arena arena) {
  final data = arena<Uint8>(value.isEmpty ? 1 : value.length);
  data.asTypedList(value.length).setAll(0, value);
  return data;
}

/// Takes bytes SimpleCBLE allocated: copies and frees them.
Uint8List takeBytes(Pointer<Uint8> data, int length) {
  try {
    return copyBytes(data, length);
  } finally {
    simpleble_free(data.cast());
  }
}
