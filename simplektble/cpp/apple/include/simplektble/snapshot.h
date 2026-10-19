#pragma once

#include <stddef.h>

#include <simplecble/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Returns handles to the peripherals CoreBluetooth already knows by these identifiers, read at once.
 *
 * SimpleCBLE retrieves one identifier per call, under a different name on iOS and macOS, and only with the
 * CoreBluetooth backend. Other backends report OPERATION_NOT_SUPPORTED.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in] identifiers NUL-terminated identifiers; may be NULL when identifier_count is zero.
 * @param[in] identifier_count The number of identifiers.
 * @param[out] out_count Required, non-NULL pointer that receives the number of handles.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An array of owned peripheral handles, or NULL when there are none or on failure. Release each handle with
 * simpleble_peripheral_release_handle() and the array with simpleble_free().
 */
simpleble_peripheral_t* simplektble_adapter_retrieve_cached_peripherals(simpleble_adapter_t handle,
                                                                        const char* const* identifiers,
                                                                        size_t identifier_count, size_t* out_count,
                                                                        simpleble_error_t** out_error);

#ifdef __cplusplus
}
#endif
