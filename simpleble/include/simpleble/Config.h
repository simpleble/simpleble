#pragma once
#include <chrono>
#include <simpleble/export.h>

//clang-format off
namespace SimpleBLE {
/**
 * @namespace SimpleBLE::Config
 * @brief Configuration options for SimpleBLE.
 *
 * @note All configuration values must be set prior to any other interaction with a SimpleBLE component.
 *       Changes made after an adapter has been retrieved may not take effect or could lead to undefined behavior.
 */
namespace Config {
namespace SimpleBluez {
    extern SIMPLEBLE_EXPORT bool use_legacy_bluez_backend;
    extern SIMPLEBLE_EXPORT bool use_system_bus; // NOTE: This is only available in the new Bluez backend.
    extern SIMPLEBLE_EXPORT std::chrono::steady_clock::duration connection_timeout;
    extern SIMPLEBLE_EXPORT std::chrono::steady_clock::duration disconnection_timeout;

    static void reset() {
        use_legacy_bluez_backend = false;
        use_system_bus = true;
        connection_timeout = std::chrono::seconds(2);
        disconnection_timeout = std::chrono::seconds(1);
    }
}  // namespace SimpleBluez

namespace WinRT {
    /**
     * @deprecated SimpleBLE uses its own MTA apartment by default. This compatibility flag will be removed in a future release.
     */
    extern SIMPLEBLE_EXPORT bool experimental_use_own_mta_apartment;
    extern SIMPLEBLE_EXPORT bool experimental_reinitialize_winrt_apartment_on_main_thread;
    extern SIMPLEBLE_EXPORT bool use_deferred_disconnect;

    static void reset() {
        experimental_use_own_mta_apartment = true;
        experimental_reinitialize_winrt_apartment_on_main_thread = false;
        use_deferred_disconnect = true;
    }
}  // namespace WinRT

namespace CoreBluetooth {
    static void reset() {}
}  // namespace CoreBluetooth

namespace Android {
    enum class ConnectionPriorityRequest { DISABLED = -1, BALANCED = 0, HIGH = 1, LOW_POWER = 2, DCK = 3 };

    extern SIMPLEBLE_EXPORT ConnectionPriorityRequest connection_priority_request;

    // MTU to request on connection (0 = disabled, leave to Android's 23-byte default).
    // BLE 4.2+ negotiates up to 517; older peripherals support 23-185.
    extern SIMPLEBLE_EXPORT int mtu_request;

    static void reset() {
        connection_priority_request = ConnectionPriorityRequest::DISABLED;
        mtu_request = 0;
    }
}  // namespace Android

namespace Dongl {
    extern SIMPLEBLE_EXPORT bool use_dongl_backend;
    extern SIMPLEBLE_EXPORT bool auto_update;
    extern SIMPLEBLE_EXPORT bool force_update;
    static void reset() {
        use_dongl_backend = false;
        auto_update = false;
        force_update = false;
    }
}  // namespace Dongl

namespace Base {
    static void reset_all() {
        SimpleBluez::reset();
        WinRT::reset();
        CoreBluetooth::reset();
        Android::reset();
        Dongl::reset();
    }
}  // namespace Base
}  // namespace Config
}  // namespace SimpleBLE
//clang-format on
