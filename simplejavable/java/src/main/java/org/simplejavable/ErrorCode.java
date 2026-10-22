package org.simplejavable;

/**
 * SimpleBLE's classification of a failure, as in its C++ {@code SimpleBLE::ErrorCode}.
 */
public enum ErrorCode {
    /** A required argument is missing or otherwise invalid. */
    INVALID_ARGUMENT,
    /** Memory allocation failed during an operation. */
    OUT_OF_MEMORY,
    /** A SimpleBLE object has not been initialized. */
    OBJECT_NOT_INITIALIZED,
    /** An internal object reference cannot be resolved. */
    INVALID_BACKEND_REFERENCE,
    /** A GATT read, write, subscription, or unsubscription requires a connected peripheral. */
    PERIPHERAL_NOT_CONNECTED,
    /** The requested service UUID is absent from the discovered GATT services. */
    GATT_SERVICE_NOT_FOUND,
    /** The requested characteristic UUID is absent from the selected GATT service. */
    GATT_CHARACTERISTIC_NOT_FOUND,
    /** The requested descriptor UUID is absent from the selected GATT characteristic. */
    GATT_DESCRIPTOR_NOT_FOUND,
    /** The adapter, the characteristic, or this platform does not support the operation. */
    OPERATION_NOT_SUPPORTED,
    /** An operation failed without a more specific classification. The message has any available detail. */
    OPERATION_FAILED,
    /** Windows denied access to the operation. */
    WINRT_ACCESS_DENIED,
    /** Windows reported another failure. */
    WINRT_EXCEPTION,
    /** CoreBluetooth reported a failure. */
    CORE_BLUETOOTH_EXCEPTION,
    /** A failure without a dedicated classification. */
    UNCLASSIFIED_EXCEPTION,
    /** Another process holds the adapter, such as a Dongl whose serial port another program has open. */
    ADAPTER_IN_USE
}
