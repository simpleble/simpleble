#pragma once

#include <simplecble/export.h>
#include <simplecble/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Releases an owned local characteristic handle.
 *
 * @param[in] handle Handle to release, or NULL for no action.
 * @note Clear callbacks registered with this handle before releasing it.
 */
SIMPLECBLE_EXPORT void simpleble_local_characteristic_release_handle(simpleble_local_characteristic_t handle);

/**
 * @brief Retrieves the local characteristic UUID.
 *
 * @param[in] handle Valid, non-NULL local characteristic handle.
 * @param[out] out_uuid Required storage for the NUL-terminated UUID.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 */
SIMPLECBLE_EXPORT void simpleble_local_characteristic_uuid(simpleble_local_characteristic_t handle,
                                                           simpleble_uuid_t* out_uuid, simpleble_error_t** out_error);

/**
 * @brief Retrieves the local characteristic capabilities.
 *
 * @param[in] handle Valid, non-NULL local characteristic handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return A bitwise OR of simpleble_local_characteristic_capability_t values, or zero on failure.
 */
SIMPLECBLE_EXPORT uint32_t simpleble_local_characteristic_capabilities(simpleble_local_characteristic_t handle,
                                                                       simpleble_error_t** out_error);

/**
 * @brief Copies the current local characteristic value.
 *
 * @param[in] handle Valid, non-NULL local characteristic handle.
 * @param[out] data_length Required output for the number of bytes returned.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An allocated buffer, or NULL for an empty value or on failure. Release with simpleble_free().
 */
SIMPLECBLE_EXPORT uint8_t* simpleble_local_characteristic_value(simpleble_local_characteristic_t handle,
                                                                size_t* data_length, simpleble_error_t** out_error);

/**
 * @brief Updates the current local characteristic value.
 *
 * @param[in] handle Valid, non-NULL local characteristic handle.
 * @param[in] data Bytes to copy; may be NULL when data_length is zero.
 * @param[in] data_length Number of bytes to copy.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note For NOTIFY or INDICATE characteristics, publishes the value to subscribers on a best-effort basis.
 */
SIMPLECBLE_EXPORT void simpleble_local_characteristic_set_value(simpleble_local_characteristic_t handle,
                                                                const uint8_t* data, size_t data_length,
                                                                simpleble_error_t** out_error);

/**
 * @brief Sets or clears the callback that supplies a dynamic read value.
 *
 * @param[in] handle Valid, non-NULL local characteristic handle.
 * @param[in] callback Callback to register, or NULL to use the stored value.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @note Set *data_length and return a borrowed buffer, or return NULL for an empty value.
 *     The buffer is copied after the callback returns; keep it valid and unchanged until the next read callback
 *     or until the callback is cleared. Do not return a pointer to stack storage.
 */
SIMPLECBLE_EXPORT void simpleble_local_characteristic_set_callback_on_read(
    simpleble_local_characteristic_t handle,
    const uint8_t* (*callback)(simpleble_local_characteristic_t handle, size_t* data_length, void* userdata),
    void* userdata);

/**
 * @brief Sets or clears the callback invoked after a client writes a value.
 *
 * @param[in] handle Valid, non-NULL local characteristic handle.
 * @param[in] callback Callback to register, or NULL to clear it.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @note Data is borrowed for the callback duration. The stored value is updated before the callback runs.
 */
SIMPLECBLE_EXPORT void simpleble_local_characteristic_set_callback_on_write(
    simpleble_local_characteristic_t handle,
    void (*callback)(simpleble_local_characteristic_t handle, const uint8_t* data, size_t data_length, void* userdata),
    void* userdata);

/**
 * @brief Sets or clears the callback invoked when the first client subscribes.
 *
 * @param[in] handle Valid, non-NULL local characteristic handle.
 * @param[in] callback Callback to register, or NULL to clear it.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 */
SIMPLECBLE_EXPORT void simpleble_local_characteristic_set_callback_on_subscribed(
    simpleble_local_characteristic_t handle, void (*callback)(simpleble_local_characteristic_t handle, void* userdata),
    void* userdata);

/**
 * @brief Sets or clears the callback invoked when the last client unsubscribes.
 *
 * @param[in] handle Valid, non-NULL local characteristic handle.
 * @param[in] callback Callback to register, or NULL to clear it.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 */
SIMPLECBLE_EXPORT void simpleble_local_characteristic_set_callback_on_unsubscribed(
    simpleble_local_characteristic_t handle, void (*callback)(simpleble_local_characteristic_t handle, void* userdata),
    void* userdata);

#ifdef __cplusplus
}
#endif
