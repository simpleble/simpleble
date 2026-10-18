#include <simplektble/advanced.h>

#include <simplecble/error.h>
#include <simpleble/Advanced.h>
#include <simpleble/SimpleBLE.h>

#include <optional>
#include <string>

// NOTE: A SimpleCBLE handle points to the public SimpleBLE object it wraps.

void simplektble_local_peripheral_set_advertisement_local_name(simpleble_local_peripheral_t handle,
                                                               const char* local_name, simpleble_error_t** out_error) {
    simpleble_error_release(out_error);

    if (handle == nullptr) {
        *out_error = new SimpleBLE::Error(SimpleBLE::ErrorCode::INVALID_ARGUMENT, "handle is NULL");
        return;
    }

    try {
        auto& peripheral = *static_cast<SimpleBLE::Local::Peripheral*>(handle);
        auto name = local_name != nullptr ? std::make_optional<std::string>(local_name) : std::nullopt;
#if SIMPLEBLE_BACKEND_MACOS
        SimpleBLE::Advanced::MacOS::set_advertisement_local_name(peripheral, std::move(name));
#elif SIMPLEBLE_BACKEND_IOS
        SimpleBLE::Advanced::iOS::set_advertisement_local_name(peripheral, std::move(name));
#else
        (void)peripheral;
        (void)name;
        throw SimpleBLE::Exception::OperationNotSupported();
#endif
    } catch (const SimpleBLE::Exception::BaseException& e) {
        *out_error = e.make_error().release();
    } catch (const std::exception& e) {
        *out_error = new SimpleBLE::Error(SimpleBLE::ErrorCode::UNCLASSIFIED_EXCEPTION, e.what());
    } catch (...) {
        *out_error = new SimpleBLE::Error(SimpleBLE::ErrorCode::UNCLASSIFIED_EXCEPTION, "Unknown exception");
    }
}
