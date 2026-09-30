#pragma once

#include <chrono>
#include <cstdint>
#include <memory>

#include <simpleble/export.h>

namespace SimpleBLE::Simulation::Internal {
class ConnectionCore;
}

namespace SimpleBLE::Simulation {

/**
 * Handle to a connection between a simulated adapter and a device.
 *
 * Copies refer to the same connection and may be kept after the callback that
 * provided them. Once the connection closes, actions on it report
 * NOT_CONNECTED.
 */
class SIMPLEBLE_EXPORT Connection {
  public:
    Connection() = default;
    explicit Connection(std::shared_ptr<Internal::ConnectionCore> internal);

    bool is_connected() const;
    uint16_t mtu() const;
    /** The current connection interval. */
    std::chrono::microseconds interval() const;

    bool operator==(const Connection& other) const;
    bool operator!=(const Connection& other) const { return !(*this == other); }

    std::shared_ptr<Internal::ConnectionCore> internal() const;

  private:
    std::weak_ptr<Internal::ConnectionCore> internal_;
};

}  // namespace SimpleBLE::Simulation
