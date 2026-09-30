#pragma once

#include <memory>
#include <string>
#include <utility>

#include <simpleble/Types.h>
#include <simpleble/export.h>
#include <simpleble/simulation/Adapter.h>
#include <simpleble/simulation/Device.h>
#include <simpleble/simulation/Link.h>

namespace SimpleBLE::Simulation::Internal {
class EnvironmentCore;
}

namespace SimpleBLE::Simulation {

/**
 * Owns simulated adapters and devices.
 *
 * Only one environment can be active at a time. Destroying an active
 * environment deactivates it, which closes its connections.
 */
class SIMPLEBLE_EXPORT Environment {
  public:
    Environment();
    ~Environment();

    Environment(const Environment&) = delete;
    Environment& operator=(const Environment&) = delete;

    Adapter add_adapter(const std::string& identifier, const BluetoothAddress& address);

    void add_device(std::shared_ptr<Device> device);

    /** The radio link between an adapter and a device of this environment. */
    Link link(const Adapter& adapter, const std::shared_ptr<Device>& device);

    template <typename T, typename... Args>
    std::shared_ptr<T> add_device(Args&&... args) {
        auto device = std::make_shared<T>(std::forward<Args>(args)...);
        add_device(device);
        return device;
    }

    void activate();
    void deactivate();
    bool is_active() const;

  private:
    std::shared_ptr<Internal::EnvironmentCore> internal_;
};

}  // namespace SimpleBLE::Simulation
