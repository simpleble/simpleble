#pragma once

#include <simplecble/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sets or clears the advertised local name.
 *
 * SimpleCBLE has this under a different name on iOS and macOS, and only with the CoreBluetooth backend. Other backends
 * report OPERATION_NOT_SUPPORTED.
 *
 * @param[in] handle Valid, non-NULL local peripheral handle.
 * @param[in] local_name NUL-terminated name, or NULL to remove the override.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 */
void simplektble_local_peripheral_set_advertisement_local_name(simpleble_local_peripheral_t handle,
                                                               const char* local_name, simpleble_error_t** out_error);

#ifdef __cplusplus
}
#endif
