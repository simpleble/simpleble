#include <pybind11/pybind11.h>

#include "simpleble/Exceptions.h"

namespace py = pybind11;

using SimpleBLE::ErrorCode;
namespace Exception = SimpleBLE::Exception;

constexpr auto kDocsErrorCode = R"pbdoc(
    Classification of a SimpleBLE error
)pbdoc";

namespace {

// Each SimpleBLE exception maps to one code, so the code is kept on its Python type.
template <typename CppException>
py::exception<CppException>& register_error(py::module& m, const char* name, py::handle base, ErrorCode code,
                                            const char* doc) {
    auto& type = py::register_exception<CppException>(m, name, base);
    type.attr("code") = code;
    type.doc() = doc;
    return type;
}

// Set once at import. pybind11 keeps the types for the life of the process.
py::handle winrt_error;
py::handle winrt_access_denied_error;

}  // namespace

void wrap_errors(py::module& m) {
    py::enum_<ErrorCode>(m, "ErrorCode", kDocsErrorCode)
        .value("INVALID_ARGUMENT", ErrorCode::INVALID_ARGUMENT)
        .value("OUT_OF_MEMORY", ErrorCode::OUT_OF_MEMORY)
        .value("OBJECT_NOT_INITIALIZED", ErrorCode::OBJECT_NOT_INITIALIZED)
        .value("INVALID_BACKEND_REFERENCE", ErrorCode::INVALID_BACKEND_REFERENCE)
        .value("PERIPHERAL_NOT_CONNECTED", ErrorCode::PERIPHERAL_NOT_CONNECTED)
        .value("GATT_SERVICE_NOT_FOUND", ErrorCode::GATT_SERVICE_NOT_FOUND)
        .value("GATT_CHARACTERISTIC_NOT_FOUND", ErrorCode::GATT_CHARACTERISTIC_NOT_FOUND)
        .value("GATT_DESCRIPTOR_NOT_FOUND", ErrorCode::GATT_DESCRIPTOR_NOT_FOUND)
        .value("OPERATION_NOT_SUPPORTED", ErrorCode::OPERATION_NOT_SUPPORTED)
        .value("OPERATION_FAILED", ErrorCode::OPERATION_FAILED)
        .value("WINRT_ACCESS_DENIED", ErrorCode::WINRT_ACCESS_DENIED)
        .value("WINRT_EXCEPTION", ErrorCode::WINRT_EXCEPTION)
        .value("CORE_BLUETOOTH_EXCEPTION", ErrorCode::CORE_BLUETOOTH_EXCEPTION)
        .value("UNCLASSIFIED_EXCEPTION", ErrorCode::UNCLASSIFIED_EXCEPTION)
        .value("ADAPTER_IN_USE", ErrorCode::ADAPTER_IN_USE);

    // NOTE: pybind11 tries the most recently registered translator first, so each type is registered after its base.
    auto& base = register_error<Exception::BaseException>(
        m, "SimpleBLEError", PyExc_RuntimeError, ErrorCode::UNCLASSIFIED_EXCEPTION,
        "Raised when a SimpleBLE operation fails. code holds the error code, and native_code the HRESULT of a WinRT "
        "failure.");
    base.attr("native_code") = py::none();

    register_error<Exception::NotInitialized>(m, "NotInitializedError", base, ErrorCode::OBJECT_NOT_INITIALIZED,
                                              "The object was not initialized.");
    register_error<Exception::InvalidReference>(m, "InvalidReferenceError", base, ErrorCode::INVALID_BACKEND_REFERENCE,
                                                "The object's backend object is no longer valid.");
    register_error<Exception::NotConnected>(m, "NotConnectedError", base, ErrorCode::PERIPHERAL_NOT_CONNECTED,
                                            "The operation requires a connected peripheral.");
    register_error<Exception::ServiceNotFound>(m, "ServiceNotFoundError", base, ErrorCode::GATT_SERVICE_NOT_FOUND,
                                               "The peripheral has no service with that UUID.");
    register_error<Exception::CharacteristicNotFound>(m, "CharacteristicNotFoundError", base,
                                                      ErrorCode::GATT_CHARACTERISTIC_NOT_FOUND,
                                                      "The service has no characteristic with that UUID.");
    register_error<Exception::DescriptorNotFound>(m, "DescriptorNotFoundError", base,
                                                  ErrorCode::GATT_DESCRIPTOR_NOT_FOUND,
                                                  "The characteristic has no descriptor with that UUID.");
    register_error<Exception::OperationNotSupported>(
        m, "OperationNotSupportedError", base, ErrorCode::OPERATION_NOT_SUPPORTED,
        "The adapter, characteristic or platform doesn't support the operation.");
    register_error<Exception::OperationFailed>(m, "OperationFailedError", base, ErrorCode::OPERATION_FAILED,
                                               "The operation failed. The message has any available detail.");
    winrt_error = register_error<Exception::WinRTException>(m, "WinRTError", base, ErrorCode::WINRT_EXCEPTION,
                                                            "A WinRT call failed.");
    winrt_access_denied_error = register_error<Exception::WinRTAccessDenied>(
        m, "WinRTAccessDeniedError", winrt_error, ErrorCode::WINRT_ACCESS_DENIED, "Windows denied access to the operation.");
    register_error<Exception::CoreBluetoothException>(m, "CoreBluetoothError", base,
                                                      ErrorCode::CORE_BLUETOOTH_EXCEPTION, "A CoreBluetooth call failed.");
    register_error<Exception::AdapterInUse>(m, "AdapterInUseError", base, ErrorCode::ADAPTER_IN_USE,
                                            "Another process holds the adapter.");
    register_error<Exception::InvalidArgument>(m, "InvalidArgumentError", py::make_tuple(base, py::handle(PyExc_ValueError)),
                                               ErrorCode::INVALID_ARGUMENT,
                                               "An argument is out of range. It is a ValueError too.");

    // WinRT errors also carry their HRESULT.
    py::register_exception_translator([](std::exception_ptr exception) {
        try {
            if (exception) std::rethrow_exception(exception);
        } catch (const Exception::WinRTException& e) {
            bool access_denied = dynamic_cast<const Exception::WinRTAccessDenied*>(&e) != nullptr;
            py::handle type = access_denied ? winrt_access_denied_error : winrt_error;
            py::object error = type(e.what());
            error.attr("native_code") = e.code();
            py::set_error(type, error);
        }
    });
}
