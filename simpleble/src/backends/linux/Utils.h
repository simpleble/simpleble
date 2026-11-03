#pragma once

#include <exception>
#include <string>

#include <simpleble/Exceptions.h>
#include <simpledbus/base/Exceptions.h>

namespace SimpleBLE {

/**
 * @brief Runs a call into BlueZ and rethrows its failures as SimpleBLE exceptions.
 *
 * BlueZ's NotSupported error becomes OperationNotSupported. Every other failure, including the D-Bus
 * reply timeout, becomes OperationFailed with BlueZ's own description of it.
 *
 * NOTE: BlueZ's NotPermitted covers runtime conditions, such as the device rejecting the request or
 * the device not being paired, so it becomes OperationFailed as well.
 */
template <typename Func>
auto bluez_call(Func&& func) -> decltype(func()) {
    try {
        return func();
    } catch (const Exception::BaseException&) {
        throw;
    } catch (const SimpleDBus::Exception::SendFailed& e) {
        if (e.name() == "org.bluez.Error.NotSupported") {
            throw Exception::OperationNotSupported();
        }
        throw Exception::OperationFailed(e.message());
    } catch (const std::exception& e) {
        throw Exception::OperationFailed(e.what());
    }
}

}  // namespace SimpleBLE
