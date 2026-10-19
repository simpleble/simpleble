#pragma once

namespace SimpleCBLE {

// NOTE: Handles copy a public SimpleBLE object, and the copies share one internal object, so its address identifies
// the object behind a handle. This reads the protected internal_ member, as SimpleJSBLE and SimpleKtBLE do.
template <typename Public>
struct Identity : Public {
    static const void* of(const Public& value) { return (value.*(&Identity::internal_)).get(); }
};

}  // namespace SimpleCBLE
