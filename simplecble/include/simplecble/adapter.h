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
 * @brief Reads the available Bluetooth adapters of every backend into a list.
 *
 * @param[out] out_list Required, non-NULL pointer that receives an owned list, or NULL on failure.
 *     Release with simpleble_adapter_list_release().
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The number of adapters in the list, or zero on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT size_t simpleble_adapter_get_adapters(simpleble_adapter_list_t* out_list,
                                                        simpleble_error_t** out_error);

/**
 * @brief Counts the adapters in a list.
 *
 * @param[in] list An adapter list, or NULL.
 * @return The number of adapters in the list, or zero for NULL.
 */
SIMPLECBLE_EXPORT size_t simpleble_adapter_list_count(simpleble_adapter_list_t list);

/**
 * @brief Obtains an owned handle for the adapter at the specified index of a list.
 *
 * @param[in] list Valid, non-NULL adapter list.
 * @param[in] index Zero-based index into the list.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned adapter handle, or NULL on failure. Release with simpleble_adapter_release_handle().
 * @note The adapter handle remains valid after releasing the list.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT simpleble_adapter_t simpleble_adapter_list_get(simpleble_adapter_list_t list, size_t index,
                                                                 simpleble_error_t** out_error);

/**
 * @brief Releases an owned adapter list.
 *
 * @param[in] list List to release, or NULL for no action.
 */
SIMPLECBLE_EXPORT void simpleble_adapter_list_release(simpleble_adapter_list_t list);

/**
 * @brief Releases an owned adapter handle.
 *
 * @param[in] handle Handle to release, or NULL for no action.
 */
SIMPLECBLE_EXPORT void simpleble_adapter_release_handle(simpleble_adapter_t handle);

/**
 * @brief Identifies the adapter behind a handle.
 *
 * @param[in] handle An adapter handle, or NULL.
 * @return A value that is the same for every handle to this adapter while any of them exists, or NULL for NULL.
 * @note Compare the value; never dereference it. An adapter created later can reuse it once every handle to this
 *     adapter is released.
 */
SIMPLECBLE_EXPORT const void* simpleble_adapter_identity(simpleble_adapter_t handle);

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
 * @param[in] callback Callback to register, or NULL to clear it.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @note The callback receives the registered adapter handle and userdata.
 */
SIMPLECBLE_EXPORT void simpleble_adapter_set_callback_on_power_on(
    simpleble_adapter_t handle, void (*callback)(simpleble_adapter_t adapter, void* userdata), void* userdata);

/**
 * @brief Registers a callback invoked when the adapter powers off.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in] callback Callback to register, or NULL to clear it.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @note The callback receives the registered adapter handle and userdata.
 */
SIMPLECBLE_EXPORT void simpleble_adapter_set_callback_on_power_off(
    simpleble_adapter_t handle, void (*callback)(simpleble_adapter_t adapter, void* userdata), void* userdata);

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
 * @brief Reads the scan results reported by the adapter into a list.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[out] out_list Required, non-NULL pointer that receives an owned list, or NULL on failure.
 *     Release with simpleble_peripheral_list_release().
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The number of peripherals in the list, or zero on failure.
 * @note The list keeps the results it read while scanning continues or restarts.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT size_t simpleble_adapter_scan_get_results(simpleble_adapter_t handle,
                                                            simpleble_peripheral_list_t* out_list,
                                                            simpleble_error_t** out_error);

/**
 * @brief Reads the paired peripherals reported by the adapter into a list.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[out] out_list Required, non-NULL pointer that receives an owned list, or NULL on failure.
 *     Release with simpleble_peripheral_list_release().
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The number of peripherals in the list, or zero on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT size_t simpleble_adapter_get_paired_peripherals(simpleble_adapter_t handle,
                                                                  simpleble_peripheral_list_t* out_list,
                                                                  simpleble_error_t** out_error);

/**
 * @brief Reads the connected peripherals reported by the adapter into a list.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[out] out_list Required, non-NULL pointer that receives an owned list, or NULL on failure.
 *     Release with simpleble_peripheral_list_release().
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The number of peripherals in the list, or zero on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT size_t simpleble_adapter_get_connected_peripherals(simpleble_adapter_t handle,
                                                                     simpleble_peripheral_list_t* out_list,
                                                                     simpleble_error_t** out_error);

/**
 * @brief Registers a callback invoked when scanning starts.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in] callback Callback to register, or NULL to clear it.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @note The callback receives the registered adapter handle and userdata.
 */
SIMPLECBLE_EXPORT void simpleble_adapter_set_callback_on_scan_start(
    simpleble_adapter_t handle, void (*callback)(simpleble_adapter_t adapter, void* userdata), void* userdata);

/**
 * @brief Registers a callback invoked when scanning stops.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in] callback Callback to register, or NULL to clear it.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @note The callback receives the registered adapter handle and userdata.
 */
SIMPLECBLE_EXPORT void simpleble_adapter_set_callback_on_scan_stop(
    simpleble_adapter_t handle, void (*callback)(simpleble_adapter_t adapter, void* userdata), void* userdata);

/**
 * @brief Registers a callback invoked when a previously discovered peripheral is updated.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in] callback Callback to register, or NULL to clear it.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @note Each callback transfers a newly allocated peripheral handle to the caller. Release it with
 *     simpleble_peripheral_release_handle(). The adapter handle is borrowed.
 */
SIMPLECBLE_EXPORT void simpleble_adapter_set_callback_on_scan_updated(
    simpleble_adapter_t handle,
    void (*callback)(simpleble_adapter_t adapter, simpleble_peripheral_t peripheral, void* userdata), void* userdata);

/**
 * @brief Registers a callback invoked when a peripheral is discovered.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in] callback Callback to register, or NULL to clear it.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @note Each callback transfers a newly allocated peripheral handle to the caller. Release it with
 *     simpleble_peripheral_release_handle(). The adapter handle is borrowed.
 */
SIMPLECBLE_EXPORT void simpleble_adapter_set_callback_on_scan_found(
    simpleble_adapter_t handle,
    void (*callback)(simpleble_adapter_t adapter, simpleble_peripheral_t peripheral, void* userdata), void* userdata);

/**
 * @brief Creates a local BLE peripheral host.
 *
 * @param[in] handle Valid, non-NULL adapter handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An owned local peripheral handle, or NULL on failure. Release with
 * simpleble_local_peripheral_release_handle().
 * @note Configure its GATT services and advertised UUIDs before starting it. Stop it before releasing the handle.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT simpleble_local_peripheral_t simpleble_adapter_create_local_peripheral(simpleble_adapter_t handle,
                                                                                         simpleble_error_t** out_error);

#ifdef __cplusplus
}
#endif
