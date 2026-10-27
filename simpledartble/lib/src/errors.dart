/// SimpleBLE's classification of a failure, as in its C++ `SimpleBLE::ErrorCode`.
enum ErrorCode {
  /// A required argument is missing or otherwise invalid.
  invalidArgument,

  /// Memory allocation failed during an operation.
  outOfMemory,

  /// A SimpleBLE object has not been initialized, or was closed.
  objectNotInitialized,

  /// An internal object reference cannot be resolved.
  invalidBackendReference,

  /// A GATT read, write, subscription, or unsubscription requires a connected peripheral.
  peripheralNotConnected,

  /// The requested service UUID is absent from the discovered GATT services.
  gattServiceNotFound,

  /// The requested characteristic UUID is absent from the selected GATT service.
  gattCharacteristicNotFound,

  /// The requested descriptor UUID is absent from the selected GATT characteristic.
  gattDescriptorNotFound,

  /// The adapter, the characteristic, or this platform does not support the operation.
  operationNotSupported,

  /// An operation failed without a more specific classification. The message has any available detail.
  operationFailed,

  /// Windows denied access to the operation.
  winRTAccessDenied,

  /// Windows reported another failure.
  winRTException,

  /// CoreBluetooth reported a failure.
  coreBluetoothException,

  /// A failure without a dedicated classification.
  unclassifiedException,

  /// Another process holds the adapter, such as a Dongl whose serial port another program has open.
  adapterInUse,
}

/// A failure reported by SimpleBLE.
///
/// Each kind of failure has its own subclass, so it can be caught separately,
/// and [code] identifies it. Failures without a dedicated subclass are thrown
/// as this class.
class SimpleDartBleException implements Exception {
  /// SimpleBLE's classification of the failure.
  final ErrorCode code;

  /// What failed, as SimpleBLE describes it.
  final String message;

  SimpleDartBleException(this.code, this.message);

  /// The exception for a failure SimpleBLE reported with [code].
  factory SimpleDartBleException.of(ErrorCode code, String message) => switch (code) {
    ErrorCode.invalidArgument => InvalidArgumentException(message),
    ErrorCode.objectNotInitialized => NotInitializedException(message),
    ErrorCode.invalidBackendReference => InvalidReferenceException(message),
    ErrorCode.peripheralNotConnected => NotConnectedException(message),
    ErrorCode.gattServiceNotFound => ServiceNotFoundException(message),
    ErrorCode.gattCharacteristicNotFound => CharacteristicNotFoundException(message),
    ErrorCode.gattDescriptorNotFound => DescriptorNotFoundException(message),
    ErrorCode.operationNotSupported => OperationNotSupportedException(message),
    ErrorCode.operationFailed => OperationFailedException(message),
    ErrorCode.winRTAccessDenied => WinRTAccessDeniedException(message),
    ErrorCode.winRTException => WinRTException(message),
    ErrorCode.coreBluetoothException => CoreBluetoothException(message),
    ErrorCode.adapterInUse => AdapterInUseException(message),
    _ => SimpleDartBleException(code, message),
  };

  @override
  String toString() => '$runtimeType: $message';
}

/// A required argument is missing or otherwise invalid.
class InvalidArgumentException extends SimpleDartBleException {
  InvalidArgumentException(String message) : super(ErrorCode.invalidArgument, message);
}

/// The object has not been initialized, or was closed.
class NotInitializedException extends SimpleDartBleException {
  NotInitializedException(String message) : super(ErrorCode.objectNotInitialized, message);
}

/// An internal object reference cannot be resolved.
class InvalidReferenceException extends SimpleDartBleException {
  InvalidReferenceException(String message) : super(ErrorCode.invalidBackendReference, message);
}

/// The operation requires a connected peripheral.
class NotConnectedException extends SimpleDartBleException {
  NotConnectedException(String message) : super(ErrorCode.peripheralNotConnected, message);
}

/// The peripheral has no service with the requested UUID.
class ServiceNotFoundException extends SimpleDartBleException {
  ServiceNotFoundException(String message) : super(ErrorCode.gattServiceNotFound, message);
}

/// The service has no characteristic with the requested UUID.
class CharacteristicNotFoundException extends SimpleDartBleException {
  CharacteristicNotFoundException(String message) : super(ErrorCode.gattCharacteristicNotFound, message);
}

/// The characteristic has no descriptor with the requested UUID.
class DescriptorNotFoundException extends SimpleDartBleException {
  DescriptorNotFoundException(String message) : super(ErrorCode.gattDescriptorNotFound, message);
}

/// The adapter, the characteristic, or this platform does not support the operation.
class OperationNotSupportedException extends SimpleDartBleException {
  OperationNotSupportedException(String message) : super(ErrorCode.operationNotSupported, message);
}

/// The operation failed. The message has any available detail.
class OperationFailedException extends SimpleDartBleException {
  OperationFailedException(String message) : super(ErrorCode.operationFailed, message);
}

/// Windows reported a failure.
class WinRTException extends SimpleDartBleException {
  WinRTException(String message) : this._(ErrorCode.winRTException, message);

  WinRTException._(super.code, super.message);
}

/// Windows denied access to the operation.
class WinRTAccessDeniedException extends WinRTException {
  WinRTAccessDeniedException(String message) : super._(ErrorCode.winRTAccessDenied, message);
}

/// CoreBluetooth reported a failure.
class CoreBluetoothException extends SimpleDartBleException {
  CoreBluetoothException(String message) : super(ErrorCode.coreBluetoothException, message);
}

/// Another process holds the adapter, such as a Dongl whose serial port another program has open.
class AdapterInUseException extends SimpleDartBleException {
  AdapterInUseException(String message) : super(ErrorCode.adapterInUse, message);
}
