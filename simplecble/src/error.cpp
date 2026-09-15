#include <simpleble/Error.h>
#include <simplecble/error.h>

// Keep the public C codes aligned with the C++ error codes.
// clang-format off
static_assert(static_cast<int32_t>(SimpleBLE::ErrorCode::INVALID_ARGUMENT) == SIMPLEBLE_ERROR_INVALID_ARGUMENT);
static_assert(static_cast<int32_t>(SimpleBLE::ErrorCode::OUT_OF_MEMORY) == SIMPLEBLE_ERROR_OUT_OF_MEMORY);
static_assert(static_cast<int32_t>(SimpleBLE::ErrorCode::OBJECT_NOT_INITIALIZED) == SIMPLEBLE_ERROR_OBJECT_NOT_INITIALIZED);
static_assert(static_cast<int32_t>(SimpleBLE::ErrorCode::INVALID_BACKEND_REFERENCE) == SIMPLEBLE_ERROR_INVALID_BACKEND_REFERENCE);
static_assert(static_cast<int32_t>(SimpleBLE::ErrorCode::PERIPHERAL_NOT_CONNECTED) == SIMPLEBLE_ERROR_PERIPHERAL_NOT_CONNECTED);
static_assert(static_cast<int32_t>(SimpleBLE::ErrorCode::GATT_SERVICE_NOT_FOUND) == SIMPLEBLE_ERROR_GATT_SERVICE_NOT_FOUND);
static_assert(static_cast<int32_t>(SimpleBLE::ErrorCode::GATT_CHARACTERISTIC_NOT_FOUND) == SIMPLEBLE_ERROR_GATT_CHARACTERISTIC_NOT_FOUND);
static_assert(static_cast<int32_t>(SimpleBLE::ErrorCode::GATT_DESCRIPTOR_NOT_FOUND) == SIMPLEBLE_ERROR_GATT_DESCRIPTOR_NOT_FOUND);
static_assert(static_cast<int32_t>(SimpleBLE::ErrorCode::OPERATION_NOT_SUPPORTED) == SIMPLEBLE_ERROR_OPERATION_NOT_SUPPORTED);
static_assert(static_cast<int32_t>(SimpleBLE::ErrorCode::OPERATION_FAILED) == SIMPLEBLE_ERROR_OPERATION_FAILED);
static_assert(static_cast<int32_t>(SimpleBLE::ErrorCode::WINRT_ACCESS_DENIED) == SIMPLEBLE_ERROR_WINRT_ACCESS_DENIED);
static_assert(static_cast<int32_t>(SimpleBLE::ErrorCode::WINRT_EXCEPTION) == SIMPLEBLE_ERROR_WINRT_EXCEPTION);
static_assert(static_cast<int32_t>(SimpleBLE::ErrorCode::CORE_BLUETOOTH_EXCEPTION) == SIMPLEBLE_ERROR_CORE_BLUETOOTH_EXCEPTION);
static_assert(static_cast<int32_t>(SimpleBLE::ErrorCode::UNCLASSIFIED_EXCEPTION) == SIMPLEBLE_ERROR_UNCLASSIFIED_EXCEPTION);
// clang-format on

simpleble_err_t simpleble_error_code(const simpleble_error_t* error) {
    return static_cast<simpleble_err_t>(error->code);
}

const char* simpleble_error_message(const simpleble_error_t* error) {
    return error->exception->what();
}

void simpleble_error_release(simpleble_error_t** error) {
    delete *error;
    *error = nullptr;
}
