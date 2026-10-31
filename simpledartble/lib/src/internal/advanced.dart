@DefaultAsset('package:simpledartble/simpledartble.dart')
library;

import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';

import '../errors.dart';
import 'bindings.g.dart';
import 'native.dart';

// SimpleCBLE's platform functions, which only the header of their own platform
// declares, so tool/ffigen.dart leaves them out.

typedef _Error = Pointer<Pointer<simpleble_error>>;

@Native<Void Function(Pointer<Void>, Pointer<Char>, _Error)>(
  symbol: 'simpleble_advanced_macos_set_advertisement_local_name',
)
external void _macosSetAdvertisementLocalName(Pointer<Void> handle, Pointer<Char> localName, _Error error);

@Native<Pointer<Void> Function(Pointer<Void>, Pointer<Char>, _Error)>(
  symbol: 'simpleble_advanced_macos_retrieve_cached_peripheral',
)
external Pointer<Void> _macosRetrieveCachedPeripheral(Pointer<Void> handle, Pointer<Char> identifier, _Error error);

@Native<Void Function(Pointer<Void>, Pointer<Char>, _Error)>(
  symbol: 'simpleble_advanced_ios_set_advertisement_local_name',
)
external void _iosSetAdvertisementLocalName(Pointer<Void> handle, Pointer<Char> localName, _Error error);

@Native<Pointer<Void> Function(Pointer<Void>, Pointer<Char>, _Error)>(
  symbol: 'simpleble_advanced_ios_retrieve_cached_peripheral',
)
external Pointer<Void> _iosRetrieveCachedPeripheral(Pointer<Void> handle, Pointer<Char> identifier, _Error error);

@Native<Void Function(Pointer<Void>, Pointer<Char>, _Error)>(
  symbol: 'simpleble_advanced_linux_set_advertisement_local_name',
)
external void _linuxSetAdvertisementLocalName(Pointer<Void> handle, Pointer<Char> localName, _Error error);

typedef _PasskeyRequest = NativeFunction<Bool Function(Pointer<Void>, Pointer<Char>, Pointer<Char>, Pointer<Void>)>;
typedef _PasskeyDisplay = NativeFunction<Void Function(Pointer<Void>, Pointer<Char>, Pointer<Char>, Pointer<Void>)>;
typedef _NumericComparison = NativeFunction<Bool Function(Pointer<Void>, Pointer<Char>, Pointer<Char>, Pointer<Void>)>;
typedef _PairingAuthorization = NativeFunction<Bool Function(Pointer<Void>, Pointer<Char>, Pointer<Void>)>;

@Native<Void Function(Pointer<Void>, Pointer<_PasskeyRequest>, Pointer<Void>, _Error)>(
  symbol: 'simpleble_advanced_linux_set_passkey_request_callback',
)
external void _linuxSetPasskeyRequestCallback(
  Pointer<Void> handle,
  Pointer<_PasskeyRequest> callback,
  Pointer<Void> userdata,
  _Error error,
);

@Native<Void Function(Pointer<Void>, Pointer<_PasskeyDisplay>, Pointer<Void>, _Error)>(
  symbol: 'simpleble_advanced_linux_set_passkey_display_callback',
)
external void _linuxSetPasskeyDisplayCallback(
  Pointer<Void> handle,
  Pointer<_PasskeyDisplay> callback,
  Pointer<Void> userdata,
  _Error error,
);

@Native<Void Function(Pointer<Void>, Pointer<_NumericComparison>, Pointer<Void>, _Error)>(
  symbol: 'simpleble_advanced_linux_set_numeric_comparison_callback',
)
external void _linuxSetNumericComparisonCallback(
  Pointer<Void> handle,
  Pointer<_NumericComparison> callback,
  Pointer<Void> userdata,
  _Error error,
);

