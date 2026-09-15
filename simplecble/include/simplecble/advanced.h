#pragma once

#include <simplecble/export.h>
#include <simplecble/types.h>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#if defined(__ANDROID__)
#include <jni.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sets or clears the Dongl passkey-entry callback.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in] callback Callback to register, or NULL to clear it.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note Before connecting, register a callback that writes six decimal digits plus a NUL terminator
 *     into passkey and returns true. Return false to reject pairing. Leading zeroes must be preserved.
 * @note Callbacks run on an internal worker thread. Entry and comparison callbacks may block for user input.
 *     A missing callback rejects entry/comparison requests. A non-Dongl handle reports OPERATION_NOT_SUPPORTED.
 */
SIMPLECBLE_EXPORT void simpleble_advanced_dongl_set_passkey_request_callback(
    simpleble_peripheral_t handle, bool (*callback)(simpleble_peripheral_t handle, char passkey[7], void* userdata),
    void* userdata, simpleble_error_t** out_error);

/**
 * @brief Sets or clears the Dongl passkey-display callback.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in] callback Callback to register, or NULL to clear it.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note Register before connecting. The passkey is a borrowed, NUL-terminated string of six decimal digits,
 *     valid during the callback. Display it without removing leading zeroes.
 * @note Callbacks run on an internal worker thread. Entry and comparison callbacks may block for user input.
 *     A missing callback rejects entry/comparison requests. A non-Dongl handle reports OPERATION_NOT_SUPPORTED.
 */
SIMPLECBLE_EXPORT void simpleble_advanced_dongl_set_passkey_display_callback(
    simpleble_peripheral_t handle, void (*callback)(simpleble_peripheral_t handle, const char* passkey, void* userdata),
    void* userdata, simpleble_error_t** out_error);

/**
 * @brief Sets or clears the Dongl numeric-comparison callback.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in] callback Callback to register, or NULL to clear it.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note Register before connecting. The passkey is a borrowed, NUL-terminated string of six decimal digits,
 *     valid during the callback. Return true only after the user confirms the same number on the peer.
 * @note Callbacks run on an internal worker thread. Entry and comparison callbacks may block for user input.
 *     A missing callback rejects entry/comparison requests. A non-Dongl handle reports OPERATION_NOT_SUPPORTED.
 */
SIMPLECBLE_EXPORT void simpleble_advanced_dongl_set_numeric_comparison_callback(
    simpleble_peripheral_t handle, bool (*callback)(simpleble_peripheral_t handle, const char* passkey, void* userdata),
    void* userdata, simpleble_error_t** out_error);

#if defined(__APPLE__) && TARGET_OS_OSX

/**
 * @brief Sets or clears the advertised local name.
 *
 * @param[in] handle Valid, non-NULL local peripheral handle.
 * @param[in] local_name NUL-terminated name, or NULL to remove the override.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note Configure before starting the local peripheral. This changes only the advertisement, not the system name.
 */
SIMPLECBLE_EXPORT void simpleble_advanced_macos_set_advertisement_local_name(simpleble_local_peripheral_t handle,
                                                                             const char* local_name,
                                                                             simpleble_error_t** out_error);

/**
 * @brief Retrieves one peripheral from the CoreBluetooth system cache.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in] identifier NUL-terminated CoreBluetooth peripheral identifier.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned peripheral handle, or NULL if not found or on failure. Release with
 * simpleble_peripheral_release_handle().
 * @note Requires a powered-on adapter. Does not scan or establish reachability.
 *     Invalid or unknown identifiers return NULL without an error.
 */
SIMPLECBLE_EXPORT simpleble_peripheral_t simpleble_advanced_macos_retrieve_cached_peripheral(
    simpleble_adapter_t handle, const char* identifier, simpleble_error_t** out_error);

#endif

#if defined(__APPLE__) && TARGET_OS_IOS

/**
 * @brief Sets or clears the advertised local name.
 *
 * @param[in] handle Valid, non-NULL local peripheral handle.
 * @param[in] local_name NUL-terminated name, or NULL to remove the override.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note Configure before starting the local peripheral. This changes only the advertisement, not the system name.
 */
SIMPLECBLE_EXPORT void simpleble_advanced_ios_set_advertisement_local_name(simpleble_local_peripheral_t handle,
                                                                           const char* local_name,
                                                                           simpleble_error_t** out_error);

/**
 * @brief Retrieves one peripheral from the CoreBluetooth system cache.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in] identifier NUL-terminated CoreBluetooth peripheral identifier.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned peripheral handle, or NULL if not found or on failure. Release with
 * simpleble_peripheral_release_handle().
 * @note Requires a powered-on adapter. Does not scan or establish reachability.
 *     Invalid or unknown identifiers return NULL without an error.
 */
SIMPLECBLE_EXPORT simpleble_peripheral_t simpleble_advanced_ios_retrieve_cached_peripheral(
    simpleble_adapter_t handle, const char* identifier, simpleble_error_t** out_error);

#endif

#if defined(__linux__) && !defined(__ANDROID__)

/**
 * @brief Sets or clears the advertised local name.
 *
 * @param[in] handle Valid, non-NULL local peripheral handle.
 * @param[in] local_name NUL-terminated name, or NULL to remove the override.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note Configure before starting the local peripheral. This changes only the advertisement, not the system name.
 */
SIMPLECBLE_EXPORT void simpleble_advanced_linux_set_advertisement_local_name(simpleble_local_peripheral_t handle,
                                                                             const char* local_name,
                                                                             simpleble_error_t** out_error);

#endif

#if defined(__ANDROID__)

/**
 * @brief Retrieves the configured Android Java VM.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The borrowed Java VM pointer, or NULL on failure, including when no VM is configured.
 */
SIMPLECBLE_EXPORT JavaVM* simpleble_advanced_android_get_jvm(simpleble_error_t** out_error);

/**
 * @brief Sets the Java VM used by the Android backend.
 * @param[in] jvm Non-NULL Java VM pointer to use.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note Setting a different VM after one has already been configured reports an error.
 */
SIMPLECBLE_EXPORT void simpleble_advanced_android_set_jvm(JavaVM* jvm, simpleble_error_t** out_error);

/**
 * @brief Sets the application context used to host Android GATT services.
 *
 * @param[in] context Non-NULL application context; SimpleBLE retains a global reference.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note Configure before creating a local peripheral. Use an application context, not an Activity.
 */
SIMPLECBLE_EXPORT void simpleble_advanced_android_set_context(jobject context, simpleble_error_t** out_error);

#endif

#ifdef __cplusplus
}
#endif
