#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <simplecble/export.h>

#include <simplecble/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Checks whether Bluetooth is enabled.
 *
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return true if Bluetooth is enabled; false otherwise or on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT bool simpleble_adapter_is_bluetooth_enabled(simpleble_error_t** out_error);

/**
 * @brief Counts the available Bluetooth adapters.
 *
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The number of available adapters, or zero on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT size_t simpleble_adapter_get_count(simpleble_error_t** out_error);

/**
 * @brief Obtains an owned handle for the adapter at the specified index.
 *
 * @param[in] index Zero-based index into the current collection.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned adapter handle, or NULL on failure. Release with simpleble_adapter_release_handle().
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT simpleble_adapter_t simpleble_adapter_get_handle(size_t index, simpleble_error_t** out_error);

/**
 * @brief Releases an owned adapter handle.
 *
 * @param[in] handle Handle to release, or NULL for no action.
 */
SIMPLECBLE_EXPORT void simpleble_adapter_release_handle(simpleble_adapter_t handle);

/**
 * @brief Retrieves the underlying OS object or handle.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The borrowed OS object or handle, or NULL if unavailable or on failure. Do not free it.
 * @note The native object type and its availability depend on the backend.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void* simpleble_adapter_underlying(simpleble_adapter_t handle, simpleble_error_t** out_error);

/**
 * @brief Retrieves the adapter identifier.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An allocated, NUL-terminated identifier, or NULL on failure. Release with simpleble_free().
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT char* simpleble_adapter_identifier(simpleble_adapter_t handle, simpleble_error_t** out_error);

/**
 * @brief Retrieves the adapter address.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An allocated, NUL-terminated address, or NULL on failure. Release with simpleble_free().
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT char* simpleble_adapter_address(simpleble_adapter_t handle, simpleble_error_t** out_error);

/**
 * @brief Requests that the adapter be powered on.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note Power control is backend-dependent; unsupported backends may do nothing.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_adapter_power_on(simpleble_adapter_t handle, simpleble_error_t** out_error);

/**
 * @brief Requests that the adapter be powered off.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note Power control is backend-dependent; unsupported backends may do nothing.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_adapter_power_off(simpleble_adapter_t handle, simpleble_error_t** out_error);

/**
 * @brief Checks whether the adapter is powered on.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return true if the adapter is powered on; false otherwise or on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT bool simpleble_adapter_is_powered(simpleble_adapter_t handle, simpleble_error_t** out_error);

/**
 * @brief Registers a callback invoked when the adapter powers on.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in] callback Non-NULL callback to register.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note The callback receives the registered adapter handle and userdata.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_adapter_set_callback_on_power_on(simpleble_adapter_t handle,
                                                                  void (*callback)(simpleble_adapter_t adapter,
                                                                                   void* userdata),
                                                                  void* userdata, simpleble_error_t** out_error);

/**
 * @brief Registers a callback invoked when the adapter powers off.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in] callback Non-NULL callback to register.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note The callback receives the registered adapter handle and userdata.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_adapter_set_callback_on_power_off(simpleble_adapter_t handle,
                                                                   void (*callback)(simpleble_adapter_t adapter,
                                                                                    void* userdata),
                                                                   void* userdata, simpleble_error_t** out_error);

/**
 * @brief Starts scanning for peripherals.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_adapter_scan_start(simpleble_adapter_t handle, simpleble_error_t** out_error);

/**
 * @brief Stops scanning for peripherals.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_adapter_scan_stop(simpleble_adapter_t handle, simpleble_error_t** out_error);

/**
 * @brief Checks whether the adapter is scanning.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return true if scanning is active; false otherwise or on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT bool simpleble_adapter_scan_is_active(simpleble_adapter_t handle, simpleble_error_t** out_error);

/**
 * @brief Scans for peripherals for the specified duration.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in] timeout_ms Nonnegative scan duration in milliseconds.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_adapter_scan_for(simpleble_adapter_t handle, int timeout_ms,
                                                  simpleble_error_t** out_error);

/**
 * @brief Counts the scan results reported by the adapter.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The number of scan results, or zero on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT size_t simpleble_adapter_scan_get_results_count(simpleble_adapter_t handle,
                                                                  simpleble_error_t** out_error);

/**
 * @brief Obtains an owned peripheral handle from the scan results.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in] index Zero-based index into the current collection.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned peripheral handle, or NULL on failure. Release with simpleble_peripheral_release_handle().
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT simpleble_peripheral_t simpleble_adapter_scan_get_results_handle(simpleble_adapter_t handle,
                                                                                   size_t index,
                                                                                   simpleble_error_t** out_error);

/**
 * @brief Counts the paired peripherals reported by the adapter.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The number of paired peripherals, or zero on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT size_t simpleble_adapter_get_paired_peripherals_count(simpleble_adapter_t handle,
                                                                        simpleble_error_t** out_error);

/**
 * @brief Obtains an owned peripheral handle from the paired peripherals.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in] index Zero-based index into the current collection.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned peripheral handle, or NULL on failure. Release with simpleble_peripheral_release_handle().
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT simpleble_peripheral_t simpleble_adapter_get_paired_peripherals_handle(simpleble_adapter_t handle,
                                                                                         size_t index,
                                                                                         simpleble_error_t** out_error);

/**
 * @brief Counts the connected peripherals reported by the adapter.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The number of connected peripherals, or zero on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT size_t simpleble_adapter_get_connected_peripherals_count(simpleble_adapter_t handle,
                                                                           simpleble_error_t** out_error);

/**
 * @brief Obtains an owned peripheral handle from the connected peripherals.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in] index Zero-based index into the current collection.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned peripheral handle, or NULL on failure. Release with simpleble_peripheral_release_handle().
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT simpleble_peripheral_t simpleble_adapter_get_connected_peripherals_handle(
    simpleble_adapter_t handle, size_t index, simpleble_error_t** out_error);

/**
 * @brief Registers a callback invoked when scanning starts.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in] callback Non-NULL callback to register.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note The callback receives the registered adapter handle and userdata.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_adapter_set_callback_on_scan_start(simpleble_adapter_t handle,
                                                                    void (*callback)(simpleble_adapter_t adapter,
                                                                                     void* userdata),
                                                                    void* userdata, simpleble_error_t** out_error);

/**
 * @brief Registers a callback invoked when scanning stops.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in] callback Non-NULL callback to register.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note The callback receives the registered adapter handle and userdata.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_adapter_set_callback_on_scan_stop(simpleble_adapter_t handle,
                                                                   void (*callback)(simpleble_adapter_t adapter,
                                                                                    void* userdata),
                                                                   void* userdata, simpleble_error_t** out_error);

/**
 * @brief Registers a callback invoked when a previously discovered peripheral is updated.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in] callback Non-NULL callback to register.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note Each callback transfers a newly allocated peripheral handle to the caller. Release it with
 *     simpleble_peripheral_release_handle(). The adapter handle is borrowed.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_adapter_set_callback_on_scan_updated(
    simpleble_adapter_t handle,
    void (*callback)(simpleble_adapter_t adapter, simpleble_peripheral_t peripheral, void* userdata), void* userdata,
    simpleble_error_t** out_error);

/**
 * @brief Registers a callback invoked when a peripheral is discovered.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in] callback Non-NULL callback to register.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note Each callback transfers a newly allocated peripheral handle to the caller. Release it with
 *     simpleble_peripheral_release_handle(). The adapter handle is borrowed.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_adapter_set_callback_on_scan_found(simpleble_adapter_t handle,
                                                                    void (*callback)(simpleble_adapter_t adapter,
                                                                                     simpleble_peripheral_t peripheral,
                                                                                     void* userdata),
                                                                    void* userdata, simpleble_error_t** out_error);

#ifdef __cplusplus
}
#endif
