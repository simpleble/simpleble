#include <fmt/core.h>
#include <simpleble/Exceptions.h>
#include <utility>

using namespace SimpleBLE;
using namespace SimpleBLE::Exception;

std::unique_ptr<Error> BaseException::make_error() const {
    return std::make_unique<Error>(ErrorCode::UNCLASSIFIED_EXCEPTION, std::make_unique<BaseException>(*this));
}

NotInitialized::NotInitialized() : BaseException("Object has not been initialized.") {}

std::unique_ptr<Error> NotInitialized::make_error() const {
    return std::make_unique<Error>(ErrorCode::OBJECT_NOT_INITIALIZED, std::make_unique<NotInitialized>(*this));
}

NotConnected::NotConnected() : BaseException("Peripheral is not connected.") {}

std::unique_ptr<Error> NotConnected::make_error() const {
    return std::make_unique<Error>(ErrorCode::PERIPHERAL_NOT_CONNECTED, std::make_unique<NotConnected>(*this));
}

InvalidReference::InvalidReference() : BaseException("Underlying reference to object is invalid.") {}

std::unique_ptr<Error> InvalidReference::make_error() const {
    return std::make_unique<Error>(ErrorCode::INVALID_BACKEND_REFERENCE, std::make_unique<InvalidReference>(*this));
}

ServiceNotFound::ServiceNotFound(BluetoothUUID uuid)
    : BaseException("Service with UUID " + uuid + " not found."), uuid_(std::move(uuid)) {}

const BluetoothUUID& ServiceNotFound::uuid() const noexcept { return uuid_; }

std::unique_ptr<Error> ServiceNotFound::make_error() const {
    return std::make_unique<Error>(ErrorCode::GATT_SERVICE_NOT_FOUND, std::make_unique<ServiceNotFound>(*this));
}

CharacteristicNotFound::CharacteristicNotFound(BluetoothUUID uuid)
    : BaseException("Characteristic with UUID " + uuid + " not found"), uuid_(std::move(uuid)) {}

const BluetoothUUID& CharacteristicNotFound::uuid() const noexcept { return uuid_; }

std::unique_ptr<Error> CharacteristicNotFound::make_error() const {
    return std::make_unique<Error>(ErrorCode::GATT_CHARACTERISTIC_NOT_FOUND,
                                   std::make_unique<CharacteristicNotFound>(*this));
}

DescriptorNotFound::DescriptorNotFound(BluetoothUUID uuid)
    : BaseException("Descriptor with UUID " + uuid + " not found"), uuid_(std::move(uuid)) {}

const BluetoothUUID& DescriptorNotFound::uuid() const noexcept { return uuid_; }

std::unique_ptr<Error> DescriptorNotFound::make_error() const {
    return std::make_unique<Error>(ErrorCode::GATT_DESCRIPTOR_NOT_FOUND, std::make_unique<DescriptorNotFound>(*this));
}

OperationNotSupported::OperationNotSupported() : BaseException("The requested operation is not supported.") {}

OperationNotSupported::OperationNotSupported(const std::string& operation, const BluetoothUUID& characteristic_uuid)
    : BaseException(
          fmt::format("Operation '{}' is not supported on characteristic '{}'", operation, characteristic_uuid)) {}

std::unique_ptr<Error> OperationNotSupported::make_error() const {
    return std::make_unique<Error>(ErrorCode::OPERATION_NOT_SUPPORTED, std::make_unique<OperationNotSupported>(*this));
}

OperationFailed::OperationFailed() : BaseException("The requested operation has failed.") {}

OperationFailed::OperationFailed(const std::string& err_msg) : BaseException("Operation Failed: " + err_msg) {}

std::unique_ptr<Error> OperationFailed::make_error() const {
    return std::make_unique<Error>(ErrorCode::OPERATION_FAILED, std::make_unique<OperationFailed>(*this));
}

WinRTException::WinRTException(int32_t err_code, const std::string& err_msg)
    : BaseException(fmt::format("WinRT Exception. Error code {}: {}", err_code, err_msg)), code_(err_code) {}

int32_t WinRTException::code() const noexcept { return code_; }

std::unique_ptr<Error> WinRTException::make_error() const {
    return std::make_unique<Error>(ErrorCode::WINRT_EXCEPTION, std::make_unique<WinRTException>(*this));
}

WinRTAccessDenied::WinRTAccessDenied(int32_t err_code, const std::string& err_msg)
    : WinRTException(err_code, err_msg) {}

std::unique_ptr<Error> WinRTAccessDenied::make_error() const {
    return std::make_unique<Error>(ErrorCode::WINRT_ACCESS_DENIED, std::make_unique<WinRTAccessDenied>(*this));
}

CoreBluetoothException::CoreBluetoothException(const std::string& err_msg)
    : BaseException(fmt::format("CoreBluetooth Exception: {}", err_msg)) {}

std::unique_ptr<Error> CoreBluetoothException::make_error() const {
    return std::make_unique<Error>(ErrorCode::CORE_BLUETOOTH_EXCEPTION,
                                   std::make_unique<CoreBluetoothException>(*this));
}
