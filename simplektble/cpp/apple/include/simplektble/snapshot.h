#pragma once

#include <stddef.h>

#include <simplecble/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Returns handles to every scan result, read at once.
 *
 * SimpleCBLE reads the results again for each index, so a scan in progress can shift them between calls.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[out] out_count Required, non-NULL pointer that receives the number of handles.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An array of owned peripheral handles, or NULL when there are none or on failure. Release each handle with
 * simpleble_peripheral_release_handle() and the array with simpleble_free().
 */
simpleble_peripheral_t* simplektble_adapter_scan_get_results(simpleble_adapter_t handle, size_t* out_count,
                                                             simpleble_error_t** out_error);

/**
 * @brief Returns handles to every paired peripheral, read at once.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[out] out_count Required, non-NULL pointer that receives the number of handles.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An array of owned peripheral handles, or NULL when there are none or on failure. Release each handle with
 * simpleble_peripheral_release_handle() and the array with simpleble_free().
 */
simpleble_peripheral_t* simplektble_adapter_get_paired_peripherals(simpleble_adapter_t handle, size_t* out_count,
                                                                   simpleble_error_t** out_error);

#ifdef __cplusplus
}
#endif