@Native<Void Function(Pointer<Void>, Pointer<_PairingAuthorization>, Pointer<Void>, _Error)>(
  symbol: 'simpleble_advanced_linux_set_pairing_authorization_callback',
)
external void _linuxSetPairingAuthorizationCallback(
  Pointer<Void> handle,
  Pointer<_PairingAuthorization> callback,
  Pointer<Void> userdata,
  _Error error,
);

/// Sets the advertised local name of a local peripheral, or removes it for null.
void setAdvertisementLocalNameOf(Pointer<Void> host, String? name) => using((arena) {
  final localName = name == null ? nullptr : name.toNativeUtf8(allocator: arena).cast<Char>();
  final function = switch (Platform.operatingSystem) {
    'macos' => _macosSetAdvertisementLocalName,
    'ios' => _iosSetAdvertisementLocalName,
    'linux' => _linuxSetAdvertisementLocalName,
    _ => throw OperationNotSupportedException('Advertisement local names need iOS, macOS or Linux'),
  };
  check((error) => function(host, localName, error));
});

/// Retrieves the owned handles of the peripherals CoreBluetooth knows by [identifiers]. Runs on a worker.
List<int> readCachedPeripherals((int, List<String>) arguments) {
  final (adapter, identifiers) = arguments;
  final function = switch (Platform.operatingSystem) {
    'macos' => _macosRetrieveCachedPeripheral,
    'ios' => _iosRetrieveCachedPeripheral,
    _ => throw OperationNotSupportedException('Cached peripherals need iOS or macOS'),
  };
  final handles = <int>[];
  try {
    for (final identifier in identifiers) {
      final handle = using(
        (arena) => check(
          (error) => function(Pointer.fromAddress(adapter), identifier.toNativeUtf8(allocator: arena).cast(), error),
        ),
      );
      if (handle != nullptr) handles.add(handle.address);
    }
    return handles;
  } catch (_) {
    for (final handle in handles) {
      simpleble_peripheral_release_handle(Pointer.fromAddress(handle));
    }
    rethrow;
  }
}

// NOTE: The core holds a callback's lock while it runs, and a pairing question
// waits for the isolate that answers it, so the functions below run on workers.
// They take an adapter's handle address and a token, or zero to clear the
// callback. Adapters of other backends reject them with OPERATION_NOT_SUPPORTED.

void _requireLinux() {
  if (Platform.operatingSystem != 'linux') throw OperationNotSupportedException('Adapter pairing handlers need Linux');
}

void setLinuxPasskeyRequest((int, int) arguments) {
  final (adapter, token) = arguments;
  _requireLinux();
  check(
    (error) => _linuxSetPasskeyRequestCallback(
      Pointer.fromAddress(adapter),
      token == 0 ? nullptr : Native.addressOf(simpledartble_on_linux_passkey_request),
      Pointer.fromAddress(token),
      error,
    ),
  );
}

void setLinuxPasskeyDisplay((int, int) arguments) {
  final (adapter, token) = arguments;
  _requireLinux();
  check(
    (error) => _linuxSetPasskeyDisplayCallback(
      Pointer.fromAddress(adapter),
      token == 0 ? nullptr : Native.addressOf(simpledartble_on_linux_passkey_display),
      Pointer.fromAddress(token),
      error,
    ),
  );
}

void setLinuxNumericComparison((int, int) arguments) {
  final (adapter, token) = arguments;
  _requireLinux();
  check(
    (error) => _linuxSetNumericComparisonCallback(
      Pointer.fromAddress(adapter),
      token == 0 ? nullptr : Native.addressOf(simpledartble_on_linux_numeric_comparison),
      Pointer.fromAddress(token),
      error,
    ),
  );
}

void setLinuxPairingAuthorization((int, int) arguments) {
  final (adapter, token) = arguments;
  _requireLinux();
  check(
    (error) => _linuxSetPairingAuthorizationCallback(
      Pointer.fromAddress(adapter),
      token == 0 ? nullptr : Native.addressOf(simpledartble_on_linux_pairing_authorization),
      Pointer.fromAddress(token),
      error,
    ),
  );
}
