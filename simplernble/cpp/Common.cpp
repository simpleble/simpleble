#include "Common.hpp"

#include <new>
#include <stdexcept>
#include <string>

namespace margelo::nitro::simplernble {

namespace {

const char* code_name(SimpleBLE::ErrorCode code) {
    switch (code) {
        case SimpleBLE::ErrorCode::INVALID_ARGUMENT:
            return "INVALID_ARGUMENT";
        case SimpleBLE::ErrorCode::OUT_OF_MEMORY:
            return "OUT_OF_MEMORY";
        case SimpleBLE::ErrorCode::OBJECT_NOT_INITIALIZED:
            return "OBJECT_NOT_INITIALIZED";
        case SimpleBLE::ErrorCode::INVALID_BACKEND_REFERENCE:
            return "INVALID_BACKEND_REFERENCE";
        case SimpleBLE::ErrorCode::PERIPHERAL_NOT_CONNECTED:
            return "PERIPHERAL_NOT_CONNECTED";
        case SimpleBLE::ErrorCode::GATT_SERVICE_NOT_FOUND:
            return "GATT_SERVICE_NOT_FOUND";
        case SimpleBLE::ErrorCode::GATT_CHARACTERISTIC_NOT_FOUND:
            return "GATT_CHARACTERISTIC_NOT_FOUND";
        case SimpleBLE::ErrorCode::GATT_DESCRIPTOR_NOT_FOUND:
            return "GATT_DESCRIPTOR_NOT_FOUND";
        case SimpleBLE::ErrorCode::OPERATION_NOT_SUPPORTED:
            return "OPERATION_NOT_SUPPORTED";
        case SimpleBLE::ErrorCode::OPERATION_FAILED:
            return "OPERATION_FAILED";
        case SimpleBLE::ErrorCode::WINRT_ACCESS_DENIED:
            return "WINRT_ACCESS_DENIED";
        case SimpleBLE::ErrorCode::WINRT_EXCEPTION:
            return "WINRT_EXCEPTION";
        case SimpleBLE::ErrorCode::CORE_BLUETOOTH_EXCEPTION:
            return "CORE_BLUETOOTH_EXCEPTION";
        case SimpleBLE::ErrorCode::UNCLASSIFIED_EXCEPTION:
            return "UNCLASSIFIED_EXCEPTION";
        case SimpleBLE::ErrorCode::ADAPTER_IN_USE:
            return "ADAPTER_IN_USE";
    }
    return "UNCLASSIFIED_EXCEPTION";
}

std::exception_ptr make(const char* code, const std::string& message) noexcept {
    try {
        return std::make_exception_ptr(std::runtime_error(std::string("[") + code + "] " + message));
    } catch (...) {
        return std::current_exception();
    }
}

}  // namespace

std::exception_ptr to_error(std::exception_ptr exception) noexcept {
    try {
        std::rethrow_exception(exception);
    } catch (const SimpleBLE::Exception::BaseException& e) {
        try {
            return make(code_name(e.make_error()->code), e.what());
        } catch (...) {
            return make("OUT_OF_MEMORY", e.what());
        }
    } catch (const std::bad_alloc& e) {
        return make("OUT_OF_MEMORY", e.what());
    } catch (const std::exception& e) {
        return make("UNCLASSIFIED_EXCEPTION", e.what());
    } catch (...) {
        return make("UNCLASSIFIED_EXCEPTION", "Unknown native exception");
    }
}

const std::shared_ptr<Executor::Strand>& lifecycle() {
    static const auto strand = std::make_shared<Executor::Strand>();
    return strand;
}

}  // namespace margelo::nitro::simplernble
