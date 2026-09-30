#pragma once

#include <chrono>
#include <memory>
#include <string>

#include <simpleble/Types.h>
#include <simpleble/export.h>

namespace SimpleBLE {
class AdapterBase;
}

namespace SimpleBLE::Simulation {

/**
 * A simulated adapter, reported through SimpleBLE::Adapter while its
 * environment is active.
 */
class SIMPLEBLE_EXPORT Adapter {
  public:
    Adapter() = default;
    explicit Adapter(std::shared_ptr<SimpleBLE::AdapterBase> internal);

    bool initialized() const;
    std::string identifier() const;
    BluetoothAddress address() const;

    // Connection policy, applied as the central. Intervals are clamped to 7.5 ms - 4 s.

    /** Interval used when the adapter connects. Default: 30 ms. */
    void set_connection_interval(std::chrono::microseconds interval);
    /** Intervals granted when a device asks for an update. Default: 7.5 ms to 4 s. */
    void set_accepted_connection_intervals(std::chrono::microseconds min, std::chrono::microseconds max);
    /** Longest the adapter spends in one connection event. Takes effect immediately. Default: 3.75 ms. */
    void set_max_event_length(std::chrono::microseconds length);
    /** A connection without a valid packet for this long is lost. Default: 4 s. */
    void set_supervision_timeout(std::chrono::milliseconds timeout);

    std::shared_ptr<SimpleBLE::AdapterBase> internal() const;

  private:
    std::shared_ptr<SimpleBLE::AdapterBase> internal_;
};

}  // namespace SimpleBLE::Simulation
