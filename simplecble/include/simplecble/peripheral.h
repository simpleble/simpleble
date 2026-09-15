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
 * @brief Releases an owned peripheral handle.
 *
 * @param[in] handle Handle to release, or NULL for no action.
 */
SIMPLECBLE_EXPORT void simpleble_peripheral_release_handle(simpleble_peripheral_t handle);

/**
 * @brief Retrieves the underlying OS object or handle.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The borrowed OS object or handle, or NULL if unavailable or on failure. Do not free it.
 * @note The native object type and its availability depend on the backend.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void* simpleble_peripheral_underlying(simpleble_peripheral_t handle, simpleble_error_t** out_error);

/**
 * @brief Retrieves the peripheral identifier.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An allocated, NUL-terminated identifier, or NULL on failure. Release with simpleble_free().
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT char* simpleble_peripheral_identifier(simpleble_peripheral_t handle, simpleble_error_t** out_error);

/**
 * @brief Retrieves the peripheral address.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An allocated, NUL-terminated address, or NULL on failure. Release with simpleble_free().
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT char* simpleble_peripheral_address(simpleble_peripheral_t handle, simpleble_error_t** out_error);

/**
 * @brief Retrieves the peripheral address type.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The address type, or SIMPLEBLE_ADDRESS_TYPE_UNSPECIFIED if unknown or on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT simpleble_address_type_t simpleble_peripheral_address_type(simpleble_peripheral_t handle,
                                                                             simpleble_error_t** out_error);

/**
 * @brief Retrieves the received signal strength.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The RSSI in dBm, or INT16_MIN if unavailable or on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT int16_t simpleble_peripheral_rssi(simpleble_peripheral_t handle, simpleble_error_t** out_error);

/**
 * @brief Retrieves the advertised transmit power.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The advertised transmit power in dBm, or INT16_MIN if unavailable or on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT int16_t simpleble_peripheral_tx_power(simpleble_peripheral_t handle, simpleble_error_t** out_error);

/**
 * @brief Retrieves the MTU reported by the backend.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The MTU in bytes reported by the backend, or zero on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT uint16_t simpleble_peripheral_mtu(simpleble_peripheral_t handle, simpleble_error_t** out_error);

/**
 * @brief Connects to the peripheral.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_peripheral_connect(simpleble_peripheral_t handle, simpleble_error_t** out_error);

/**
 * @brief Disconnects from the peripheral.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_peripheral_disconnect(simpleble_peripheral_t handle, simpleble_error_t** out_error);

/**
 * @brief Checks whether the peripheral is connected.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return true if the peripheral is connected; false otherwise or on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT bool simpleble_peripheral_is_connected(simpleble_peripheral_t handle, simpleble_error_t** out_error);

/**
 * @brief Checks whether the peripheral is connectable.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return true if the peripheral is connectable; false otherwise or on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT bool simpleble_peripheral_is_connectable(simpleble_peripheral_t handle,
                                                           simpleble_error_t** out_error);

/**
 * @brief Checks whether the peripheral is paired.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return true if the peripheral is paired; false otherwise or on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT bool simpleble_peripheral_is_paired(simpleble_peripheral_t handle, simpleble_error_t** out_error);

/**
 * @brief Removes the pairing with the peripheral.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_peripheral_unpair(simpleble_peripheral_t handle, simpleble_error_t** out_error);

/**
 * @brief Counts the services reported by the peripheral.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The number of services, or zero on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT size_t simpleble_peripheral_services_count(simpleble_peripheral_t handle,
                                                             simpleble_error_t** out_error);

/**
 * @brief Retrieves the service at the specified index.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in] index Zero-based index into the current collection.
 * @param[out] out_service Required caller-owned storage for one service and its characteristics and descriptors.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note Characteristic and descriptor counts are clamped to their fixed array capacities. Service data is
 *     also truncated to its array capacity, but data_length reports the original length. Read at most
 *     sizeof(out_service->data) bytes.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_peripheral_services_get(simpleble_peripheral_t handle, size_t index,
                                                         simpleble_service_t* out_service,
                                                         simpleble_error_t** out_error);

/**
 * @brief Counts the manufacturer data entries reported by the peripheral.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return The number of manufacturer data entries, or zero on failure.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT size_t simpleble_peripheral_manufacturer_data_count(simpleble_peripheral_t handle,
                                                                      simpleble_error_t** out_error);

/**
 * @brief Retrieves the manufacturer data entry at the specified index.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in] index Zero-based index into the current collection.
 * @param[out] out_data Required caller-owned storage for one manufacturer data entry.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note Data is truncated to the fixed array capacity, but data_length reports the original length. Read
 *     at most sizeof(out_data->data) bytes.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_peripheral_manufacturer_data_get(simpleble_peripheral_t handle, size_t index,
                                                                  simpleble_manufacturer_data_t* out_data,
                                                                  simpleble_error_t** out_error);

/**
 * @brief Reads a characteristic value.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in] service Service UUID, NUL-terminated within its value array.
 * @param[in] characteristic Characteristic UUID, NUL-terminated within its value array.
 * @param[out] data_length Required output for the number of bytes read.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An allocated byte buffer, or NULL. Release the returned buffer with simpleble_free().
 * @note For an empty value, *data_length is zero and the returned pointer may be NULL.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT uint8_t* simpleble_peripheral_read(simpleble_peripheral_t handle, simpleble_uuid_t service,
                                                     simpleble_uuid_t characteristic, size_t* data_length,
                                                     simpleble_error_t** out_error);

/**
 * @brief Writes a characteristic value with a response.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in] service Service UUID, NUL-terminated within its value array.
 * @param[in] characteristic Characteristic UUID, NUL-terminated within its value array.
 * @param[in] data Bytes to write; must be non-NULL, even when data_length is zero.
 * @param[in] data_length Number of readable bytes in data to write.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_peripheral_write_request(simpleble_peripheral_t handle, simpleble_uuid_t service,
                                                          simpleble_uuid_t characteristic, const uint8_t* data,
                                                          size_t data_length, simpleble_error_t** out_error);

/**
 * @brief Writes a characteristic value without a response.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in] service Service UUID, NUL-terminated within its value array.
 * @param[in] characteristic Characteristic UUID, NUL-terminated within its value array.
 * @param[in] data Bytes to write; must be non-NULL, even when data_length is zero.
 * @param[in] data_length Number of readable bytes in data to write.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_peripheral_write_command(simpleble_peripheral_t handle, simpleble_uuid_t service,
                                                          simpleble_uuid_t characteristic, const uint8_t* data,
                                                          size_t data_length, simpleble_error_t** out_error);

/**
 * @brief Subscribes to characteristic notifications.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in] service Service UUID, NUL-terminated within its value array.
 * @param[in] characteristic Characteristic UUID, NUL-terminated within its value array.
 * @param[in] callback Non-NULL callback to register.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note The callback data buffer is borrowed and valid only during the callback. The callback receives the
 *     registered peripheral handle, UUIDs, and userdata.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_peripheral_notify(
    simpleble_peripheral_t handle, simpleble_uuid_t service, simpleble_uuid_t characteristic,
    void (*callback)(simpleble_peripheral_t handle, simpleble_uuid_t service, simpleble_uuid_t characteristic,
                     const uint8_t* data, size_t data_length, void* userdata),
    void* userdata, simpleble_error_t** out_error);

/**
 * @brief Subscribes to characteristic indications.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in] service Service UUID, NUL-terminated within its value array.
 * @param[in] characteristic Characteristic UUID, NUL-terminated within its value array.
 * @param[in] callback Non-NULL callback to register.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note The callback data buffer is borrowed and valid only during the callback. The callback receives the
 *     registered peripheral handle, UUIDs, and userdata.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_peripheral_indicate(
    simpleble_peripheral_t handle, simpleble_uuid_t service, simpleble_uuid_t characteristic,
    void (*callback)(simpleble_peripheral_t handle, simpleble_uuid_t service, simpleble_uuid_t characteristic,
                     const uint8_t* data, size_t data_length, void* userdata),
    void* userdata, simpleble_error_t** out_error);

/**
 * @brief Unsubscribes from characteristic notifications or indications.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in] service Service UUID, NUL-terminated within its value array.
 * @param[in] characteristic Characteristic UUID, NUL-terminated within its value array.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_peripheral_unsubscribe(simpleble_peripheral_t handle, simpleble_uuid_t service,
                                                        simpleble_uuid_t characteristic, simpleble_error_t** out_error);

/**
 * @brief Reads a descriptor value.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in] service Service UUID, NUL-terminated within its value array.
 * @param[in] characteristic Characteristic UUID, NUL-terminated within its value array.
 * @param[in] descriptor Descriptor UUID, NUL-terminated within its value array.
 * @param[out] data_length Required output for the number of bytes read.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @return An allocated byte buffer, or NULL. Release the returned buffer with simpleble_free().
 * @note For an empty value, *data_length is zero and the returned pointer may be NULL.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT uint8_t* simpleble_peripheral_read_descriptor(simpleble_peripheral_t handle, simpleble_uuid_t service,
                                                                simpleble_uuid_t characteristic,
                                                                simpleble_uuid_t descriptor, size_t* data_length,
                                                                simpleble_error_t** out_error);

/**
 * @brief Writes a descriptor value.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in] service Service UUID, NUL-terminated within its value array.
 * @param[in] characteristic Characteristic UUID, NUL-terminated within its value array.
 * @param[in] descriptor Descriptor UUID, NUL-terminated within its value array.
 * @param[in] data Bytes to write; must be non-NULL, even when data_length is zero.
 * @param[in] data_length Number of readable bytes in data to write.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_peripheral_write_descriptor(simpleble_peripheral_t handle, simpleble_uuid_t service,
                                                             simpleble_uuid_t characteristic,
                                                             simpleble_uuid_t descriptor, const uint8_t* data,
                                                             size_t data_length, simpleble_error_t** out_error);

/**
 * @brief Registers a callback invoked when the peripheral connects.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in] callback Non-NULL callback to register.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note The callback receives the registered peripheral handle and userdata.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_peripheral_set_callback_on_connected(
    simpleble_peripheral_t handle, void (*callback)(simpleble_peripheral_t peripheral, void* userdata), void* userdata,
    simpleble_error_t** out_error);

/**
 * @brief Registers a callback invoked when the peripheral disconnects.
 *
 * @param[in] handle Valid, non-NULL peripheral handle.
 * @param[in] callback Non-NULL callback to register.
 * @param[in] userdata User data passed unchanged to the callback; may be NULL.
 * @param[in,out] out_error Required, non-NULL pointer to NULL or an owned error.
 * @note The callback receives the registered peripheral handle and userdata.
 * @see simpleble_error_t
 */
SIMPLECBLE_EXPORT void simpleble_peripheral_set_callback_on_disconnected(
    simpleble_peripheral_t handle, void (*callback)(simpleble_peripheral_t peripheral, void* userdata), void* userdata,
    simpleble_error_t** out_error);

#ifdef __cplusplus
}
#endif
