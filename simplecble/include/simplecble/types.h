#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SIMPLEBLE_UUID_STR_LEN 37  // 36 characters + null terminator

/**
 * @brief Opaque error details for a failed call.
 *
 * Initialize the error variable to NULL and pass its address as out_error.
 * Each call releases the previous error before clearing the variable, then
 * leaves it NULL on success or stores an owned error on failure.
 * Inspect an error before the next call if its details are needed.
 * Release the final error by passing its address to simpleble_error_release(),
 * which also sets the variable to NULL.
 */
typedef struct simpleble_error simpleble_error_t;

/**
 * @brief Identifies the reported exception or binding error.
 *
 * A NULL error pointer indicates success; these codes describe failures only.
 */
typedef enum {
    /** @brief A required argument is NULL or otherwise invalid. */
    SIMPLEBLE_ERROR_INVALID_ARGUMENT = 0,

    /** @brief Memory allocation failed during an operation. */
    SIMPLEBLE_ERROR_OUT_OF_MEMORY = 1,

    /** @brief A SimpleBLE object has not been initialized. */
    SIMPLEBLE_ERROR_OBJECT_NOT_INITIALIZED = 2,

    /** @brief An internal object reference cannot be resolved. */
    SIMPLEBLE_ERROR_INVALID_BACKEND_REFERENCE = 3,

    /** @brief A GATT read, write, subscription, or unsubscription requires a connected peripheral. */
    SIMPLEBLE_ERROR_PERIPHERAL_NOT_CONNECTED = 4,

    /** @brief The requested service UUID is absent from the discovered GATT services. */
    SIMPLEBLE_ERROR_GATT_SERVICE_NOT_FOUND = 5,

    /** @brief The requested characteristic UUID is absent from the selected GATT service. */
    SIMPLEBLE_ERROR_GATT_CHARACTERISTIC_NOT_FOUND = 6,

    /** @brief The requested descriptor UUID is absent from the selected GATT characteristic. */
    SIMPLEBLE_ERROR_GATT_DESCRIPTOR_NOT_FOUND = 7,

    /** @brief The requested operation is not supported by the adapter or characteristic,
     * or is unavailable in the library implementation. */
    SIMPLEBLE_ERROR_OPERATION_NOT_SUPPORTED = 8,

    /** @brief An operation failed without a more specific exception classification.
     * Includes connection and GATT failures, rejected local-peripheral configuration,
     * and hosting failures. The exception message supplies any available detail. */
    SIMPLEBLE_ERROR_OPERATION_FAILED = 9,

    /** @brief Retrieving a WinRT asynchronous result raised hresult_access_denied.
     * The exception retains the original signed 32-bit HRESULT and message. */
    SIMPLEBLE_ERROR_WINRT_ACCESS_DENIED = 10,

    /** @brief Retrieving a WinRT asynchronous result raised another hresult_error.
     * The exception retains the original signed 32-bit HRESULT and message. */
    SIMPLEBLE_ERROR_WINRT_EXCEPTION = 11,

    /** @brief A CoreBluetoothException containing a diagnostic message. */
    SIMPLEBLE_ERROR_CORE_BLUETOOTH_EXCEPTION = 12,

    /** @brief An exception has no dedicated error classification.
     * Covers BaseException, other standard exceptions, and non-standard exceptions;
     * the message is preserved when available. */
    SIMPLEBLE_ERROR_UNCLASSIFIED_EXCEPTION = 13,
} simpleble_err_t;

typedef struct {
    char value[SIMPLEBLE_UUID_STR_LEN];
} simpleble_uuid_t;

typedef struct {
    simpleble_uuid_t uuid;
} simpleble_descriptor_t;

typedef struct {
    simpleble_uuid_t uuid;
    bool can_read;
    bool can_write_request;
    bool can_write_command;
    bool can_notify;
    bool can_indicate;
    size_t descriptor_count;
    simpleble_descriptor_t* descriptors;
} simpleble_characteristic_t;

/** @brief Owned service data. Release its contents with simpleble_service_release(). */
typedef struct {
    simpleble_uuid_t uuid;
    size_t data_length;
    uint8_t* data;
    size_t characteristic_count;
    simpleble_characteristic_t* characteristics;
} simpleble_service_t;

/** @brief Owned manufacturer data. Release its contents with simpleble_manufacturer_data_release(). */
typedef struct {
    uint16_t manufacturer_id;
    size_t data_length;
    uint8_t* data;
} simpleble_manufacturer_data_t;

typedef void* simpleble_backend_t;
typedef void* simpleble_adapter_t;
typedef void* simpleble_peripheral_t;
typedef void* simpleble_local_peripheral_t;
typedef void* simpleble_local_service_t;
typedef void* simpleble_local_characteristic_t;

/** @brief Local characteristic capability flags; combine with bitwise OR. */
typedef enum {
    SIMPLEBLE_LOCAL_CHARACTERISTIC_READ = 1u << 0,
    SIMPLEBLE_LOCAL_CHARACTERISTIC_WRITE_REQUEST = 1u << 1,
    SIMPLEBLE_LOCAL_CHARACTERISTIC_WRITE_COMMAND = 1u << 2,
    SIMPLEBLE_LOCAL_CHARACTERISTIC_NOTIFY = 1u << 3,
    SIMPLEBLE_LOCAL_CHARACTERISTIC_INDICATE = 1u << 4,
} simpleble_local_characteristic_capability_t;

typedef enum {
    SIMPLEBLE_OS_WINDOWS = 0,
    SIMPLEBLE_OS_MACOS = 1,
    SIMPLEBLE_OS_LINUX = 2,
    SIMPLEBLE_OS_IOS = 3,
    SIMPLEBLE_OS_ANDROID = 4,
    SIMPLEBLE_OS_UNKNOWN = 5,
} simpleble_os_t;

typedef enum {
    SIMPLEBLE_ADDRESS_TYPE_PUBLIC = 0,
    SIMPLEBLE_ADDRESS_TYPE_RANDOM = 1,
    SIMPLEBLE_ADDRESS_TYPE_UNSPECIFIED = 2,
} simpleble_address_type_t;
