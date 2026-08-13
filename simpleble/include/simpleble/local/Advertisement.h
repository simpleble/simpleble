#pragma once

#include <optional>
#include <string>
#include <vector>

#include <simpleble/export.h>

#include <simpleble/Types.h>

namespace SimpleBLE::Local {

struct SIMPLEBLE_EXPORT Advertisement {
    std::optional<std::string> local_name;

    /**
     * If left empty, backends should advertise the service UUIDs configured on
     * the local peripheral when the platform supports doing so.
     */
    std::vector<BluetoothUUID> service_uuids;
};

}  // namespace SimpleBLE::Local
