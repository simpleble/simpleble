#include <simplecble/advanced.h>
#include <simplecble/error.h>

#include <simpleble/Advanced.h>
#include <simpleble/Exceptions.h>

#include <optional>
#include <string>

using SimpleBLE::Error;
using SimpleBLE::ErrorCode;

#if SIMPLEBLE_BACKEND_MACOS

void simpleble_advanced_macos_set_advertisement_local_name(simpleble_local_peripheral_t handle, const char* local_name,
                                                           simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    try {
        auto* peripheral = (SimpleBLE::Local::Peripheral*)handle;
        SimpleBLE::Advanced::MacOS::set_advertisement_local_name(
            *peripheral, local_name ? std::make_optional<std::string>(local_name) : std::nullopt);
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

simpleble_peripheral_t simpleble_advanced_macos_retrieve_cached_peripheral(simpleble_adapter_t handle,
                                                                           const char* identifier,
                                                                           simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return nullptr;
    }

    if (identifier == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "identifier is NULL");
        return nullptr;
    }

    try {
        auto* adapter = (SimpleBLE::Adapter*)handle;
        auto peripherals = SimpleBLE::Advanced::MacOS::retrieve_cached_peripherals(
            *adapter, {SimpleBLE::BluetoothAddress(identifier)});
        if (peripherals.empty()) return nullptr;
        return new SimpleBLE::Peripheral(peripherals.front());
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return nullptr;
}

#endif

#if SIMPLEBLE_BACKEND_IOS

void simpleble_advanced_ios_set_advertisement_local_name(simpleble_local_peripheral_t handle, const char* local_name,
                                                         simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    try {
        auto* peripheral = (SimpleBLE::Local::Peripheral*)handle;
        SimpleBLE::Advanced::iOS::set_advertisement_local_name(
            *peripheral, local_name ? std::make_optional<std::string>(local_name) : std::nullopt);
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

simpleble_peripheral_t simpleble_advanced_ios_retrieve_cached_peripheral(simpleble_adapter_t handle,
                                                                         const char* identifier,
                                                                         simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return nullptr;
    }

    if (identifier == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "identifier is NULL");
        return nullptr;
    }

    try {
        auto* adapter = (SimpleBLE::Adapter*)handle;
        auto peripherals = SimpleBLE::Advanced::iOS::retrieve_cached_peripherals(
            *adapter, {SimpleBLE::BluetoothAddress(identifier)});
        if (peripherals.empty()) return nullptr;
        return new SimpleBLE::Peripheral(peripherals.front());
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return nullptr;
}

#endif

#if SIMPLEBLE_BACKEND_LINUX

void simpleble_advanced_linux_set_advertisement_local_name(simpleble_local_peripheral_t handle, const char* local_name,
                                                           simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    try {
        auto* peripheral = (SimpleBLE::Local::Peripheral*)handle;
        SimpleBLE::Advanced::Linux::set_advertisement_local_name(
            *peripheral, local_name ? std::make_optional<std::string>(local_name) : std::nullopt);
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

#endif

#if SIMPLEBLE_BACKEND_ANDROID

JavaVM* simpleble_advanced_android_get_jvm(simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    try {
        return SimpleBLE::Advanced::Android::get_jvm();
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }

    return nullptr;
}

void simpleble_advanced_android_set_jvm(JavaVM* jvm, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (jvm == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "jvm is NULL");
        return;
    }

    try {
        SimpleBLE::Advanced::Android::set_jvm(jvm);
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

void simpleble_advanced_android_set_context(jobject context, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (context == nullptr) {
        *out_error = new Error(ErrorCode::INVALID_ARGUMENT, "context is NULL");
        return;
    }

    try {
        SimpleBLE::Advanced::Android::set_context(context);
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new Error(ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}

#endif
