#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <simplecble/export.h>
#include <simplecble/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Counts the available backends, initializing them as needed.
 *
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The number of available backends, or zero on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT size_t simpleble_backend_get_count(simpleble_error_t** out_error);

/**
 * @brief Obtains an owned handle for the backend at the specified index.
 *
 * @param[in] index Zero-based index into the current collection.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned backend handle, or NULL on failure. Release with simpleble_backend_release_handle().
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT simpleble_backend_t simpleble_backend_get_handle(size_t index, simpleble_error_t** out_error);

/**
 * @brief Releases an owned backend handle.
 *
 * @param[in] handle Handle to release, or NULL for no action.
 */
SIMPLECBLE_EXPORT void simpleble_backend_release_handle(simpleble_backend_t handle);

/**
 * @brief Retrieves the backend identifier.
 *
 * @param[in] handle Valid, non-NULL backend handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An allocated, NUL-terminated identifier, or NULL on failure. Release with simpleble_free().
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT char* simpleble_backend_identifier(simpleble_backend_t handle, simpleble_error_t** out_error);

/**
 * @brief Checks whether Bluetooth is enabled for the backend.
 *
 * @param[in] handle Valid, non-NULL backend handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return true if Bluetooth is enabled; false otherwise or on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT bool simpleble_backend_is_bluetooth_enabled(simpleble_backend_t handle,
                                                              simpleble_error_t** out_error);

/**
 * @brief Counts the available adapters belonging to the backend.
 *
 * @param[in] handle Valid, non-NULL backend handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The number of available adapters, or zero on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT size_t simpleble_backend_get_adapters_count(simpleble_backend_t handle,
                                                              simpleble_error_t** out_error);

/**
 * @brief Obtains an owned adapter handle from the backend.
 *
 * @param[in] handle Valid, non-NULL backend handle.
 * @param[in] index Zero-based index into the backend's current adapter collection.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned adapter handle, or NULL on failure. Release with simpleble_adapter_release_handle().
 * @note The adapter handle remains valid after releasing the backend handle.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT simpleble_adapter_t simpleble_backend_get_adapters_handle(simpleble_backend_t handle, size_t index,
                                                                            simpleble_error_t** out_error);

#ifdef __cplusplus
}
#endif
