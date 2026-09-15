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
    /** @brief A required argument is NULL or otherwise invalid. */
    INVALID_ARGUMENT = 0,

    /** @brief Memory allocation failed during an operation. */
    OUT_OF_MEMORY = 1,

    /** @brief A SimpleBLE object has not been initialized. */
    OBJECT_NOT_INITIALIZED = 2,

    /** @brief An internal object reference cannot be resolved. */
    INVALID_BACKEND_REFERENCE = 3,

    /** @brief A GATT read, write, subscription, or unsubscription requires a connected peripheral. */
    PERIPHERAL_NOT_CONNECTED = 4,

    /** @brief The requested service UUID is absent from the discovered GATT services. */
    GATT_SERVICE_NOT_FOUND = 5,

    /** @brief The requested characteristic UUID is absent from the selected GATT service. */
    GATT_CHARACTERISTIC_NOT_FOUND = 6,

    /** @brief The requested descriptor UUID is absent from the selected GATT characteristic. */
    GATT_DESCRIPTOR_NOT_FOUND = 7,

    /** @brief The requested operation is not supported by the adapter or characteristic,
     * or is unavailable in the library implementation. */
    OPERATION_NOT_SUPPORTED = 8,

    /** @brief An operation failed without a more specific exception classification.
     * Includes connection and GATT failures, rejected local-peripheral configuration,
     * and hosting failures. The exception message supplies any available detail. */
    OPERATION_FAILED = 9,

    /** @brief Retrieving a WinRT asynchronous result raised hresult_access_denied.
     * The exception retains the original signed 32-bit HRESULT and message. */
    WINRT_ACCESS_DENIED = 10,

    /** @brief Retrieving a WinRT asynchronous result raised another hresult_error.
     * The exception retains the original signed 32-bit HRESULT and message. */
    WINRT_EXCEPTION = 11,

    /** @brief A CoreBluetoothException containing a diagnostic message. */
    CORE_BLUETOOTH_EXCEPTION = 12,

    /** @brief An exception has no dedicated error classification.
     * Covers BaseException, other standard exceptions, and non-standard exceptions;
     * the message is preserved when available. */
    UNCLASSIFIED_EXCEPTION = 13,
};

}  // namespace SimpleBLE

/**
 * @brief Owns an exception copy and its error classification.
 */
struct SIMPLEBLE_EXPORT simpleble_error final {
    simpleble_error(SimpleBLE::ErrorCode code, std::unique_ptr<std::exception> exception);
    simpleble_error(SimpleBLE::ErrorCode code, const char* message);

    SimpleBLE::ErrorCode code;
    std::unique_ptr<std::exception> exception;
};

namespace SimpleBLE {

using Error = ::simpleble_error;

}  // namespace SimpleBLE
