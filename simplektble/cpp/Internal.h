#pragma once

namespace Org {
namespace SimpleKtBLE {

/**
 * Reads the SimpleBLE object behind a public Adapter or Peripheral, the way SimpleJSBLE does.
 *
 * SimpleBLE keeps one internal object per adapter and per device, so its address identifies them. Public objects are
 * copies that share it, and underlying() cannot stand in: it returns the operating system's object, which the Dongl
 * and simulated backends do not have.
 */
template <typename Public>
struct Internal : Public {
    static const void* of(const Public& value) { return (value.*(&Internal::internal_)).get(); }
};

}  // namespace SimpleKtBLE
}  // namespace Org
