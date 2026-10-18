#include <simplektble/identity.h>

#include <simpleble/SimpleBLE.h>

#include "../Internal.h"

// NOTE: A SimpleCBLE handle points to the public SimpleBLE object it wraps.

const void* simplektble_adapter_identity(simpleble_adapter_t handle) {
    if (handle == nullptr) return nullptr;
    return Org::SimpleKtBLE::Internal<SimpleBLE::Adapter>::of(*static_cast<SimpleBLE::Adapter*>(handle));
}

const void* simplektble_peripheral_identity(simpleble_peripheral_t handle) {
    if (handle == nullptr) return nullptr;
    return Org::SimpleKtBLE::Internal<SimpleBLE::Peripheral>::of(*static_cast<SimpleBLE::Peripheral*>(handle));
}
