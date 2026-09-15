#include <simplecble/advanced.h>
#include <simplecble/error.h>

#include <simpleble/Advanced.h>
#include <simpleble/Exceptions.h>

#include <optional>
#include <string>

using SimpleBLE::Error;
using SimpleBLE::ErrorCode;

void simpleble_advanced_dongl_set_passkey_request_callback(simpleble_peripheral_t handle,
                                                           bool (*callback)(simpleble_peripheral_t handle,
                                                                            char passkey[7], void* userdata),
                                                           void* userdata, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    try {
        auto* peripheral = (SimpleBLE::Peripheral*)handle;
        if (callback == nullptr) {
            SimpleBLE::Advanced::Dongl::set_passkey_request_callback(*peripheral, nullptr);
        } else {
            SimpleBLE::Advanced::Dongl::set_passkey_request_callback(*peripheral, [=]() -> std::optional<std::string> {
                char passkey[7] = {};
                if (!callback(handle, passkey, userdata)) return std::nullopt;
                return std::string(passkey, 6);
            });
        }
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

void simpleble_advanced_dongl_set_passkey_display_callback(simpleble_peripheral_t handle,
                                                           void (*callback)(simpleble_peripheral_t handle,
                                                                            const char* passkey, void* userdata),
                                                           void* userdata, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    try {
        auto* peripheral = (SimpleBLE::Peripheral*)handle;
        if (callback == nullptr) {
            SimpleBLE::Advanced::Dongl::set_passkey_display_callback(*peripheral, nullptr);
        } else {
            SimpleBLE::Advanced::Dongl::set_passkey_display_callback(
                *peripheral, [=](const std::string& passkey) { callback(handle, passkey.c_str(), userdata); });
        }
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

void simpleble_advanced_dongl_set_numeric_comparison_callback(simpleble_peripheral_t handle,
                                                              bool (*callback)(simpleble_peripheral_t handle,
                                                                               const char* passkey, void* userdata),
                                                              void* userdata, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    try {
        auto* peripheral = (SimpleBLE::Peripheral*)handle;
        if (callback == nullptr) {
            SimpleBLE::Advanced::Dongl::set_numeric_comparison_callback(*peripheral, nullptr);
        } else {
            SimpleBLE::Advanced::Dongl::set_numeric_comparison_callback(
                *peripheral, [=](const std::string& passkey) { return callback(handle, passkey.c_str(), userdata); });
        }
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}
