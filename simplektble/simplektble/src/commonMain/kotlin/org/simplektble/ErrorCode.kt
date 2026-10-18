package org.simplektble

/** SimpleBLE's classification of a failure, as in its C++ `SimpleBLE::ErrorCode`. */
public enum class ErrorCode(internal val value: Int) {
    /** A required argument is missing or otherwise invalid. */
    INVALID_ARGUMENT(0),

    /** Memory allocation failed during an operation. */
    OUT_OF_MEMORY(1),

    /** A SimpleBLE object has not been initialized. */
    OBJECT_NOT_INITIALIZED(2),

    /** An internal object reference cannot be resolved. */
    INVALID_BACKEND_REFERENCE(3),

    /** A GATT read, write, subscription, or unsubscription requires a connected peripheral. */
    PERIPHERAL_NOT_CONNECTED(4),

    /** The requested service UUID is absent from the discovered GATT services. */
    GATT_SERVICE_NOT_FOUND(5),

    /** The requested characteristic UUID is absent from the selected GATT service. */
    GATT_CHARACTERISTIC_NOT_FOUND(6),

    /** The requested descriptor UUID is absent from the selected GATT characteristic. */
    GATT_DESCRIPTOR_NOT_FOUND(7),

    /** The adapter, the characteristic, or this platform does not support the operation. */
    OPERATION_NOT_SUPPORTED(8),

    /** An operation failed without a more specific classification. The message has any available detail. */
    OPERATION_FAILED(9),

    /** Windows denied access to the operation. */
    WINRT_ACCESS_DENIED(10),

    /** Windows reported another failure. */
    WINRT_EXCEPTION(11),

    /** CoreBluetooth reported a failure. */
    CORE_BLUETOOTH_EXCEPTION(12),

    /** A failure without a dedicated classification. */
    UNCLASSIFIED_EXCEPTION(13),

    /** Another process holds the adapter, such as a Dongl whose serial port another program has open. */
    ADAPTER_IN_USE(14);

    internal companion object {
        fun fromValue(value: Int): ErrorCode = entries.firstOrNull { it.value == value } ?: UNCLASSIFIED_EXCEPTION
    }
}
