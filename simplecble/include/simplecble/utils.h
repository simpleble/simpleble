#pragma once

#include <simplecble/export.h>

#include <simplecble/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the operating system on which SimpleBLE library is currently running.
 *
 * @return A simpleble_os_t value representing the current operating system.
 */
SIMPLECBLE_EXPORT simpleble_os_t simpleble_get_operating_system(void);

/**
 * Returns a string representing the version of the SimpleBLE library.
 *
 * @return A const char pointer to the version string.
 */
SIMPLECBLE_EXPORT const char* simpleble_get_version(void);

/**
 * Returns the company name for a given Bluetooth company identifier code.
 *
 * @param company_id The Bluetooth SIG assigned company identifier.
 * @return A const char pointer to the company name, or "Unknown" if not found.
 *         The returned string is valid until the next call to this function.
 */
SIMPLECBLE_EXPORT const char* simpleble_get_company_name(uint16_t company_id);

#ifdef __cplusplus
}
#endif
