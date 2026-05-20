#include <simplecble/config.h>

#include <simpleble/Config.h>

#include <chrono>

void simpleble_config_reset_all(void) { SimpleBLE::Config::Base::reset_all(); }

void simpleble_config_simplebluez_reset(void) { SimpleBLE::Config::SimpleBluez::reset(); }

bool simpleble_config_simplebluez_get_use_legacy_bluez_backend(void) {
    return SimpleBLE::Config::SimpleBluez::use_legacy_bluez_backend;
}

void simpleble_config_simplebluez_set_use_legacy_bluez_backend(bool enabled) {
    SimpleBLE::Config::SimpleBluez::use_legacy_bluez_backend = enabled;
}

bool simpleble_config_simplebluez_get_use_system_bus(void) {
    return SimpleBLE::Config::SimpleBluez::use_system_bus;
}

void simpleble_config_simplebluez_set_use_system_bus(bool enabled) {
    SimpleBLE::Config::SimpleBluez::use_system_bus = enabled;
}

int64_t simpleble_config_simplebluez_get_connection_timeout_ms(void) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               SimpleBLE::Config::SimpleBluez::connection_timeout)
        .count();
}

void simpleble_config_simplebluez_set_connection_timeout_ms(int64_t timeout_ms) {
    SimpleBLE::Config::SimpleBluez::connection_timeout = std::chrono::milliseconds(timeout_ms);
}

int64_t simpleble_config_simplebluez_get_disconnection_timeout_ms(void) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               SimpleBLE::Config::SimpleBluez::disconnection_timeout)
        .count();
}

void simpleble_config_simplebluez_set_disconnection_timeout_ms(int64_t timeout_ms) {
    SimpleBLE::Config::SimpleBluez::disconnection_timeout = std::chrono::milliseconds(timeout_ms);
}

void simpleble_config_winrt_reset(void) { SimpleBLE::Config::WinRT::reset(); }

bool simpleble_config_winrt_get_experimental_use_own_mta_apartment(void) {
    return SimpleBLE::Config::WinRT::experimental_use_own_mta_apartment;
}

void simpleble_config_winrt_set_experimental_use_own_mta_apartment(bool enabled) {
    SimpleBLE::Config::WinRT::experimental_use_own_mta_apartment = enabled;
}

bool simpleble_config_winrt_get_experimental_reinitialize_winrt_apartment_on_main_thread(void) {
    return SimpleBLE::Config::WinRT::experimental_reinitialize_winrt_apartment_on_main_thread;
}

void simpleble_config_winrt_set_experimental_reinitialize_winrt_apartment_on_main_thread(bool enabled) {
    SimpleBLE::Config::WinRT::experimental_reinitialize_winrt_apartment_on_main_thread = enabled;
}

bool simpleble_config_winrt_get_use_deferred_disconnect(void) {
    return SimpleBLE::Config::WinRT::use_deferred_disconnect;
}

void simpleble_config_winrt_set_use_deferred_disconnect(bool enabled) {
    SimpleBLE::Config::WinRT::use_deferred_disconnect = enabled;
}

void simpleble_config_corebluetooth_reset(void) { SimpleBLE::Config::CoreBluetooth::reset(); }

void simpleble_config_android_reset(void) { SimpleBLE::Config::Android::reset(); }

simpleble_config_android_connection_priority_t simpleble_config_android_get_connection_priority(void) {
    return static_cast<simpleble_config_android_connection_priority_t>(
        SimpleBLE::Config::Android::connection_priority_request);
}

void simpleble_config_android_set_connection_priority(simpleble_config_android_connection_priority_t priority) {
    SimpleBLE::Config::Android::connection_priority_request =
        static_cast<SimpleBLE::Config::Android::ConnectionPriorityRequest>(priority);
}

void simpleble_config_set_android_connection_priority(int priority) {
    simpleble_config_android_set_connection_priority(
        static_cast<simpleble_config_android_connection_priority_t>(priority));
}

void simpleble_config_dongl_reset(void) { SimpleBLE::Config::Dongl::reset(); }

bool simpleble_config_dongl_get_use_dongl_backend(void) { return SimpleBLE::Config::Dongl::use_dongl_backend; }

void simpleble_config_dongl_set_use_dongl_backend(bool enabled) {
    SimpleBLE::Config::Dongl::use_dongl_backend = enabled;
}

bool simpleble_config_dongl_get_auto_update(void) { return SimpleBLE::Config::Dongl::auto_update; }

void simpleble_config_dongl_set_auto_update(bool enabled) { SimpleBLE::Config::Dongl::auto_update = enabled; }

bool simpleble_config_dongl_get_force_update(void) { return SimpleBLE::Config::Dongl::force_update; }

void simpleble_config_dongl_set_force_update(bool enabled) { SimpleBLE::Config::Dongl::force_update = enabled; }
