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

  constructor(code: ErrorCode, message: string) {
    super(message);
    this.code = code;
  }
}

// The native side reports SimpleBLE's errors as "[CODE] message".
const CODED = /\[([A-Z_]+)\] ([\s\S]*)$/;

function translate(error: unknown): unknown {
  const match = error instanceof Error ? CODED.exec(error.message) : null;
  return match ? new SimpleBLEError(match[1] as ErrorCode, match[2]) : error;
}

/** Calls `fn`, turning a native error into a SimpleBLEError. @internal */
export function sync<T>(fn: () => T): T {
  try {
    return fn();
  } catch (error) {
    throw translate(error);
  }
}

/** Awaits `fn()`, turning a native error into a SimpleBLEError. @internal */
export async function async<T>(fn: () => Promise<T>): Promise<T> {
  try {
    return await fn();
  } catch (error) {
    throw translate(error);
  }
}
