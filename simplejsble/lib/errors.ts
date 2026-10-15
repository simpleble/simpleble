/** SimpleBLE's error classification, as in the C++ `SimpleBLE::ErrorCode`. */
export type ErrorCode =
  | 'INVALID_ARGUMENT'
  | 'OUT_OF_MEMORY'
  | 'OBJECT_NOT_INITIALIZED'
  | 'INVALID_BACKEND_REFERENCE'
  | 'PERIPHERAL_NOT_CONNECTED'
  | 'GATT_SERVICE_NOT_FOUND'
  | 'GATT_CHARACTERISTIC_NOT_FOUND'
  | 'GATT_DESCRIPTOR_NOT_FOUND'
  | 'OPERATION_NOT_SUPPORTED'
  | 'OPERATION_FAILED'
  | 'WINRT_ACCESS_DENIED'
  | 'WINRT_EXCEPTION'
  | 'CORE_BLUETOOTH_EXCEPTION'
  | 'UNCLASSIFIED_EXCEPTION'
  | 'ADAPTER_IN_USE';

/** An error raised by SimpleBLE. */
export class SimpleBLEError extends Error {
  override readonly name = 'SimpleBLEError';
  readonly code: ErrorCode;
  /** The original HRESULT of a WinRT failure. */
  readonly nativeCode?: number;

  constructor(code: ErrorCode, message: string, nativeCode?: number) {
    super(message);
    this.code = code;
    if (nativeCode !== undefined) this.nativeCode = nativeCode;
  }
}
