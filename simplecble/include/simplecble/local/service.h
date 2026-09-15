#pragma once

#include <simplecble/export.h>
#include <simplecble/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Releases an owned local service handle.
 *
 * @param[in] handle Handle to release, or NULL for no action.
 * @note Clear callbacks registered with this handle before releasing it.
 */
SIMPLECBLE_EXPORT void simpleble_local_service_release_handle(simpleble_local_service_t handle);

/**
 * @brief Counts the local characteristics.
 *
 * @param[in] handle Valid, non-NULL local service handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The number of entries, or zero on failure.
 */
SIMPLECBLE_EXPORT size_t simpleble_local_service_characteristics_count(simpleble_local_service_t handle,
                                                                       simpleble_error_t** out_error);

/**
 * @brief Obtains a local characteristic handle at the specified index.
 *
 * @param[in] handle Valid, non-NULL local service handle.
 * @param[in] index Zero-based index into the current collection.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned handle, or NULL on failure. Release with simpleble_local_characteristic_release_handle().
 */
SIMPLECBLE_EXPORT simpleble_local_characteristic_t simpleble_local_service_characteristics_get(
    simpleble_local_service_t handle, size_t index, simpleble_error_t** out_error);

/**
 * @brief Retrieves the local service UUID.
 *
 * @param[in] handle Valid, non-NULL local service handle.
 * @param[out] out_uuid Required storage for the NUL-terminated UUID.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 */
SIMPLECBLE_EXPORT void simpleble_local_service_uuid(simpleble_local_service_t handle, simpleble_uuid_t* out_uuid,
                                                    simpleble_error_t** out_error);

/**
 * @brief Adds a characteristic to the local service.
 *
 * @param[in] handle Valid, non-NULL local service handle.
 * @param[in] uuid NUL-terminated characteristic UUID.
 * @param[in] capabilities Bitwise OR of simpleble_local_characteristic_capability_t values.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned handle, or NULL on failure. Release with simpleble_local_characteristic_release_handle().
 * @note Add characteristics before starting the local peripheral.
 */
SIMPLECBLE_EXPORT simpleble_local_characteristic_t simpleble_local_service_add_characteristic(
    simpleble_local_service_t handle, simpleble_uuid_t uuid, uint32_t capabilities, simpleble_error_t** out_error);

#ifdef __cplusplus
}
#endif
