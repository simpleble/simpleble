#pragma once

#include <simpleble/export.h>

#include <simpleble/Types.h>

namespace SimpleBLE {

OperatingSystem SIMPLEBLE_EXPORT get_operating_system();

std::string SIMPLEBLE_EXPORT get_simpleble_version();

/**
 * Returns the company name for a given Bluetooth company identifier code.
 *
 * @param company_id The Bluetooth SIG assigned company identifier.
 * @return The company name, or "Unknown" if the identifier is not found.
 */
std::string SIMPLEBLE_EXPORT get_company_name(uint16_t company_id);

}  // namespace SimpleBLE
