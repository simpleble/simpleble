#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <simplecble/export.h>
#include <simplecble/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SIMPLEBLE_CONFIG_ANDROID_CONNECTION_PRIORITY_DISABLED = -1,
    SIMPLEBLE_CONFIG_ANDROID_CONNECTION_PRIORITY_BALANCED = 0,
    SIMPLEBLE_CONFIG_ANDROID_CONNECTION_PRIORITY_HIGH = 1,
    SIMPLEBLE_CONFIG_ANDROID_CONNECTION_PRIORITY_LOW_POWER = 2,
    SIMPLEBLE_CONFIG_ANDROID_CONNECTION_PRIORITY_DCK = 3,
} simpleble_config_android_connection_priority_t;

/**
 * @brief Reset all SimpleBLE configuration values to their defaults.
 *
 * @note Configuration values must be set before any adapter handles are retrieved.
 */
SIMPLECBLE_EXPORT void simpleble_config_reset_all(void);

SIMPLECBLE_EXPORT void simpleble_config_simplebluez_reset(void);
SIMPLECBLE_EXPORT bool simpleble_config_simplebluez_get_use_legacy_bluez_backend(void);
SIMPLECBLE_EXPORT void simpleble_config_simplebluez_set_use_legacy_bluez_backend(bool enabled);
SIMPLECBLE_EXPORT bool simpleble_config_simplebluez_get_use_system_bus(void);
SIMPLECBLE_EXPORT void simpleble_config_simplebluez_set_use_system_bus(bool enabled);
SIMPLECBLE_EXPORT int64_t simpleble_config_simplebluez_get_connection_timeout_ms(void);
SIMPLECBLE_EXPORT void simpleble_config_simplebluez_set_connection_timeout_ms(int64_t timeout_ms);
SIMPLECBLE_EXPORT int64_t simpleble_config_simplebluez_get_disconnection_timeout_ms(void);
SIMPLECBLE_EXPORT void simpleble_config_simplebluez_set_disconnection_timeout_ms(int64_t timeout_ms);

SIMPLECBLE_EXPORT void simpleble_config_winrt_reset(void);

/**
 * @deprecated SimpleBLE uses its own WinRT MTA apartment by default.
 *             This compatibility flag will be removed in a future release.
 */
SIMPLECBLE_EXPORT bool simpleble_config_winrt_get_experimental_use_own_mta_apartment(void);

/**
 * @deprecated SimpleBLE uses its own WinRT MTA apartment by default.
 *             This compatibility flag will be removed in a future release.
 */
SIMPLECBLE_EXPORT void simpleble_config_winrt_set_experimental_use_own_mta_apartment(bool enabled);
SIMPLECBLE_EXPORT bool simpleble_config_winrt_get_experimental_reinitialize_winrt_apartment_on_main_thread(void);
SIMPLECBLE_EXPORT void simpleble_config_winrt_set_experimental_reinitialize_winrt_apartment_on_main_thread(
    bool enabled);
SIMPLECBLE_EXPORT bool simpleble_config_winrt_get_use_deferred_disconnect(void);
SIMPLECBLE_EXPORT void simpleble_config_winrt_set_use_deferred_disconnect(bool enabled);

SIMPLECBLE_EXPORT void simpleble_config_corebluetooth_reset(void);

SIMPLECBLE_EXPORT void simpleble_config_android_reset(void);
SIMPLECBLE_EXPORT simpleble_config_android_connection_priority_t
simpleble_config_android_get_connection_priority(void);
SIMPLECBLE_EXPORT void simpleble_config_android_set_connection_priority(
    simpleble_config_android_connection_priority_t priority);

/**
 * @brief Set the Android connection priority request using the raw Android value.
 *
 * @note This compatibility helper is intended for FFI users that marshal enums as integers.
 *       Known values are -1 (disabled), 0 (balanced), 1 (high), 2 (low power), and 3 (DCK).
 */
SIMPLECBLE_EXPORT void simpleble_config_set_android_connection_priority(int priority);

SIMPLECBLE_EXPORT void simpleble_config_dongl_reset(void);
SIMPLECBLE_EXPORT bool simpleble_config_dongl_get_use_dongl_backend(void);
SIMPLECBLE_EXPORT void simpleble_config_dongl_set_use_dongl_backend(bool enabled);
SIMPLECBLE_EXPORT bool simpleble_config_dongl_get_auto_update(void);
SIMPLECBLE_EXPORT void simpleble_config_dongl_set_auto_update(bool enabled);
SIMPLECBLE_EXPORT bool simpleble_config_dongl_get_force_update(void);
SIMPLECBLE_EXPORT void simpleble_config_dongl_set_force_update(bool enabled);

#ifdef __cplusplus
}
#endif
