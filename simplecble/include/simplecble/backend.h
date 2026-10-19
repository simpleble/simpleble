#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <simplecble/export.h>
#include <simplecble/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reads the available backends into a list, initializing them as needed.
 *
 * @param[out] out_list Required, non-NULL pointer that receives an owned list, or NULL on failure.
 *     Release with simpleble_backend_list_release().
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The number of backends in the list, or zero on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT size_t simpleble_backend_get_backends(simpleble_backend_list_t* out_list,
                                                        simpleble_error_t** out_error);

/**
 * @brief Counts the backends in a list.
 *
 * @param[in] list A backend list, or NULL.
 * @return The number of backends in the list, or zero for NULL.
 */
SIMPLECBLE_EXPORT size_t simpleble_backend_list_count(simpleble_backend_list_t list);

/**
 * @brief Obtains an owned handle for the backend at the specified index of a list.
 *
 * @param[in] list Valid, non-NULL backend list.
 * @param[in] index Zero-based index into the list.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned backend handle, or NULL on failure. Release with simpleble_backend_release_handle().
 * @note The backend handle remains valid after releasing the list.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT simpleble_backend_t simpleble_backend_list_get(simpleble_backend_list_t list, size_t index,
                                                                 simpleble_error_t** out_error);

/**
 * @brief Releases an owned backend list.
 *
 * @param[in] list List to release, or NULL for no action.
 */
SIMPLECBLE_EXPORT void simpleble_backend_list_release(simpleble_backend_list_t list);

/**
 * @brief Obtains an owned handle for the backend of the operating system's Bluetooth stack.
 *
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned backend handle, or NULL on failure. Release with simpleble_backend_release_handle().
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT simpleble_backend_t simpleble_backend_get_native(simpleble_error_t** out_error);

/**
 * @brief Obtains an owned handle for the Dongl backend.
 *
 * This works whether or not the Dongl backend is part of the backend list. Fails with
 * SIMPLEBLE_ERROR_OPERATION_NOT_SUPPORTED if this build of SimpleBLE doesn't include it.
 *
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned backend handle, or NULL on failure. Release with simpleble_backend_release_handle().
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT simpleble_backend_t simpleble_backend_get_dongl(simpleble_error_t** out_error);

/**
 * @brief Obtains an owned handle for the Simulator backend.
 *
 * It only has adapters while a simulation environment is active. Fails with
 * SIMPLEBLE_ERROR_OPERATION_NOT_SUPPORTED if this build of SimpleBLE doesn't include it.
 *
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned backend handle, or NULL on failure. Release with simpleble_backend_release_handle().
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT simpleble_backend_t simpleble_backend_get_simulator(simpleble_error_t** out_error);

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
 * @brief Reads the adapters belonging to the backend into a list.
 *
 * @param[in] handle Valid, non-NULL backend handle.
 * @param[out] out_list Required, non-NULL pointer that receives an owned list, or NULL on failure.
 *     Release with simpleble_adapter_list_release().
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The number of adapters in the list, or zero on failure.
 * @note The list and its adapter handles remain valid after releasing the backend handle.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT size_t simpleble_backend_get_adapters(simpleble_backend_t handle, simpleble_adapter_list_t* out_list,
                                                        simpleble_error_t** out_error);

#ifdef __cplusplus
}
#endif
