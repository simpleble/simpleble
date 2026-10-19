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
 * @brief Identifies the local service behind a handle.
 *
 * @param[in] handle A local service handle, or NULL.
 * @return A value that is the same for every handle to this service while any of them exists, or NULL for NULL.
 * @note Compare the value; never dereference it. A service created later can reuse it once every handle to this
 *     service is released.
 */
SIMPLECBLE_EXPORT const void* simpleble_local_service_identity(simpleble_local_service_t handle);

/**
 * @brief Counts the local services in a list.
 *
 * @param[in] list A local service list, or NULL.
 * @return The number of services in the list, or zero for NULL.
 */
SIMPLECBLE_EXPORT size_t simpleble_local_service_list_count(simpleble_local_service_list_t list);

/**
 * @brief Obtains an owned handle for the local service at the specified index of a list.
 *
 * @param[in] list Valid, non-NULL local service list.
 * @param[in] index Zero-based index into the list.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned handle, or NULL on failure. Release with simpleble_local_service_release_handle().
 * @note The handle remains valid after releasing the list.
 */
SIMPLECBLE_EXPORT simpleble_local_service_t simpleble_local_service_list_get(simpleble_local_service_list_t list,
                                                                             size_t index,
                                                                             simpleble_error_t** out_error);

/**
 * @brief Releases an owned local service list.
 *
 * @param[in] list List to release, or NULL for no action.
 */
SIMPLECBLE_EXPORT void simpleble_local_service_list_release(simpleble_local_service_list_t list);

/**
 * @brief Reads the local characteristics into a list.
 *
 * @param[in] handle Valid, non-NULL local service handle.
 * @param[out] out_list Required, non-NULL pointer that receives an owned list, or NULL on failure.
 *     Release with simpleble_local_characteristic_list_release().
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The number of characteristics in the list, or zero on failure.
 */
SIMPLECBLE_EXPORT size_t simpleble_local_service_characteristics(simpleble_local_service_t handle,
                                                                 simpleble_local_characteristic_list_t* out_list,
                                                                 simpleble_error_t** out_error);

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
