#pragma once

#include <cstdint>
#include <exception>
#include <memory>

#include <simpleble/export.h>

namespace SimpleBLE {

/**
 * @brief Error codes shared with foreign-language bindings.
 *
 * Codes identify the reported exception or binding error.
 */
enum class ErrorCode : int32_t {
    /** @brief The stored exception has the wrong type or lacks the requested diagnostic data. */
    INVALID_TYPE = 0,

    /** @brief A required argument is NULL or otherwise invalid. */
    INVALID_ARGUMENT = 1,

    /** @brief Memory allocation failed, including allocation of error details. */
    OUT_OF_MEMORY = 2,

    /** @brief A SimpleBLE object has not been initialized. */
    OBJECT_NOT_INITIALIZED = 3,

    /** @brief An internal object reference cannot be resolved. */
    INVALID_BACKEND_REFERENCE = 4,

    /** @brief A GATT read, write, subscription, or unsubscription requires a connected peripheral. */
    PERIPHERAL_NOT_CONNECTED = 5,

    /** @brief The requested service UUID is absent from the discovered GATT services. */
    GATT_SERVICE_NOT_FOUND = 6,

    /** @brief The requested characteristic UUID is absent from the selected GATT service. */
    GATT_CHARACTERISTIC_NOT_FOUND = 7,

    /** @brief The requested descriptor UUID is absent from the selected GATT characteristic. */
    GATT_DESCRIPTOR_NOT_FOUND = 8,

    /** @brief The requested operation is not supported by the adapter or characteristic,
     * or is unavailable in the library implementation. */
    OPERATION_NOT_SUPPORTED = 9,

    /** @brief An operation failed without a more specific exception classification.
     * Includes connection and GATT failures, rejected local-peripheral configuration,
     * and hosting failures. The exception message supplies any available detail. */
    OPERATION_FAILED = 10,

    /** @brief Retrieving a WinRT asynchronous result raised hresult_access_denied.
     * The exception retains the original signed 32-bit HRESULT and message. */
    WINRT_ACCESS_DENIED = 11,

    /** @brief Retrieving a WinRT asynchronous result raised another hresult_error.
     * The exception retains the original signed 32-bit HRESULT and message. */
    WINRT_EXCEPTION = 12,

    /** @brief A CoreBluetoothException containing a diagnostic message. */
    CORE_BLUETOOTH_EXCEPTION = 13,

    /** @brief An exception has no dedicated error classification.
     * Covers BaseException, other standard exceptions, and non-standard exceptions;
     * the message is preserved when available. */
    UNCLASSIFIED_EXCEPTION = 14,
};

/**
 * @brief Owns an exception copy and its error classification.
 */
struct SIMPLEBLE_EXPORT Error final {
    Error(ErrorCode code, std::unique_ptr<std::exception> exception);

    ErrorCode code;
    std::unique_ptr<std::exception> exception;
};

}  // namespace SimpleBLE
