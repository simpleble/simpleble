#pragma once

#include <simplecble/export.h>
#include <simplecble/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Releases an owned local peripheral handle.
 *
 * @param[in] handle Handle to release, or NULL for no action.
 * @note Clear callbacks registered with this handle before releasing it.
 */
SIMPLECBLE_EXPORT void simpleble_local_peripheral_release_handle(simpleble_local_peripheral_t handle);

/**
 * @brief Retrieves the underlying OS object or handle.
 *
 * @param[in] handle Valid, non-NULL local peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return A borrowed native handle, or NULL if unavailable or on failure. Do not free it.
 */
SIMPLECBLE_EXPORT void* simpleble_local_peripheral_underlying(simpleble_local_peripheral_t handle,
                                                              simpleble_error_t** out_error);

/**
 * @brief Adds a service UUID to the advertisement.
 *
 * @param[in] handle Valid, non-NULL local peripheral handle.
 * @param[in] service NUL-terminated service UUID.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note Configure advertised services before starting the peripheral.
 */
SIMPLECBLE_EXPORT void simpleble_local_peripheral_add_advertised_service(simpleble_local_peripheral_t handle,
                                                                         simpleble_uuid_t service,
                                                                         simpleble_error_t** out_error);

/**
 * @brief Adds a primary local GATT service.
 *
 * @param[in] handle Valid, non-NULL local peripheral handle.
 * @param[in] uuid NUL-terminated service UUID.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned service handle, or NULL on failure. Release with simpleble_local_service_release_handle().
 * @note Configure services before starting the peripheral.
 */
SIMPLECBLE_EXPORT simpleble_local_service_t simpleble_local_peripheral_add_service(simpleble_local_peripheral_t handle,
                                                                                   simpleble_uuid_t uuid,
                                                                                   simpleble_error_t** out_error);

/**
 * @brief Counts the local services.
 *
 * @param[in] handle Valid, non-NULL local peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The number of entries, or zero on failure.
 */
SIMPLECBLE_EXPORT size_t simpleble_local_peripheral_services_count(simpleble_local_peripheral_t handle,
                                                                   simpleble_error_t** out_error);

/**
 * @brief Obtains a local service handle at the specified index.
 *
 * @param[in] handle Valid, non-NULL local peripheral handle.
 * @param[in] index Zero-based index into the current collection.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned handle, or NULL on failure. Release with simpleble_local_service_release_handle().
 */
SIMPLECBLE_EXPORT simpleble_local_service_t simpleble_local_peripheral_services_get(simpleble_local_peripheral_t handle,
                                                                                    size_t index,
                                                                                    simpleble_error_t** out_error);

/**
 * @brief Removes all local GATT services before starting the peripheral.
 *
 * @param[in] handle Valid, non-NULL local peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 */
SIMPLECBLE_EXPORT void simpleble_local_peripheral_remove_all_services(simpleble_local_peripheral_t handle,
                                                                      simpleble_error_t** out_error);

/**
 * @brief Publishes the configured services and starts advertising.
 *
 * @param[in] handle Valid, non-NULL local peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note Configure services, characteristics, and advertised UUIDs before calling start().
 */
SIMPLECBLE_EXPORT void simpleble_local_peripheral_start(simpleble_local_peripheral_t handle,
                                                        simpleble_error_t** out_error);

/**
 * @brief Stops the local peripheral.
 *
 * @param[in] handle Valid, non-NULL local peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 */
SIMPLECBLE_EXPORT void simpleble_local_peripheral_stop(simpleble_local_peripheral_t handle,
                                                       simpleble_error_t** out_error);

/**
 * @brief Checks whether the local peripheral is started.
 *
 * @param[in] handle Valid, non-NULL local peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return true if active; false otherwise or on failure.
 */
SIMPLECBLE_EXPORT bool simpleble_local_peripheral_is_started(simpleble_local_peripheral_t handle,
                                                             simpleble_error_t** out_error);

/**
 * @brief Checks whether the local peripheral is advertising.
 *
 * @param[in] handle Valid, non-NULL local peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return true if active; false otherwise or on failure.
 */
SIMPLECBLE_EXPORT bool simpleble_local_peripheral_is_advertising(simpleble_local_peripheral_t handle,
                                                                 simpleble_error_t** out_error);

/**
 * @brief Sets or clears the client connected callback.
 *
 * @param[in] handle Valid, non-NULL local peripheral handle.
 * @param[in] callback Callback to register, or NULL to clear it.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @note The client address is borrowed for the callback duration and may be a platform-specific identifier.
 */
SIMPLECBLE_EXPORT void simpleble_local_peripheral_set_callback_on_client_connected(
    simpleble_local_peripheral_t handle,
    void (*callback)(simpleble_local_peripheral_t handle, const char* client_address, void* userdata), void* userdata);

/**
 * @brief Sets or clears the client disconnected callback.
 *
 * @param[in] handle Valid, non-NULL local peripheral handle.
 * @param[in] callback Callback to register, or NULL to clear it.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @note The client address is borrowed for the callback duration and may be a platform-specific identifier.
 */
SIMPLECBLE_EXPORT void simpleble_local_peripheral_set_callback_on_client_disconnected(
    simpleble_local_peripheral_t handle,
    void (*callback)(simpleble_local_peripheral_t handle, const char* client_address, void* userdata), void* userdata);

#ifdef __cplusplus
}
#endif
