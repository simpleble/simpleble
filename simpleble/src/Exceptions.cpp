#include <fmt/core.h>
#include <simpleble/Exceptions.h>
#include <utility>

using namespace SimpleBLE;
using namespace SimpleBLE::Exception;

NotInitialized::NotInitialized() : BaseException("Object has not been initialized.") {}

NotConnected::NotConnected() : BaseException("Peripheral is not connected.") {}

InvalidReference::InvalidReference() : BaseException("Underlying reference to object is invalid.") {}

ServiceNotFound::ServiceNotFound(BluetoothUUID uuid)
    : BaseException("Service with UUID " + uuid + " not found."), uuid_(std::move(uuid)) {}

CharacteristicNotFound::CharacteristicNotFound(BluetoothUUID uuid)
    : BaseException("Characteristic with UUID " + uuid + " not found"), uuid_(std::move(uuid)) {}

DescriptorNotFound::DescriptorNotFound(BluetoothUUID uuid)
    : BaseException("Descriptor with UUID " + uuid + " not found"), uuid_(std::move(uuid)) {}

OperationNotSupported::OperationNotSupported() : BaseException("The requested operation is not supported.") {}

OperationNotSupported::OperationNotSupported(const std::string& operation, const BluetoothUUID& characteristic_uuid)
    : BaseException(fmt::format("Operation '{}' is not supported on characteristic '{}'", operation, characteristic_uuid)) {}

OperationFailed::OperationFailed() : BaseException("The requested operation has failed.") {}

OperationFailed::OperationFailed(const std::string& err_msg) : BaseException("Operation Failed: " + err_msg) {}

WinRTException::WinRTException(int32_t err_code, const std::string& err_msg)
    : BaseException(fmt::format("WinRT Exception. Error code {}: {}", err_code, err_msg)), code_(err_code) {}

WinRTAccessDenied::WinRTAccessDenied(int32_t err_code, const std::string& err_msg)
    : WinRTException(err_code, err_msg) {}

CoreBluetoothException::CoreBluetoothException(const std::string& err_msg)
    : BaseException(fmt::format("CoreBluetooth Exception: {}", err_msg)) {}
