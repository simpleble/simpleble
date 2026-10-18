#pragma once

#include <simplecble/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The SimpleBLE object behind an adapter handle.
 * @note Handles to the same adapter return the same value, although SimpleCBLE returns a new handle every time.
 */
const void* simplektble_adapter_identity(simpleble_adapter_t handle);

/**
 * @brief The SimpleBLE object behind a peripheral handle.
 * @note Handles to the same device return the same value, although SimpleCBLE returns a new handle every time.
 */
const void* simplektble_peripheral_identity(simpleble_peripheral_t handle);

#ifdef __cplusplus
}
#endif
